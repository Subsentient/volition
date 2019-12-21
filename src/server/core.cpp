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



#include <stdio.h>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "../libvolition/include/common.h"
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/utils.h"
#include "core.h"
#include "clients.h"
#include "db.h"
#include "logger.h"
#include "routines.h"

#include <map>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif //WIN32

//Static globals and types

static bool SkipHeadRelease;
static bool ResetLoopAfter;
static bool InsideHandleInterface;
static Clients::ClientObj *ActiveClient;

//External globals
Net::ServerDescriptor ServerDesc;

//Prototypes
static void MasterLoop(void);

//Function definitions
bool Core::ValidServerAdminLogin(const char *const Username, const char *const Password)
{
	DB::GlobalConfigDBEntry *Lookup = DB::LookupGlobalConfigDBEntry("AdminLogins");

	if (!Lookup && !*Username && !*Password) return true; //No passwords specified, so we accept emptiness.

	std::vector<VLString> *LoginPairs = Utils::SplitTextByCharacter(Lookup->Value, '|');

	delete Lookup;
	
	if (!LoginPairs || !LoginPairs->size())
	{
		delete LoginPairs;
		return false;
	}

	for (size_t Inc = 0; Inc < LoginPairs->size(); ++Inc)
	{
		std::vector<VLString> *UserPassPair =  Utils::SplitTextByCharacter(LoginPairs->at(Inc), ',');

		if (!UserPassPair || UserPassPair->size() != 2)
		{ //Broken/corrupted. Skip I guess.
			delete UserPassPair;
			continue;
		}

		const VLString PairUsername = UserPassPair->at(0);
		const VLString PairPassword = UserPassPair->at(1);

		delete UserPassPair;

		if (PairUsername == Username && PairPassword == Password)
		{
			delete LoginPairs;
			return true;
		}
	}
	
	return false;
}

void Core::SignalSkipHeadRelease(void)
{
	SkipHeadRelease = true;
}

void Core::SignalResetLoopAfter(void)
{
	ResetLoopAfter = true;
}

bool Core::CheckInsideHandleInterface(void)
{
	return InsideHandleInterface;
}

bool Core::CheckIsActiveClient(void *Client)
{
	return Client == ActiveClient;
}

int main(const int argc, const char **argv)
{
	srand(time(nullptr) ^ clock());
	
#ifdef WIN32
	WSADATA WSAData;

    if (WSAStartup(MAKEWORD(2,2), &WSAData) != 0)
    { /*Initialize winsock*/
        fprintf(stderr, "Unable to initialize WinSock2!");
        exit(1);
    }
#endif //WIN32

	Net::InitNetcore(true);

	puts(VLString("Volition server -- Binary compatible with r") + VLString::IntToString(Conation::PROTOCOL_VERSION) + " series.\nStarting up...");
	Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("Volition server for Conation protocol version r") + VLString::IntToString(Conation::PROTOCOL_VERSION) + " starting up.");
	
	if (!Utils::FileExists(SERVER_DBFILE))
	{
		if (!DB::InitializeEmptyDB())
		{
			fputs("Failed to create and initialize database file \"" SERVER_DBFILE "\"!\n"
					"Cannot continue.\n", stderr);
			exit(1);
		}
	}
	
	//Load known routines into memory so we can monitor them more efficiently than constantly polling the database.
	Routines::ScanRoutineDB();
	
	if (!(ServerDesc = Net::InitServer(MASTER_PORT)))
	{
		fputs("Failed to fire up server: Net::InitServer() failed.\n", stderr);
		Utils::vl_sleep(5000); //Let us see wtf is going on.
		exit(1);
	}
	
	printf("Listening on port %d, startup complete.\n", MASTER_PORT);

	while (1) MasterLoop();
	
	return 0;
}

static void MasterLoop(void)
{
	static uint32_t IterCounter = 0;
	
	fd_set Set{};		
	FD_SET(ServerDesc.Descriptor, &Set);
	fd_set ErrSet = Set;
	struct timeval Time{}; //Zero for immediate return.
	
	if (select(ServerDesc.Descriptor + 1, &Set, nullptr, &ErrSet, &Time) == 1)
	{
		Clients::ClientObj *NewClient = Clients::AcceptClient_Auth(ServerDesc);
		
		(void)NewClient;
	}

RestartLoop:
	for (auto Iter = Clients::ClientMap.begin(); Iter != Clients::ClientMap.end(); ++Iter)
	{
		Clients::ClientObj *const Client = Iter->second;

		if (Client->HasNetworkError())
		{
			Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_CONNBREAK);
			goto RestartLoop;
		}
		
		Conation::ConationStream *Stream = Client->RecvStream_Acquire();
		
		if (!Stream) goto NothingToDo;
		
		InsideHandleInterface = true;
		ActiveClient = Client;
		
		Clients::HandleClientInterface(Client, Stream);
		
		InsideHandleInterface = false;
		ActiveClient = nullptr;
		
	NothingToDo:
		if (SkipHeadRelease)
		{
			SkipHeadRelease = false;
			goto RestartLoop;
		}
		else
		{
			Client->RecvStream_Release();
		}
		
		if (ResetLoopAfter)
		{
			ResetLoopAfter = false;
			goto RestartLoop;
		}
	}
	Clients::CheckPingsAndQueues();

	if (IterCounter == 1000 / SERVER_CORE_FREQUENCY_MS)
	{
		IterCounter = 0;
		
		Routines::ProcessScheduledRoutines(time(nullptr));
	}
	else ++IterCounter;
	
	Utils::vl_sleep(SERVER_CORE_FREQUENCY_MS);
}

