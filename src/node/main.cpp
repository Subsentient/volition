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
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/utils.h"
#include "../libvolition/include/netscheduler.h"
#include "interface.h"
#include "jobs.h"
#include "identity_module.h"
#include "files.h"
#include "script.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>

#ifdef WIN32
#include <winsock2.h>
#endif //WIN32
#include "main.h"
#include "updates.h"

//prototypes
static void MasterLoop(Net::ClientDescriptor &Descriptor);
static inline bool PingedOut(void);

static int argc_;
static char **argv_;

static Net::ClientDescriptor SocketDescriptor;

static NetScheduler::ReadQueue MasterReadQueue;
static NetScheduler::WriteQueue MasterWriteQueue;

static NetScheduler::SchedulerStatusObj ReadQueueStatus;
static NetScheduler::SchedulerStatusObj WriteQueueStatus;

Net::PingTracker Main::PingTrack;

static inline bool PingedOut(void)
{
	return !Main::PingTrack.CheckPingout() &&
			ReadQueueStatus.GetSecsSinceActivity() >= PING_PINGOUT_TIME_SECS &&
			WriteQueueStatus.GetSecsSinceActivity() >= PING_PINGOUT_TIME_SECS;
}

int main(int argc, char **argv)
{
	argc_ = argc;
	argv_ = argv;
	
	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);
	
	srand(time(nullptr) ^ clock());
	
	const time_t CompileTime = IdentityModule::GetCompileTime();
	
	struct tm TimeStruct = *localtime(&CompileTime);
	char TimeBuf[128]{};
	
	strftime(TimeBuf, sizeof TimeBuf, "%a %m/%d/%Y %I:%M:%S %p", &TimeStruct);
	
	std::cout << "Volition node software revision " << +IdentityModule::GetNodeRevision() << " starting. Node compiled at " << (const char*)TimeBuf << std::endl << std::endl;

	
#ifdef WIN32
	WSADATA WSAData{};

    VLASSERT_ERRMSG(WSAStartup(MAKEWORD(2,2), &WSAData) == 0, "Failed to initialize WinSock2!");

#endif //WIN32


	///Initialize the SSL-enabled networking core.
	Net::LoadRootCert(IdentityModule::GetCertificate());
	Net::InitNetcore(false);
	
	///Process updates
	Updates::HandleUpdateRecovery();

	Main::Begin();
}

void Main::Begin(const bool JustUpdated)
{
	while (!(SocketDescriptor = Interface::Establish(IdentityModule::GetServerAddr() ) ).Internal ) Utils::vl_sleep(1000);

	MasterReadQueue.SetStatusObj(&ReadQueueStatus);
	MasterWriteQueue.SetStatusObj(&WriteQueueStatus);
	
	MasterReadQueue.Begin(SocketDescriptor);
	MasterWriteQueue.Begin(SocketDescriptor);
	
	if (JustUpdated)
	{
		//Tell the server we're alright.
		Conation::ConationStream Msg(CMDCODE_B2C_USEUPDATE, Conation::IDENT_ISREPORT_BIT, 0u);
		Msg.Push_NetCmdStatus(true);
		Msg.Push_String(IdentityModule::GetNodeRevision());
		
		MasterWriteQueue.Push(new Conation::ConationStream(Msg));
	}
	
	VLScopedPtr<VLThreads::Thread*> StartupScriptThread { Script::SpawnInitScript() };
	
	//Do the things.
	while (1) MasterLoop(SocketDescriptor);
}


void Main::MurderAllThreads(void)
{ /*This function is necessary for updates to work on Windows.
	Windows doesn't let a process die until all its threads have aborted themselves,
	and Windows also doesn't let you overwrite a running process image.*/
	
	/*We want to try and stop them this way because just killing the queue threads will cause a memory leak
	* and possibly corrupt network data being sent/received from the server.*/
	MasterReadQueue.StopThread();
	MasterWriteQueue.StopThread();
	
	//Job threads will also prevent us from updating on Windows, any thread really.
	Jobs::KillAllJobs();
}

static void MasterLoop(Net::ClientDescriptor &Descriptor)
{
Restart:
	//Poll for it.
	if (MasterReadQueue.HasError() || MasterWriteQueue.HasError() || PingedOut())
	{
		VLDEBUG("Server lost connection. Attempting to reconnect.");
		MasterReadQueue.StopThread();
		MasterWriteQueue.StopThread();
		
		Net::Close(Descriptor);
		
		while (!(Descriptor = Interface::Establish(IdentityModule::GetServerAddr() ) ).Internal) Utils::vl_sleep(1000);

		VLDEBUG("Reconnect succeeded. Restarting network queues.");
		//Restart network queues.
		MasterReadQueue.ClearError();
		MasterWriteQueue.ClearError();
		
		MasterReadQueue.Begin(Descriptor);
		MasterWriteQueue.Begin(Descriptor);
		
		VLDEBUG("Queues restarted.");
		
		return; //Restart the loop to be sure.
	}
	
	
	Conation::ConationStream *Stream = MasterReadQueue.Head_Acquire();
	
	if (Stream)
	{
		VLDEBUG("Acquired stream with command code " + CommandCodeToString(Stream->GetCommandCode()) + " and flags " + Utils::ToBinaryString(Stream->GetCmdIdentFlags()));
		Interface::HandleServerInterface(Stream);
	}
	
	MasterReadQueue.Head_Release(Stream != nullptr);
	
	if (Stream) goto Restart; //We're just checking if it's a null pointer, so this is still legal.

	Jobs::ProcessCompletedJobs();
	
	Utils::vl_sleep(50);
}

void Main::ForceReleaseMutexes(void)
{ //For when we call exit() mostly. If you use this function like a retard I'm going to gut you like a fish.
	MasterReadQueue.Head_Release(false);
}

void Main::InitNetQueues(void)
{
	MasterReadQueue.Begin();
	MasterWriteQueue.Begin();
}

char **Main::GetArgvData(int **ArgcOut)
{
	if (ArgcOut) *ArgcOut = &argc_;
	
	return argv_;
}

const Net::ClientDescriptor *Main::GetSocketDescriptor(void)
{
	return &SocketDescriptor;
}

void Main::PushStreamToWriteQueue(Conation::ConationStream *Stream)
{
	MasterWriteQueue.Push(Stream);
}

void Main::PushStreamToWriteQueue(const Conation::ConationStream &Stream)
{
	MasterWriteQueue.Push(new Conation::ConationStream(Stream.GetHeader(), Stream.GetArgData()));
}

NetScheduler::ReadQueue &Main::GetReadQueue(void)
{
	return MasterReadQueue;
}

NetScheduler::WriteQueue &Main::GetWriteQueue(void)
{
	return MasterWriteQueue;
}

