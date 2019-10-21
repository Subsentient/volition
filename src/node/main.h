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


#ifndef _VL_NODE_MAIN_H_
#define _VL_NODE_MAIN_H_

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/netscheduler.h"

#include <queue>
#include <map>

namespace Main
{
	extern Net::PingTracker PingTrack;
	
	char **GetArgvData(int **ArgcOut);
	const Net::ClientDescriptor *GetSocketDescriptor(void);
	void Begin(const bool JustUpdated = false);
	
	void PushStreamToWriteQueue(Conation::ConationStream *Stream);
	void PushStreamToWriteQueue(const Conation::ConationStream &Stream);
	
	void ForceReleaseMutexes(void);
	
	void MurderAllThreads(void);
	
	void InitNetQueues(void);
	
	NetScheduler::ReadQueue &GetReadQueue(void);
	NetScheduler::WriteQueue &GetWriteQueue(void);

	class DLOpenQueue
	{ //Some platforms act shitty if you dlopen() from another thread
	public:
		struct QueueMember
		{
			VLString LibPath;
			VLString FuncName;
			VLThreads::ValueWaiter<void*> *Waiter;
		};
	private:
		VLThreads::Mutex RequestsLock, FuncsLock, LibsLock;
		std::queue<QueueMember> Requests;
		std::map<VLString, void*> Functions;
		std::map<VLString, void*> Libs;
		
	public:
		void *GetFunction(const VLString &LibPath, const VLString &FuncName);

		
		void ProcessRequests(void);
	};
	
	extern DLOpenQueue DLQ;
}

#endif //_VL_NODE_MAIN_H_
