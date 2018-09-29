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
		MutexKeeper(const MutexKeeper&);
		MutexKeeper &operator=(const MutexKeeper&);
	public:

		MutexKeeper(MutexKeeper &&Ref) : Target(Ref.Target)
		{
			Ref.Target = nullptr;
		}

		MutexKeeper &operator=(MutexKeeper &&Ref)
		{
			this->Target = Ref.Target;
			Ref.Target = nullptr;

			return *this;
		}
		
		MutexKeeper(Mutex *const InTarget) : Target(InTarget)
		{
			if (!this->Target) return;
			
			this->Target->Lock();
		}

		~MutexKeeper(void)
		{
			if (!this->Target) return;
			
			this->Target->Unlock();
		}

		void Forget(void)
		{
			this->Target = nullptr;
		}

		void ReleaseNow(void)
		{
			if (!this->Target) return;
			
			this->Target->Unlock();

			this->Target = nullptr;
		}
		
		void Encase(Mutex *const NewTarget)
		{
			this->ReleaseNow();
			this->Target = NewTarget;
		}

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
}
#endif //_VL_VLTHREADS_H_
