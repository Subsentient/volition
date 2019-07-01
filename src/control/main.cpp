/**
* This file is part of Volition.

* Volition is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* Volition is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with Volition.  If not, see <https://www.gnu.org/licenses/>.
**/


#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/utils.h"
#include "../libvolition/include/netscheduler.h"

#include <gtk/gtk.h>
#include "gui_base.h"
#include "gui_mainwindow.h"
#include "gui_dialogs.h"
#include "gui_icons.h"
#include "config.h"
#include "interface.h"
#include "main.h"
#include "ticker.h"
#include "scriptscanner.h"

#ifdef WIN32
#include <winsock2.h>
#endif

//Globals
static NetScheduler::ReadQueue SockReadQueue;
static NetScheduler::WriteQueue SockWriteQueue;

static NetScheduler::SchedulerStatusObj ReadOperationStatus;
static NetScheduler::SchedulerStatusObj WriteOperationStatus;

static Net::ClientDescriptor SocketDescriptor;

Net::PingTracker Main::PingTrack;

//Prototypes
static gboolean PrimaryLoop(void* = nullptr);
static gboolean UpdateNetProgress(void* = nullptr);
static gboolean UpdateTicker(void* = nullptr);
static void FailureDismissCallback(const void *);

int main(int argc, char **argv)
{
#ifdef WIN32
	WSADATA WSAData;

    if (WSAStartup(MAKEWORD(2,2), &WSAData) != 0)
    { /*Initialize winsock*/
        fputs("Unable to initialize WinSock2!\n", stderr);
        exit(1);
    }
#endif //WIN32

	gtk_init(&argc, &argv);

	//Because some of our dialogs use them to relay crucial information.
	g_object_set(gtk_settings_get_default(), "gtk-button-images", TRUE, nullptr);

	//Set theme for Windows.

	if (!Config::ReadConfig())
	{
		fputs("Failed to read configuration!\n", stderr);
		GuiDialogs::CmdStatusDialog *FailMsg = new GuiDialogs::CmdStatusDialog("Failure reading configuration", NetCmdStatus(false, STATUS_MISSING, "Unable to read configuration file control.conf!\nCheck that your configuration is present\nand is neither corrupt nor missing required keys."), FailureDismissCallback);
		
		FailMsg->Show();
		gtk_main();
	}

	const VLString &RootCertPath = Config::GetKey("RootCertificate");
	const std::vector<uint8_t> *RootCertData = Utils::Slurp(RootCertPath);

	if (!RootCertData)
	{
		fprintf(stderr, "CRITICAL: Cannot load X509 root certificate at file path \"%s\"!\n", +RootCertPath);
		exit(1);
	}
	
	Net::LoadRootCert((const char*)RootCertData->data());

	delete RootCertData;
	
	Net::InitNetcore(false);
	
	if (!GuiIcons::LoadAllIcons())
	{
		fputs("Unable to load icons!\n", stderr);
		GuiDialogs::CmdStatusDialog *FailMsg = new GuiDialogs::CmdStatusDialog("Failure loading icons", NetCmdStatus(false, STATUS_MISSING, "Unable to load icons in directory \"./icons/\"!"), FailureDismissCallback);
		
		FailMsg->Show();
		gtk_main();
	}
	
	if (!ScriptScanner::ScanScriptsDirectory())
	{
		fputs("Unable to scan scripts directory!\n", stderr);
		GuiDialogs::CmdStatusDialog *FailMsg = new GuiDialogs::CmdStatusDialog("Failure scanning scripts directory", NetCmdStatus(false, STATUS_MISSING, VLString("Unable to scan scripts directory ") + Config::GetKey("ScriptsDirectory") + "!"), FailureDismissCallback);
		
		FailMsg->Show();
		gtk_main();
	}
	
	GuiBase::LoginScreen *Login = new GuiBase::LoginScreen;
	
	Login->Show();
	
	gtk_main();

	return 0;
}

static void FailureDismissCallback(const void *)
{
	exit(1);
}

void Main::InitiateLogin(GuiBase::LoginScreen *Instance)
{
	const VLString &Username = Instance->GetUsernameField();
	const VLString &Password = Instance->GetPasswordField();

	delete Instance;

	GuiBase::LoadingScreen *LoadScr = new GuiBase::LoadingScreen("Connecting to Volition server");
	
	LoadScr->Show();
	gtk_main_iteration_do(false);
	
	if (!Interface::Establish(Username, Password, Config::GetKey("ServerName"), &SocketDescriptor))
	{
		NetCmdStatus FailStatus(false, STATUS_FAILED, VLString("Failed to connect to server \"") + Config::GetKey("ServerName") + "\".");
		GuiDialogs::CmdStatusDialog *FailMsg = new GuiDialogs::CmdStatusDialog("Unable to connect", FailStatus, FailureDismissCallback);

		FailMsg->Show();
		gtk_main_iteration_do(false);

		delete LoadScr;
		return;
	}

	//Configure read and write queues. We NEVER delete these.
	SockReadQueue.SetStatusObj(&ReadOperationStatus);
	SockWriteQueue.SetStatusObj(&WriteOperationStatus);
	
	SockReadQueue.Begin(SocketDescriptor);
	SockWriteQueue.Begin(SocketDescriptor);
	
	LoadScr->SetStatusText("Downloading index of nodes");
	LoadScr->SetProgressBarFraction(0.1);

	gtk_main_iteration_do(false);

	Interface::RequestIndex();

	g_timeout_add(100, (GSourceFunc)PrimaryLoop, nullptr);
	g_timeout_add(250, (GSourceFunc)UpdateTicker, nullptr);

	gtk_main_iteration_do(false);
}

void Main::TerminateLink(void)
{
	Conation::ConationStream *QuitNotification = new Conation::ConationStream(CMDCODE_ANY_DISCONNECT, 0, 0);

	SockWriteQueue.Push(QuitNotification);
	
	uint64_t OnWriteQueue = 0;
	
	do
	{
		WriteOperationStatus.GetValues(nullptr, nullptr, &OnWriteQueue, nullptr);
	} while (OnWriteQueue);
	
	SockReadQueue.StopThread(1000, 50);
	SockWriteQueue.StopThread(1000, 50);

	Net::Close(SocketDescriptor);

}
const Net::ClientDescriptor &Main::GetSocketDescriptor(void)
{
	return SocketDescriptor;
}

static gboolean PrimaryLoop(void*)
{
Restart:
	;
	if ( (!Main::PingTrack.CheckPingout() &&
		ReadOperationStatus.GetSecsSinceActivity() >= PING_PINGOUT_TIME_SECS &&
		WriteOperationStatus.GetSecsSinceActivity() >= PING_PINGOUT_TIME_SECS) || SockReadQueue.HasError())
	{
		fputs(VLString("Socket ") + (SockReadQueue.HasError() ? "slammed shut by remote server" : "timed out") + "! Shutting down.\n", stderr);
		NetCmdStatus FailStatus(false, STATUS_FAILED, "Connection to server has been lost!\nCannot continue. Shutting down.");
		
		if (GuiMainWindow::MainWindowScreen *Temp = (GuiMainWindow::MainWindowScreen*)GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW))
		{
			Temp->Set_Clickable(false);
		}
		GuiDialogs::CmdStatusDialog *FailMsg = new GuiDialogs::CmdStatusDialog("Connection lost", FailStatus, FailureDismissCallback);
		
		FailMsg->Show();
		return false; //Stop the loop now.
	}
	
	UpdateNetProgress(); //Draw progress for status bar.
	
	Conation::ConationStream *Stream = SockReadQueue.Head_Acquire();
	
	//Poll for data.
	if (Stream)
	{
		Interface::HandleServerInterface(Stream);
		
		SockReadQueue.Head_Release(true);
		
		goto Restart; //We do this so we can keep receiving results without a delay.
	}
	
	SockReadQueue.Head_Release(true);
	
	Utils::vl_sleep(10);
	return true; //Must return true.
}

static gboolean UpdateTicker(void*)
{
	GuiMainWindow::MainWindowScreen *Screen = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));

	if (!Screen) return true;

	//We have a screen. Rescan and update the ticker tree.
	Screen->LoadTickerTree();
	
	return true;
}

static gboolean UpdateNetProgress(void*)
{
	GuiMainWindow::MainWindowScreen *Screen = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));
	
	if (!Screen) return false; //For another day.
	
	uint64_t RecvTotal = 0, SendTotal = 0;
	uint64_t RecvTransferred = 0, SendTransferred = 0;
	uint64_t RecvNumInQueue = 0, SendNumInQueue = 0;
	
	NetScheduler::SchedulerStatusObj::OperationType RecvCurrentOperation;
	NetScheduler::SchedulerStatusObj::OperationType SendCurrentOperation;
	
	ReadOperationStatus.GetValues(&RecvTotal, &RecvTransferred, &RecvNumInQueue, &RecvCurrentOperation);
	WriteOperationStatus.GetValues(&SendTotal, &SendTransferred, &SendNumInQueue, &SendCurrentOperation);
	
	char Buffer[2][2048]{};
	
	if (RecvCurrentOperation == NetScheduler::SchedulerStatusObj::OPERATION_RECV)
	{
		snprintf(Buffer[0], sizeof Buffer[0], "Q: %llu, %lluB/%lluB",
				(unsigned long long)RecvNumInQueue,
				(unsigned long long)RecvTransferred,
				(unsigned long long)RecvTotal);
	}
	else strcpy(Buffer[0], "inactive");
	
	if (SendCurrentOperation == NetScheduler::SchedulerStatusObj::OPERATION_SEND)
	{
	
		snprintf(Buffer[1], sizeof Buffer[1], "Q: %llu, %lluB/%lluB",
				(unsigned long long)SendNumInQueue,
				(unsigned long long)SendTransferred,
				(unsigned long long)SendTotal);
	}
	else strcpy(Buffer[1], "inactive");
	
	const VLString &Result = VLString("Recv: ") + (const char*)Buffer[0] + " | Send: " + (const char*)Buffer[1];
	
	Screen->SetStatusBarText(Result);	
	return false;
}

NetScheduler::ReadQueue &Main::GetReadQueue(void)
{
	return SockReadQueue;
}
NetScheduler::WriteQueue &Main::GetWriteQueue(void)
{
	return SockWriteQueue;
}
