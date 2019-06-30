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
		this->Mutex.Lock();
		this->ThreadShouldDie = true;
		this->Semaphore.Post(); //Because if it's waiting on new data we want it to check and see.
		this->Mutex.Unlock();
		
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
	this->Mutex.Lock();
	
	const bool Value = !this->Error;
	
	this->Error = false;
	
	this->Mutex.Unlock();
	
	return Value;
}

bool NetScheduler::QueueBase::HasError(void)
{
	this->Mutex.Lock();
	
	const bool Value = this->Error;
	
	this->Mutex.Unlock();
	
	return Value;
}

void NetScheduler::QueueBase::SetStatusObj(SchedulerStatusObj *const StatusObj)
{
	this->Mutex.Lock();
	this->StatusObj = StatusObj;
	this->Mutex.Unlock();
}

NetScheduler::WriteQueue::WriteQueue(const Net::ClientDescriptor &DescriptorIn) : QueueBase((VLThreads::Thread::EntryFunc)WriteQueue::ThreadFunc, DescriptorIn)
{
}

void NetScheduler::WriteQueue::Push(Conation::ConationStream *Stream)
{
#ifdef DEBUG
	puts(VLString("NetScheduler::WriteQueue::Push(): Accepted stream with command code ") + CommandCodeToString(Stream->GetCommandCode()) + " and flags " + Utils::ToBinaryString(Stream->GetCmdIdentFlags()));
#endif
	this->Mutex.Lock();
	
	this->Queue.push_back(Stream);
	
	if (this->StatusObj) this->StatusObj->SetNumOnQueue(this->Queue.size());

	this->Semaphore.Post(); //Has its own internal mutex.
	
	this->Mutex.Unlock();
}


void *NetScheduler::WriteQueue::ThreadFunc(WriteQueue *ThisPointer)
{
	while (1)
	{
		ThisPointer->Mutex.Lock();
		
		if (ThisPointer->ThreadShouldDie)
		{ //This thread is done for.
			ThisPointer->Mutex.Unlock();
			return nullptr;
		}
		
		//Nothing to do
		if (ThisPointer->Queue.empty())
		{
			ThisPointer->Mutex.Unlock();
			ThisPointer->Semaphore.Wait(); //Wait for some new data to show up.
			continue; //If the semaphore has a higher than one value, we just keep looping until we're done.
		}
		
		Conation::ConationStream *Head = ThisPointer->Queue.front();

		SchedulerStatusObj::CallbackStruct CBS = { ThisPointer->StatusObj, ThisPointer->Queue.size() };


		/** We have the head, now UNLOCK the mutex since nobody else is able to delete what we're reading right now anyways,
		* and if we don't, nobody can add to the queue while we're sending data which could take who knows how long. */
		ThisPointer->Mutex.Unlock();
		
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
		ThisPointer->Mutex.Lock();
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
		
		if (ThisPointer->StatusObj) ThisPointer->StatusObj->SetValues(0u, 0u, ThisPointer->Queue.size(), SchedulerStatusObj::OPERATION_IDLE);

		ThisPointer->Mutex.Unlock(); //Done again
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
		ThisPointer->Mutex.Lock();
		
		//We're being asked to die.
		if (ThisPointer->ThreadShouldDie)
		{
#ifdef DEBUG
			puts("NetScheduler::ReadQueue::ThreadFunc(): Thread was told to die. Terminating.");
#endif //DEBUG
			ThisPointer->Mutex.Unlock();
#ifdef DEBUG
			puts("NetScheduler::ReadQueue::ThreadFunc(): Thread mutex unlocked, thread can now die.");
#endif //DEBUG
			return nullptr; //Terminate our own thread.
		}
		
		SchedulerStatusObj::CallbackStruct CBS = { ThisPointer->StatusObj, ThisPointer->Queue.size() };

		ThisPointer->Mutex.Unlock(); //We don't need access right now.

		fd_set Set{};
		const int IntDesc = Net::ToRawDescriptor(ThisPointer->Descriptor);
			
		FD_SET(IntDesc, &Set);
		fd_set ErrSet = Set;
		struct timeval Time{};
		Time.tv_usec = 30;
		

		//Something to do?
		
		const int SelectStatus = select(IntDesc + 1, &Set, nullptr, &ErrSet, &Time);
		
		if (!SelectStatus || (SelectStatus < 0 && errno == EINTR))
		{ //Guess not.
			continue;
		}
		
		if ((SelectStatus < 0 && errno != EINTR) || FD_ISSET(IntDesc, &ErrSet) || !Net::HasRealDataToRead(ThisPointer->Descriptor))
		{
#ifdef DEBUG
			if (SelectStatus < 0) puts(VLString("NetScheduler::ReadQueue::ThreadFunc(): Error detected was ") + (const char*)strerror(errno));
#endif
			ThisPointer->Error = true;
			Utils::vl_sleep(10);
			continue;
		}
		
		///Guess we need to receive some data after all!
		
		Conation::ConationStream *Stream = nullptr;

#ifdef DEBUG
		puts("NetScheduler::ReadQueue::ThreadFunc(): Attempting to download stream.");
#endif

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
		
		ThisPointer->Mutex.Lock();

		if (Stream)
		{
#ifdef DEBUG
			puts(VLString("NetScheduler::ReadQueue::ThreadFunc(): Success downloading stream, command code is ") + CommandCodeToString(Stream->GetCommandCode()) + " with flags " + Utils::ToBinaryString(Stream->GetCmdIdentFlags() ));
#endif
			ThisPointer->Queue.push_back(Stream);
		}
		else
		{
		ReadError:
#ifdef DEBUG
			puts("NetScheduler::ReadQueue::ThreadFunc(): Error downloading stream.");
#endif
			//We're gonna change stuff now so this is needed again.

			ThisPointer->Error = true;
			
		}
		
		if (ThisPointer->StatusObj) ThisPointer->StatusObj->SetValues(0u, 0u, ThisPointer->Queue.size(), SchedulerStatusObj::OPERATION_IDLE);

		ThisPointer->Mutex.Unlock();		
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
	: Total(), Transferred(), NumOnQueue(), CurrentOperation()
{
}

void NetScheduler::SchedulerStatusObj::GetValues(uint64_t *TotalOut,
												uint64_t *TransferredOut,
												uint64_t *NumOnQueueOut,
												OperationType *CurrentOperationOut)
{
	this->Mutex.Lock();
	if (TotalOut) *TotalOut = this->Total;
	if (TransferredOut) *TransferredOut = this->Transferred;
	if (NumOnQueueOut) *NumOnQueueOut = this->NumOnQueue;
	if (CurrentOperationOut) *CurrentOperationOut = this->CurrentOperation;
	this->Mutex.Unlock();
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
	this->Mutex.Lock();
	this->Total = Total;
	this->Transferred = Transferred;
	this->CurrentOperation = CurrentOperation;
	this->NumOnQueue = NumOnQueue;
	this->Mutex.Unlock();
}
void NetScheduler::SchedulerStatusObj::SetNumOnQueue(const uint64_t NumOnQueue)
{
	this->Mutex.Lock();
	this->NumOnQueue = NumOnQueue;
	this->Mutex.Unlock();
}

void NetScheduler::SchedulerStatusObj::NetRecvStatusFunc(const int64_t Transferred,
														const int64_t Total,
														CallbackStruct *Sub)
{
	if (Transferred == -1)
	{
		Sub->ThisPointer->SetValues(0u, 0u, Sub->NumOnQueue, OPERATION_IDLE); //Completed.
	}
	else
	{
		Sub->ThisPointer->SetValues(Total, Transferred, Sub->NumOnQueue, OPERATION_RECV);
	}
}

void NetScheduler::SchedulerStatusObj::NetSendStatusFunc(const int64_t Transferred,
														const int64_t Total,
														CallbackStruct *Sub)
{
	if (Transferred == -1)
	{
		Sub->ThisPointer->SetValues(0u, 0u, Sub->NumOnQueue, OPERATION_IDLE); //Completed.
	}
	else
	{
		Sub->ThisPointer->SetValues(Total, Transferred, Sub->NumOnQueue, OPERATION_SEND);
	}
}
