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



#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "../libvolition/include/common.h"
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/netscheduler.h"

#include <queue>
#include <vector>
#include <list>
#include <chrono>

#include <time.h>
#ifndef PING_PINGOUT_TIME_SECS
#define PING_PINGOUT_TIME_SECS 15 //Keep this relatively small.
#endif //PING_PINGOUT_TIME_SECS

#ifndef PING_INTERVAL_TIME_SECS
#define PING_INTERVAL_TIME_SECS 60
#endif //PING_INTERVAL_TIME_SECS

namespace Clients
{
	
	enum NodeDeauthType
	{
		NODE_DEAUTH_INVALID,
		NODE_DEAUTH_CONNBREAK, //Broken connection.
		NODE_DEAUTH_PINGOUT, //Pinged out
		NODE_DEAUTH_AUTO, //Node or admin disconnected on its own
		NODE_DEAUTH_ORDER, //Node or admin was following an order to shut down.
		NODE_DEAUTH_EVIL, //Malicious client.
		NODE_DEAUTH_BADAUTHTOKEN, //Bad authentication token, either revoked or connecting with an invalid one from start.
		NODE_DEAUTH_MAX };

	class ClientObj
	{
	private:
		int Descriptor;
		time_t ConnectedTime; //The time authentication was completed.
		VLString ID;
		VLString IPAddr;
		VLString PlatformString; //OS/CPU it's running on.
		VLString NodeRevision; //Version string basically
		VLString Group; //Whatever group this node belongs to, must be empty if none.
		VLString AuthToken; //The token which gave this node permission to connect at all.
		
		struct PingSubStruct
		{
			bool Waiting;
			int64_t RecvTime;
			int64_t SentTime;
			int64_t PingDiffMillisecs;
			
			PingSubStruct(void);
		} Ping;

		NetScheduler::ReadQueue *ClientReadQueue;
		NetScheduler::WriteQueue *ClientWriteQueue;
		NetScheduler::SchedulerStatusObj *ReadQueueStatus;
		NetScheduler::SchedulerStatusObj *WriteQueueStatus;
		
		ClientObj &operator=(const ClientObj &);
	public:
		inline NetScheduler::ReadQueue *GetReadQueue(void) { return this->ClientReadQueue; }
		inline NetScheduler::WriteQueue *GetWriteQueue(void) { return this->ClientWriteQueue; }
		inline int GetDescriptor(void) const { return this->Descriptor; }
		inline VLString GetPlatformString(void) const { return PlatformString; }
		inline VLString GetNodeRevision(void) const { return NodeRevision; }
		inline time_t GetConnectedTime(void) const { return ConnectedTime; }
		inline VLString GetGroup(void) const { return this->Group; }
		inline int64_t GetPingLatency(void) const { return Ping.PingDiffMillisecs; }
		inline VLString GetIPAddr(void) const { return IPAddr; }
		inline VLString GetID(void) const { return ID; }
		inline VLString GetAuthToken(void) const { return this->AuthToken; }
		inline Conation::ConationStream *RecvStream_Acquire(void) { if (!this->ClientReadQueue) return nullptr; else return ClientReadQueue->Head_Acquire(); }
		inline void RecvStream_Release(const bool Pop = true) { if (this->ClientReadQueue) return ClientReadQueue->Head_Release(Pop); }
		inline bool HasNetworkError(void) { return this->ClientReadQueue->HasError() || this->ClientWriteQueue->HasError(); }
		inline void SetGroup(const char *NewGroup) { this->Group = NewGroup; }
		inline void SetRevision(const char *NewRevision) { this->NodeRevision = NewRevision; }
		//The following 2 functions modify the mutable object this->WriteQueue.
		bool SendStream(Conation::ConationStream *Stream); //EXPECTS A NEWLY ALLOCATED STREAM IT CAN DELETE!!!
		bool SendStream(Conation::ConationStream &Stream); //Just copies the stream to a new one on the heap and calls the function above.
		
		//Ping control
		bool SendPing(void);
		bool CompletePing(void);
		
		//Constructors
		ClientObj(const int InDesc = 0) : Descriptor(InDesc), PlatformString("NA"),
				NodeRevision("NA"), Group(), Ping(),
				ClientReadQueue(new NetScheduler::ReadQueue(InDesc)), ClientWriteQueue(new NetScheduler::WriteQueue(InDesc)),
				ReadQueueStatus(new NetScheduler::SchedulerStatusObj), WriteQueueStatus(new NetScheduler::SchedulerStatusObj) {}
		
		//Destructors
		inline ~ClientObj(void)
		{
			//Wait up to 500 milliseconds each for the read and write queue threads to shut down, and wait 50 milliseconds before checking.
			this->ClientReadQueue->StopThread(500, 50);
			this->ClientWriteQueue->StopThread(500, 50);
			
			delete this->ClientReadQueue;
			delete this->ClientWriteQueue;
			
			delete this->ReadQueueStatus;
			delete this->WriteQueueStatus;
		}
		
		//Friends
		friend ClientObj *AcceptClient_Auth(const Net::ServerDescriptor &ServerDesc);
		friend void CheckPingsAndQueues(void);
		friend bool ProcessNodeDisconnect(ClientObj *Client, const NodeDeauthType Type);
	};
	
	struct Err_DuplicateClientDescriptor
	{
		ClientObj *Dupe;
		Err_DuplicateClientDescriptor(ClientObj *const InDupe) : Dupe(InDupe) {}
	};

	ClientObj *AddClient(const int Descriptor);
	bool DeleteClient(const char *ID);
	bool DeleteClient(const ClientObj *const Ptr);
	
	ClientObj *LookupClient(const VLString &Identity);
	const std::list<ClientObj> &GetList(void);
	bool HandleClientInterface(Clients::ClientObj *Client, Conation::ConationStream *Stream);
	void FlushAll(void);
	ClientObj *AcceptClient_Auth(const Net::ServerDescriptor &ServerDesc);
	void CheckPingsAndQueues(void);
	ClientObj *LookupCurAdmin(void);

	bool ProcessNodeDisconnect(const char *ID, const NodeDeauthType Type);
	bool ProcessNodeDisconnect(const int Descriptor, const NodeDeauthType Type);
	bool ProcessNodeDisconnect(ClientObj *Client, const NodeDeauthType Type);

}
#endif //__CLIENT_H__
