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

#ifndef _VL_SERVER_NETSCHEDULER_H_
#define _VL_SERVER_NETSCHEDULER_H_

#include "common.h"
#include "conation.h"
#include "netcore.h"
#include "vlthreads.h"

#include <list>

namespace NetScheduler
{
	class SchedulerStatusObj
	{ //Pretty much nothing of this can be const because we have to deal with the mutex.
	public:
		enum OperationType : uint8_t { OPERATION_IDLE = 0, OPERATION_SEND, OPERATION_RECV }; //Keep IDLE as zero!
	private:
		mutable VLThreads::Mutex Mutex;
		uint64_t Total;
		uint64_t Transferred;
		uint64_t NumOnQueue;
		time_t LastActivity;
		OperationType CurrentOperation;
		
		SchedulerStatusObj(const SchedulerStatusObj &) = delete;
		SchedulerStatusObj &operator=(const SchedulerStatusObj &) = delete;
	public:
		SchedulerStatusObj(const uint64_t InTotal = 0);
		
		struct CallbackStruct
		{
			SchedulerStatusObj *ThisPointer;
			uint64_t NumOnQueue;
		};
		
		void SetValues(const uint64_t Total, const uint64_t Transferred, const uint64_t NumOnQueue, const OperationType CurrentOperation);
		void SetNumOnQueue(const uint64_t NumOnQueue);
		//Callbacks passed to Net::Read and Net::Write by the queues
		static void NetRecvStatusFunc(const int64_t Transferred, const int64_t Total, CallbackStruct *Sub);
		static void NetSendStatusFunc(const int64_t Transferred, const int64_t Total, CallbackStruct *Sub);

		void GetValues(uint64_t *TotalOut, uint64_t *TransferredOut, uint64_t *NumOnQueueOut, OperationType *CurrentOperationOut);
		uint64_t GetSecsSinceActivity(void) const;
		void RegisterActivity(void);
		
		OperationType GetCurrentOperation(void);
	};
		
		
	//WriteQueue and ReadQueue are multithreaded ways to send and receive network data from many clients at once.
	
	/**LISTEN CAREFULLY:
	 * For both WriteQueue and ReadQueue, they unlock the mutex once they have begun reading/sending data.
	 * They only want the mutex again when they have to delete something out of themselves.
	 * They do this because the API exposed to users does NOT allow deleting data, only adding it,
	 * and std::list doesn't invalidate iterators etc when you only add things.
	 * All client functions hold onto the mutex tight, as they need to.
	 */

	class QueueBase
	{
	protected:
		//Data members.
		VLThreads::Mutex Mutex;
		VLThreads::Semaphore Semaphore;
		VLThreads::Thread Thread;
		std::list<Conation::ConationStream*> Queue;
		Net::ClientDescriptor Descriptor;
		bool Error;
		bool ThreadShouldDie;
		SchedulerStatusObj *StatusObj;
	
	private:	
		//Private member functions.
	
		QueueBase(const QueueBase&);
		QueueBase(QueueBase&&);
		QueueBase &operator=(const QueueBase&);
	public:
		QueueBase(VLThreads::Thread::EntryFunc EntryFuncParam, const Net::ClientDescriptor &RecvDescriptor = {});
		virtual ~QueueBase(void);
		
		virtual void Begin(const Net::ClientDescriptor &NewDescriptor = {});

		virtual bool StopThread(const size_t WaitInMS = 0, const size_t PreCheckWait = 0);
		virtual bool KillThread(void);
		
		virtual bool HasError(void);
		virtual bool ClearError(void);
		virtual void SetStatusObj(SchedulerStatusObj *StatusObj);
	};
	
	class WriteQueue : public QueueBase
	{
	private:		
		//Private member functions
		static void *ThreadFunc(WriteQueue *ThisPointer);
		
		WriteQueue(const WriteQueue&);
		WriteQueue(WriteQueue&&);
		WriteQueue &operator=(const WriteQueue&);
	public:
		WriteQueue(const Net::ClientDescriptor &SendDescriptor = {});
		virtual ~WriteQueue(void) = default;
		
		void Push(Conation::ConationStream *Stream);
	};
	
	class ReadQueue : public QueueBase
	{
	private:
		
		//Private member functions.
		static void *ThreadFunc(ReadQueue *ThisPointer);
		
		ReadQueue(const ReadQueue&);
		ReadQueue(ReadQueue&&);
		ReadQueue &operator=(const ReadQueue&);
	public:
		ReadQueue(const Net::ClientDescriptor &RecvDescriptor = {});
		virtual ~ReadQueue(void) = default;
		
		Conation::ConationStream *Head_Acquire(void);
		void Head_Release(const bool Pop);
		
	};
}

#endif //_VL_SERVER_NETSCHEDULER_H_
