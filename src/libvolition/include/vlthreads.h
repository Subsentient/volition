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

#ifndef _VL_VLTHREADS_H_
#define _VL_VLTHREADS_H_

#include "common.h"

namespace VLThreads
{
	class Mutex
	{
	private:
		void *Resource;
		Mutex(const Mutex&);
		Mutex(Mutex&&);
		Mutex &operator=(const Mutex&);
	public:
		Mutex(void);
		~Mutex(void);
		void Lock(void);
		void Unlock(void);
	};

	class MutexKeeper
	{ //Locks the mutex you feed it and unlocks the mutex when the MutexKeeper falls out of scope.
	private:
		Mutex *Target;
		bool Locked;
		MutexKeeper(const MutexKeeper&);
		MutexKeeper &operator=(const MutexKeeper&);
	public:

		MutexKeeper(MutexKeeper &&Ref) : Target(Ref.Target), Locked(Ref.Locked)
		{
			Ref.Target = nullptr;
		}

		MutexKeeper &operator=(MutexKeeper &&Ref)
		{
			this->Target = Ref.Target;
			this->Locked = Ref.Locked;
		
			Ref.Target = nullptr;
			Ref.Locked = false;
			return *this;
		}
		
		MutexKeeper(Mutex *const InTarget) : Target(InTarget), Locked()
		{
			if (!this->Target) return;
			
			this->Target->Lock();
		}

		~MutexKeeper(void)
		{
			this->Target->Unlock();
		}

		void Forget(void)
		{
			this->Target = nullptr;
			this->Locked = false;
		}

		void Unlock(void)
		{
			if (!this->Target || !this->Locked) return;
			
			this->Target->Unlock();

			this->Locked = false;
		}
		
		void Lock(void)
		{
			if (!this->Target || this->Locked) return;
			
			this->Target->Lock();
			
			this->Locked = true;
		}
		
		void Encase(Mutex *const NewTarget)
		{
			this->Unlock();
			
			this->Target = NewTarget;
			
			this->Lock();
		}

		bool IsLocked(void) const { return this->Locked; }
		
		Mutex *GetTarget(void) const
		{
			return this->Target;
		}
	};	

	class Semaphore
	{
	private:
		void *Resource;
		Semaphore(const Semaphore&);
		Semaphore(Semaphore&&);
		Semaphore &operator=(const Semaphore&);
	public:
		Semaphore(const size_t InitialCount = 0);
		~Semaphore(void);
		void Wait(void);
		void Post(void);
	};
	
	class Thread
	{
	public:
		typedef void *(*EntryFunc)(void*);
	private:
		void *ThreadObj;
		EntryFunc EntryPoint;
		void *DataPointer;
		bool Dead;
		
		Thread(const Thread&);
		Thread &operator=(const Thread&);
	public:
		Thread(EntryFunc EntryPointIn, void *DataPointerIn);
		~Thread(void);
		void Join(void);
		bool Started(void) const;
		void Start(void);
		bool Alive(void);
		bool Kill(void);
	};
	
	template <typename T>
	class ValueWaiter
	{
	private:
		Mutex Lock;
		T Value;
		Semaphore Waiter;
	public:
		inline T Await(void)
		{
			this->Waiter.Wait();
			
			MutexKeeper G { &this->Lock };
			return this->Value;
		}
		
		inline void Post(const T &ValueIn)
		{
			MutexKeeper G { &this->Lock };
			this->Value = ValueIn;
			
			this->Waiter.Post();
		}
	};
}
#endif //_VL_VLTHREADS_H_
