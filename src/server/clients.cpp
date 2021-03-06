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
#include "clients.h"
#include "core.h"
#include "cmdhandling.h"
#include "db.h"
#include "nodeupdates.h"
#include "logger.h"
#include "routines.h"

#include <fcntl.h>
#include <list>
#include <vector>
#include <map>

#include <time.h>
#include <sys/time.h>
#include <assert.h>
std::map<VLString, VLScopedPtr<Clients::ClientObj*> > Clients::ClientMap;
static Clients::ClientObj *CurrentAdmin;

#define MK_TEXT(x) Clients::x, #x

static std::map<Clients::NodeDeauthType, VLString> NodeDeauthTypeText
{
	{ MK_TEXT(NODE_DEAUTH_INVALID) },
	{ MK_TEXT(NODE_DEAUTH_CONNBREAK) },//Broken connection.
	{ MK_TEXT(NODE_DEAUTH_PINGOUT) },//Pinged out
	{ MK_TEXT(NODE_DEAUTH_AUTO) },//Node or admin disconnected on its own
	{ MK_TEXT(NODE_DEAUTH_ORDER) },//Node or admin was following an order to shut down.
	{ MK_TEXT(NODE_DEAUTH_EVIL) },//Malicious client.
	{ MK_TEXT(NODE_DEAUTH_BADAUTHTOKEN) },//Bad authentication token) },either revoked or connecting with an invalid one from start.
	{ MK_TEXT(NODE_DEAUTH_MAX) },
};

//Function definitions.
bool Clients::AddClient(const ClientObj *const NewClient)
{
	if (ClientMap.count(NewClient->GetID())) return false;
	
	ClientMap.emplace(NewClient->GetID(), const_cast<ClientObj*>(NewClient));
	
	if (NewClient->GetID() == "ADMIN")
	{
		CurrentAdmin = const_cast<ClientObj*>(NewClient);
	}

	return true;
}

bool Clients::DeleteClient(const ClientObj *const Ptr)
{
	if (!ClientMap.count(Ptr->GetID())) return false;
	
	ClientMap.erase(Ptr->GetID());

	if (Ptr == CurrentAdmin) CurrentAdmin = nullptr;
	
	return true;
}

bool Clients::DeleteClient(const char *const ID)
{
	if (!ClientMap.count(ID)) return false;
	
	const ClientObj *const Ptr = ClientMap.at(ID);
	
	ClientMap.erase(ID);

	if (Ptr == CurrentAdmin) CurrentAdmin = nullptr;

	return false;
}

Clients::ClientObj *Clients::LookupClient(const VLString &Identity)
{
	if (!ClientMap.count(Identity)) return nullptr;
	
	return +ClientMap.at(Identity);
}

Clients::ClientObj *Clients::LookupCurAdmin(void)
{
	return CurrentAdmin;
}

Clients::ClientObj *Clients::AcceptClient_Auth(const Net::ServerDescriptor &Desc)
{
	if (!Desc) return nullptr;
	
	Net::ClientDescriptor ClientDesc{};
	
	char IPBuf[512] = { 0 };
	
	if (!Net::AcceptClient(Desc, &ClientDesc, IPBuf, sizeof IPBuf))
	{
		Logger::WriteLogLine(Logger::LOGITEM_SYSWARN, "Client attempted to connect but Net::AcceptClient() failed.");
		return nullptr;
	}

	VLScopedPtr<ClientObj*> NewClient { new ClientObj { ClientDesc } };
	
	NewClient->IPAddr = IPBuf;

	VLScopedPtr<Conation::ConationStream*> Stream;
	
	try
	{
		Stream = new Conation::ConationStream(ClientDesc);
	}
	catch (...)
	{
		Logger::WriteLogLine(Logger::LOGITEM_SYSERROR, VLString("Exception occurred decoding and receiving stream from client at ") + NewClient->IPAddr);
		goto EasyError;
	}
	
	//Download the message.
	if (!Stream)
	{
		Logger::WriteLogLine(Logger::LOGITEM_SYSWARN, VLString("Failed to download stream from client at ") + NewClient->IPAddr);
		
	EasyError:
		Logger::WriteLogLine(Logger::LOGITEM_CONN, VLString("Aborting client accept from IP ") + NewClient->IPAddr);

		Net::Close(ClientDesc);

		return nullptr;
	}

	uint8_t Flags = 0;
	uint64_t Ident = 0;
	
	Stream->GetCommandIdent(&Flags, &Ident);

	//We determine what kind of client they are by what argument sequence they send us.
	const bool IsNodeArgSequence = Stream->VerifyArgTypes({Conation::ARGTYPE_INT32, Conation::ARGTYPE_STRING,
														Conation::ARGTYPE_STRING, Conation::ARGTYPE_STRING,
														Conation::ARGTYPE_STRING});
														
	const bool IsAdminArgSequence = Stream->VerifyArgTypes({Conation::ARGTYPE_INT32, Conation::ARGTYPE_STRING,
														Conation::ARGTYPE_STRING});

	if ((!IsAdminArgSequence && !IsNodeArgSequence) ||
		Stream->GetCommandCode() != CMDCODE_B2S_AUTH || (Flags & Conation::IDENT_ISREPORT_BIT) || Ident != 0)
	{
		Logger::WriteLogLine(Logger::LOGITEM_SYSWARN, VLString("Stream failed integrity test, client at ") + NewClient->IPAddr);
		goto EasyError;
	}

	int ProtocolVersion = Stream->Pop_Int32();
	///Process arguments.
	if (ProtocolVersion != Conation::PROTOCOL_VERSION)
	{ //Incompatible protocol version.
		VLString Buf(512);
		snprintf(Buf.GetBuffer(), Buf.GetCapacity(), "Client attempting to connect with invalid protocol version. Expected %i, got %i\n", Conation::PROTOCOL_VERSION, ProtocolVersion);

		Logger::WriteLogLine(Logger::LOGITEM_CONN, Buf);
		goto EasyError;
	}
	
	//Are they a node or an admin?

	if (IsNodeArgSequence)
	{
		const VLString ID = Stream->Pop_String();

		if (ID == "ADMIN")
		{
			Clients::ProcessNodeDisconnect(NewClient, Clients::NODE_DEAUTH_EVIL);
			Logger::WriteLogLine(Logger::LOGITEM_SECUREWARN, VLString("Node at IP ") + NewClient->IPAddr + " attempted to connect using ADMIN designation.");
		}

		if (Clients::LookupClient(ID) != nullptr)
		{
			VLString Buf(1024);
			snprintf(Buf.GetBuffer(), Buf.GetCapacity(), "Node \"%s\" trying to connect twice, this attempt from IP %s. Disallowed. Aborting authentication.\n", +ID, +NewClient->GetIPAddr());
			Logger::WriteLogLine(Logger::LOGITEM_CONN, Buf);
			goto EasyError;
		}
		//Get the client ID
		NewClient->ID = ID;

		//Get the authorization token.
		NewClient->AuthToken = Stream->Pop_String();
		
		DB::AuthTokensDBEntry *TokenLookup = DB::LookupAuthToken(NewClient->AuthToken);
		
		if (!TokenLookup) ///DISALLOWED!!! Token either bogus or we revoked it. We're just gonna close the connection.
		{
			Logger::WriteLogLine(Logger::LOGITEM_PERMS, VLString("Node \"") + ID + "\"::" + NewClient->IPAddr + " provided invalid authentication token \"" + NewClient->AuthToken + "\".");

			Clients::ProcessNodeDisconnect(NewClient, Clients::NODE_DEAUTH_BADAUTHTOKEN);
			
			return nullptr;
		}
		
		//Get platform string
		NewClient->PlatformString = Stream->Pop_String();
		
		//Get node revision
		NewClient->NodeRevision = Stream->Pop_String();

		Logger::WriteLogLine(Logger::LOGITEM_CONN, VLString("Accepted node \"") + NewClient->ID + "\" at IP " + NewClient->IPAddr + " using authentication token \"" + NewClient->AuthToken + "\".");
	} //we need a password if we're an admin.
	else
	{
		//We let the ID be empty on purpose.
		
		//Get username
		const VLString &Username = Stream->Pop_String(); 

		//Get password
		const VLString &Password = Stream->Pop_String();
		
		/**Verify login info**/
		if (!Core::ValidServerAdminLogin(Username, Password))
		{ //Wrong password, get fucked.
			Conation::ConationStream *Response = new Conation::ConationStream(CMDCODE_B2S_AUTH, true, 0u);
			Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_ACCESSDENIED, "Invalid administrator login provided. Connection terminates."));
			
			NewClient->SendStream(Response);

			Logger::WriteLogLine(Logger::LOGITEM_PERMS, VLString("Admin login failed from client at IP ") + NewClient->IPAddr + " using username \"" + Username + "\".");
			goto EasyError;
		}

		Logger::WriteLogLine(Logger::LOGITEM_CONN, VLString("Admin \"") + Username + "\" has connected from IP " + NewClient->IPAddr);

		NewClient->ID = "ADMIN";
	}


	//Tell the client they're accepted.
	Conation::ConationStream *Response = new Conation::ConationStream(CMDCODE_B2S_AUTH, true, 0u);
	
	Response->Push_NetCmdStatus(NetCmdStatus(true, STATUS_OK, NewClient->ID == "ADMIN" ?
													"Welcome, volition administrator. Please use volition responsibly."
													: VLString("Greetings, node ") + NewClient->ID + ". Your presence is now registered.")); //Yes, they're allowed in.
	
	///Send our response to the client
	NewClient->SendStream(Response);
	

	//Save connection time.
	NewClient->ConnectedTime = time(nullptr);

	///We're now far enough along that we know we want to keep this client.
	ClientObj *const StoredClient = NewClient.Forget();
	
	if (IsAdminArgSequence)
	{
		if (CurrentAdmin != nullptr)
		{
			Conation::ConationStream *DieMsg = new Conation::ConationStream(CMDCODE_S2A_ADMINDEAUTH, false, 0u);
			
			DieMsg->Push_String("New administrator has connected.");
			
			Logger::WriteLogLine(Logger::LOGITEM_CONN, VLString("Deauthenticating previous administrator at IP ") + CurrentAdmin->IPAddr);

			//Hopefully it makes it out in time.
			CurrentAdmin->SendStream(DieMsg);
			
			Clients::ProcessNodeDisconnect(CurrentAdmin, Clients::NODE_DEAUTH_INVALID);
		}
		
		Clients::AddClient(StoredClient);
		
		assert(CurrentAdmin == StoredClient);
	}
	else
	{
		Clients::AddClient(StoredClient);
		
		//Load node group.
		if (DB::NodeDBEntry *Lookup = DB::LookupNodeInfo(StoredClient->GetID()))
		{
			StoredClient->Group = Lookup->Group;
			if (StoredClient->Group)
			{
				Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("Merged node \"") + StoredClient->GetID() + "\" into assigned group \"" + StoredClient->Group + "\".");
			}
			else
			{
				Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("Node \"") + StoredClient->GetID() + "\" has no group, merging into the unsorted group.");
			}
		}
		
		//Update on-disk database for this node.
		if (!DB::UpdateNodeDB(StoredClient->GetID(), StoredClient->GetPlatformString(), StoredClient->GetNodeRevision(),
								StoredClient->GetGroup(), StoredClient->GetConnectedTime()))
		{
			Logger::WriteLogLine(Logger::LOGITEM_SYSERROR, VLString("Failed to update database for connecting node \"") + StoredClient->GetID() + "\"!");
		}

		//Notify Admin of new node connecting.
		CmdHandling::NotifyAdmin_NodeChange(StoredClient->GetID(), true);	

		//Transmit any updated binaries we have for this node's platform.
		NodeUpdates::HandleUpdatesForNode(StoredClient);

		//Any routines they're supposed to do on startup?
		Routines::ProcessOnConnectRoutines(StoredClient);
	}
	
	//Configure network scheduling status reporting objects.
	StoredClient->ClientReadQueue->SetStatusObj(StoredClient->ReadQueueStatus);
	StoredClient->ClientWriteQueue->SetStatusObj(StoredClient->WriteQueueStatus);
	
	//Begin fireup of network scheduling threads.
	StoredClient->ClientReadQueue->Begin();
	StoredClient->ClientWriteQueue->Begin();

	return StoredClient;
	
}

bool Clients::ClientObj::SendStream(Conation::ConationStream *Stream)
{
	this->ClientWriteQueue->Push(Stream);
	return true;
}

bool Clients::ClientObj::SendStream(Conation::ConationStream &Stream)
{
	Conation::ConationStream *Clone = new Conation::ConationStream(Stream);
	
	return this->SendStream(Clone);
}

bool Clients::HandleClientInterface(ClientObj *Client, Conation::ConationStream *Stream)
{
#ifdef DEBUG
	puts("Entered Clients::HandleClientInterface()");
#endif

	uint8_t Flags = 0;
	uint64_t Ident = 0u;
	
	Stream->GetCommandIdent(&Flags, &Ident);
	
	if		(Flags & Conation::IDENT_ISN2N_BIT) //Node to node communications are mostly just passed through after a brief permissions check.
	{
		CmdHandling::HandleN2N(Client, Stream);
	}
	else if (Flags & Conation::IDENT_ISREPORT_BIT) //This is just a status report.
	{
		CmdHandling::HandleReport(Client, Stream);
	}
	else			//This is an actual request for something.
	{
		CmdHandling::HandleRequest(Client, Stream);
	}
	
	return true;
}

bool Clients::ClientObj::SendPing(void)
{
	if (this->Ping.Waiting) return false; //Don't let us double up.
	
	Conation::ConationStream *ToSend = new Conation::ConationStream(CMDCODE_ANY_PING, false, 0u);

#ifdef DEBUG
	puts(VLString("Clients::ClientObj::SendPing(): Sending ping to client \"") + (this == CurrentAdmin ? "ADMIN" : this->GetID()) + "\".");
#endif
	const bool RetVal = this->SendStream(ToSend);
	//We don't delete ToSend, it's going into the network write scheduler.
	
	this->Ping.SentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	this->Ping.Waiting = true;
	
	return RetVal;
}

bool Clients::ClientObj::CompletePing(void)
{
	if (!this->Ping.Waiting) return false; //We don't even have a ping waiting for us...
	
	this->Ping.Waiting = false;

	this->Ping.RecvTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	
#ifdef DEBUG
	puts(VLString("Clients::ClientObj::CompletePing(): Ping response received for client \"") + (this == CurrentAdmin ? "ADMIN" : this->GetID()) + "\".");
#endif
	this->Ping.PingDiffMillisecs = (this->Ping.RecvTime - this->Ping.SentTime);

	if (this->Ping.PingDiffMillisecs - SERVER_CORE_FREQUENCY_MS >= 0)
	{ //Compensate for our internal core timer.
		this->Ping.PingDiffMillisecs -= SERVER_CORE_FREQUENCY_MS;
	}
	
	return false;
}

Clients::ClientObj::PingSubStruct::PingSubStruct(void) : Waiting(), PingDiffMillisecs()
{ //We do this so we don't send our first ping too early.
	this->SentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	this->RecvTime = this->SentTime;
}
	
void Clients::CheckPingsAndQueues(void)
{

LoopStart:
	for (auto &Pair : ClientMap)
	{
		ClientObj *const Client = +Pair.second;
		
		uint64_t ReadQueueSize = 0, WriteQueueSize = 0;
		NetScheduler::SchedulerStatusObj::OperationType ReadQueueState{}, WriteQueueState{};
		
		Client->ReadQueueStatus->GetValues(nullptr, nullptr, &ReadQueueSize, &ReadQueueState);
		Client->WriteQueueStatus->GetValues(nullptr, nullptr, &WriteQueueSize, &WriteQueueState);
		
		if (!Client->Ping.Waiting)
		{ //See if it's time to send another ping.
			if ((Client->Ping.SentTime / 1000) + PING_INTERVAL_TIME_SECS <= time(nullptr))
			{
				/*Make sure we aren't in the middle of transmitting some enormous blob that will take forever
				 * and make the client ping out when they're really just busy.*/
				
				if (ReadQueueSize == 0 && ReadQueueState == NetScheduler::SchedulerStatusObj::OPERATION_IDLE &&
					WriteQueueSize == 0 && WriteQueueState == NetScheduler::SchedulerStatusObj::OPERATION_IDLE)
				{
					Client->SendPing();
				}
			}
			continue;
		}
		else if ((Client->Ping.SentTime / 1000) + PING_PINGOUT_TIME_SECS <= time(nullptr) &&
				Client->ReadQueueStatus->GetSecsSinceActivity() >= PING_PINGOUT_TIME_SECS &&
				Client->WriteQueueStatus->GetSecsSinceActivity() >= PING_PINGOUT_TIME_SECS)
		{
			Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_PINGOUT);
			
			goto LoopStart;
			
			continue;
		}
	}
}

bool Clients::ProcessNodeDisconnect(const char *ID, const NodeDeauthType Type)
{
	ClientObj *Client = LookupClient(ID);

	if (!Client) return false;

	return ProcessNodeDisconnect(Client, Type);
}

bool Clients::ProcessNodeDisconnect(ClientObj *Client, const NodeDeauthType Type)
{ //ENSURE THIS FUNCTION WILL ACCEPT NODES THAT ARE UNKNOWN TO US!
	if (!Client) return false;

#ifdef DEBUG
	puts("Entered ProcessNodeDisconnect()");
#endif

	VLString LogEntry(1024);
	LogEntry += Client == CurrentAdmin ? VLString("Admin") : (VLString("Node ") + Client->GetID());
	LogEntry += VLString(" (") + Client->GetIPAddr() + ") has disconnected. Reason: " + NodeDeauthTypeText[Type];
	LogEntry.ShrinkToFit();
	
	Logger::WriteLogLine(Logger::LOGITEM_CONN, LogEntry);
	
	if (Client == CurrentAdmin)
	{
		CurrentAdmin = nullptr;
	}
	else
	{
		CmdHandling::NotifyAdmin_NodeChange(Client->GetID(), false);
	}

	//Give them a chance to finish sending data before we close their socket.

	const Net::ClientDescriptor ToDieDesc = Client->GetDescriptor();
	
	/*We're doing all this to prevent a needless memory leak when it kills the read queue thread
	*just because the primary loop has the queue head locked and the thread can't check if it needs to die.*/
	if (Core::CheckInsideHandleInterface())
	{
		if (Core::CheckIsActiveClient(Client))
		{
			Core::SignalSkipHeadRelease();
		}
		else
		{
			Core::SignalResetLoopAfter();
		}
		
		Client->ClientReadQueue->Head_Release(true);
	}
	
	DeleteClient(Client);
	
	Net::Close(ToDieDesc);

#ifdef DEBUG
	puts("Exiting ProcessNodeDisconnect()");
#endif
	return true;
}


