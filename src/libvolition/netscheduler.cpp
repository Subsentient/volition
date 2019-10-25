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

#include "include/common.h"
#include "include/conation.h"
#include "include/vlthreads.h"
#include "include/utils.h"
#include "include/netcore.h"

#include "include/netscheduler.h"

//Needed for select().
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif //WIN32

#include <errno.h>

NetScheduler::QueueBase::QueueBase(VLThreads::Thread::EntryFunc EntryFuncParam, const Net::ClientDescriptor &DescriptorIn)
	: Thread(EntryFuncParam, this),
	Descriptor(DescriptorIn),
	Error(),
	ThreadShouldDie(),
	StatusObj()
{
}


void NetScheduler::QueueBase::Begin(const Net::ClientDescriptor &NewDescriptor)
{
	if (NewDescriptor.Internal) this->Descriptor = NewDescriptor;
	
	this->ThreadShouldDie = false;
	this->Thread.Start();
}


NetScheduler::QueueBase::~QueueBase(void)
{
	this->StopThread();

	//Delete the queue.
	for (auto Iter = this->Queue.begin(); Iter != this->Queue.end(); ++Iter)
	{
		delete *Iter;
	}
	
	this->Queue.clear();
}

bool NetScheduler::QueueBase::StopThread(const size_t WaitInMS, const size_t PreCheckWait)
{
	if (this->Thread.Started() && this->Thread.Alive())
	{
		VLThreads::MutexKeeper Keeper { &this->Mutex };

		this->ThreadShouldDie = true;
		this->Semaphore.Post(); //Because if it's waiting on new data we want it to check and see.
		Keeper.Unlock();
		
		if (!WaitInMS)
		{ //Wait forever.
			while (this->Thread.Alive());
			
			this->Thread.Join();
		}
		else
		{			
			if (PreCheckWait) Utils::vl_sleep(PreCheckWait);
			
			if (this->Thread.Alive())
			{
				Utils::vl_sleep(WaitInMS);
				this->Thread.Kill();
				this->Thread.Join();
			}
		}
		return true;
	}
	else return false;
}

bool NetScheduler::QueueBase::KillThread(void)
{
	if (this->Thread.Started() && this->Thread.Alive())
	{
		this->Thread.Kill();
		this->Thread.Join();
		return true;
	}
	else return false;
}

bool NetScheduler::QueueBase::ClearError(void)
{
	VLThreads::MutexKeeper Keeper { &this->Mutex };
	
	const bool Value = !this->Error;
	
	this->Error = false;
	
	return Value;
}

bool NetScheduler::QueueBase::HasError(void)
{
	VLThreads::MutexKeeper Keeper { &this->Mutex };
	
	const bool Value = this->Error;
	
	return Value;
}

void NetScheduler::QueueBase::SetStatusObj(SchedulerStatusObj *const StatusObj)
{
	VLThreads::MutexKeeper Keeper { &this->Mutex };
	this->StatusObj = StatusObj;
}

NetScheduler::WriteQueue::WriteQueue(const Net::ClientDescriptor &DescriptorIn) : QueueBase((VLThreads::Thread::EntryFunc)WriteQueue::ThreadFunc, DescriptorIn)
{
}

void NetScheduler::WriteQueue::Push(Conation::ConationStream *Stream)
{
	VLDEBUG("Accepted stream with command code " + CommandCodeToString(Stream->GetCommandCode()) + " and flags " + Utils::ToBinaryString(Stream->GetCmdIdentFlags()));
	VLThreads::MutexKeeper Keeper { &this->Mutex };
	
	this->Queue.push_back(Stream);
	
	Keeper.Unlock();
	
	if (this->StatusObj) this->StatusObj->SetNumOnQueue(this->Queue.size());

	this->Semaphore.Post(); //Has its own internal mutex.
}


void *NetScheduler::WriteQueue::ThreadFunc(WriteQueue *ThisPointer)
{
	while (1)
	{
		VLThreads::MutexKeeper Keeper { &ThisPointer->Mutex };
		
		if (ThisPointer->ThreadShouldDie)
		{ //This thread is done for.
			return nullptr;
		}
		
		//Nothing to do
		if (ThisPointer->Queue.empty())
		{
			Keeper.Unlock();
			ThisPointer->Semaphore.Wait(); //Wait for some new data to show up.
			continue; //If the semaphore has a higher than one value, we just keep looping until we're done.
		}
		
		Conation::ConationStream *Head = ThisPointer->Queue.front();

		SchedulerStatusObj::CallbackStruct CBS = { ThisPointer->StatusObj, ThisPointer->Queue.size() };


		/** We have the head, now UNLOCK the mutex since nobody else is able to delete what we're reading right now anyways,
		* and if we don't, nobody can add to the queue while we're sending data which could take who knows how long. */
		Keeper.Unlock();
		
		bool Result = false; //It's important this be initialized to false in case Net::Write throws an exception.
		
		try
		{
			Result = Net::Write(ThisPointer->Descriptor, Head->GetData().data(), Head->GetData().size(),
								ThisPointer->StatusObj ? (Net::NetRWStatusForEachFunc)SchedulerStatusObj::NetSendStatusFunc : nullptr,
								ThisPointer->StatusObj ? &CBS : nullptr);
		}
		catch (...)
		{
			goto WriteFailure;
		}
		
		
		///Potential need to modify, lock mutex.
		Keeper.Lock();
		if (Result)
		{
			delete Head;
			ThisPointer->Queue.pop_front();
		}
		else
		{
		WriteFailure:
			ThisPointer->Error = true;
		}
		
		Keeper.Unlock(); //SetValues uses this mutex
		
		if (ThisPointer->StatusObj) ThisPointer->StatusObj->SetValues(0u, 0u, ThisPointer->Queue.size(), SchedulerStatusObj::OPERATION_IDLE);
	}
	return nullptr;
}

NetScheduler::ReadQueue::ReadQueue(const Net::ClientDescriptor &DescriptorIn) : QueueBase((VLThreads::Thread::EntryFunc)ReadQueue::ThreadFunc, DescriptorIn)
{
}


void *NetScheduler::ReadQueue::ThreadFunc(ReadQueue *ThisPointer)
{
	while (1)
	{
		VLThreads::MutexKeeper Keeper { &ThisPointer->Mutex };

		//We're being asked to die.
		if (ThisPointer->ThreadShouldDie)
		{
			VLDEBUG("Thread was told to die. Terminating.");

			Keeper.Unlock();
			
			VLDEBUG("Thread mutex unlocked, thread can now die.");
			return nullptr; //Terminate our own thread.
		}
		
		SchedulerStatusObj::CallbackStruct CBS = { ThisPointer->StatusObj, ThisPointer->Queue.size() };

		Keeper.Unlock(); //We don't need access right now.

		fd_set Set{};
		const int IntDesc = Net::ToRawDescriptor(ThisPointer->Descriptor);
			
		FD_SET(IntDesc, &Set);
		fd_set ErrSet = Set;
		struct timeval Time{};
		Time.tv_usec = 10000;
		

		//Something to do?
		
		const int SelectStatus = select(IntDesc + 1, &Set, nullptr, &ErrSet, &Time);
		
		if (!SelectStatus || (SelectStatus < 0 && errno == EINTR))
		{ //Guess not.
			continue;
		}
		
		if ((SelectStatus < 0 && errno != EINTR) || FD_ISSET(IntDesc, &ErrSet) || !Net::HasRealDataToRead(ThisPointer->Descriptor))
		{
			if (SelectStatus < 0) VLDEBUG("Error detected was " + (const char*)strerror(errno));

			ThisPointer->Error = true;
			Utils::vl_sleep(10);
			continue;
		}
		
		///Guess we need to receive some data after all!
		
		Conation::ConationStream *Stream = nullptr;

		VLDEBUG("Attempting to download stream.");

		try
		{
			
			Stream = new Conation::ConationStream(ThisPointer->Descriptor,
												ThisPointer->StatusObj ? (Net::NetRWStatusForEachFunc)SchedulerStatusObj::NetRecvStatusFunc : nullptr,
												ThisPointer->StatusObj ? &CBS : nullptr);
		}
		catch (...)
		{
			goto ReadError;
		}
		
		Keeper.Lock();

		if (Stream)
		{
			VLDEBUG("Success downloading stream, command code is " + CommandCodeToString(Stream->GetCommandCode()) + " with flags " + Utils::ToBinaryString(Stream->GetCmdIdentFlags() ));
			ThisPointer->Queue.push_back(Stream);
		}
		else
		{
		ReadError:
			VLDEBUG("Error downloading stream.");
			//We're gonna change stuff now so this is needed again.

			ThisPointer->Error = true;
			
		}
		
		Keeper.Unlock();
		
		if (ThisPointer->StatusObj) ThisPointer->StatusObj->SetValues(0u, 0u, ThisPointer->Queue.size(), SchedulerStatusObj::OPERATION_IDLE);

	}
	return nullptr;
}

Conation::ConationStream *NetScheduler::ReadQueue::Head_Acquire(void)
{
	this->Mutex.Lock();
	
	if (this->Queue.empty()) return nullptr;
	
	return this->Queue.front();
}

void NetScheduler::ReadQueue::Head_Release(const bool Pop)
{
	if (Pop && this->Queue.size())
	{
		delete this->Queue.front();
		this->Queue.pop_front();
		if (this->StatusObj) this->StatusObj->SetNumOnQueue(this->Queue.size());
	}
	
	this->Mutex.Unlock();
}

NetScheduler::SchedulerStatusObj::SchedulerStatusObj(const uint64_t InTotal)
	: Total(), Transferred(), NumOnQueue(), LastActivity(time(nullptr)), CurrentOperation()
{
}

void NetScheduler::SchedulerStatusObj::GetValues(uint64_t *TotalOut,
												uint64_t *TransferredOut,
												uint64_t *NumOnQueueOut,
												OperationType *CurrentOperationOut)
{
	VLThreads::MutexKeeper Keeper { &this->Mutex };

	if (TotalOut) *TotalOut = this->Total;
	if (TransferredOut) *TransferredOut = this->Transferred;
	if (NumOnQueueOut) *NumOnQueueOut = this->NumOnQueue;
	if (CurrentOperationOut) *CurrentOperationOut = this->CurrentOperation;
}

NetScheduler::SchedulerStatusObj::OperationType NetScheduler::SchedulerStatusObj::GetCurrentOperation(void)
{
	const VLThreads::MutexKeeper Keeper { &this->Mutex };
	
	return this->CurrentOperation;
}

void NetScheduler::SchedulerStatusObj::SetValues(const uint64_t Total,
												const uint64_t Transferred,
												const uint64_t NumOnQueue,
												const OperationType CurrentOperation)
{
	VLThreads::MutexKeeper Keeper { &this->Mutex };

	this->Total = Total;
	this->Transferred = Transferred;
	this->CurrentOperation = CurrentOperation;
	this->NumOnQueue = NumOnQueue;
}
void NetScheduler::SchedulerStatusObj::SetNumOnQueue(const uint64_t NumOnQueue)
{
	VLThreads::MutexKeeper Keeper { &this->Mutex };

	this->NumOnQueue = NumOnQueue;
}

void NetScheduler::SchedulerStatusObj::NetRecvStatusFunc(const int64_t Transferred,
														const int64_t Total,
														CallbackStruct *Sub)
{
	if (Transferred == -1)
	{
		Sub->ThisPointer->SetValues(0u, 0u, Sub->NumOnQueue, OPERATION_IDLE); //Completed.
		return;
	}
	
	Sub->ThisPointer->RegisterActivity();
	Sub->ThisPointer->SetValues(Total, Transferred, Sub->NumOnQueue, OPERATION_RECV);
}

void NetScheduler::SchedulerStatusObj::NetSendStatusFunc(const int64_t Transferred,
														const int64_t Total,
														CallbackStruct *Sub)
{
	if (Transferred == -1)
	{
		Sub->ThisPointer->SetValues(0u, 0u, Sub->NumOnQueue, OPERATION_IDLE); //Completed.
		return;
	}

	Sub->ThisPointer->RegisterActivity();
	Sub->ThisPointer->SetValues(Total, Transferred, Sub->NumOnQueue, OPERATION_SEND);
}

uint64_t NetScheduler::SchedulerStatusObj::GetSecsSinceActivity(void) const
{
	const VLThreads::MutexKeeper Keeper { &this->Mutex };
	
	return time(nullptr) - this->LastActivity;
}

void NetScheduler::SchedulerStatusObj::RegisterActivity(void)
{
	const VLThreads::MutexKeeper Keeper { &this->Mutex };
	
	this->LastActivity = time(nullptr);
}
