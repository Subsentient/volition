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



#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <signal.h>

#ifdef MACOSX ///Apparently Mac OS X has very, very incomplete and broken support for pthread semaphores....
extern "C"
{
#include <dispatch/dispatch.h>
}
#else
#include <semaphore.h>
#endif //MACOSX

#endif //WIN32
#include <stdio.h>

#include "include/vlthreads.h"
VLThreads::Mutex::Mutex(void)
{
#ifdef WIN32
	this->Resource = CreateMutex(nullptr, false, nullptr);
#else
	this->Resource = new pthread_mutex_t(PTHREAD_MUTEX_INITIALIZER);
#endif
}

void VLThreads::Mutex::Lock(void)
{
#ifdef WIN32
	WaitForSingleObject((HANDLE)this->Resource, INFINITE);
#else
	pthread_mutex_lock((pthread_mutex_t*)this->Resource);
#endif
}

void VLThreads::Mutex::Unlock(void)
{
#ifdef WIN32
	ReleaseMutex((HANDLE)this->Resource);
#else
	pthread_mutex_unlock((pthread_mutex_t*)this->Resource);
#endif
}


VLThreads::Mutex::~Mutex(void)
{
#ifdef WIN32
	CloseHandle((HANDLE)this->Resource);
#else
	delete (pthread_mutex_t*)this->Resource;
#endif
}

VLThreads::Semaphore::Semaphore(const size_t InitialCount) :
#ifdef WIN32
	Resource(CreateSemaphore(nullptr, InitialCount, INT_MAX, nullptr))
#elif defined(MACOSX)
	Resource(new dispatch_semaphore_t(dispatch_semaphore_create(InitialCount)) )
#else
	Resource(new sem_t())
#endif
{

#if !defined(WIN32) && !defined(MACOSX)
	sem_init((sem_t*)this->Resource, 0, InitialCount);
#endif
}

void VLThreads::Semaphore::Wait(void)
{
#ifdef WIN32
	WaitForSingleObject((HANDLE)this->Resource, INFINITE);
#elif defined(MACOSX)
	dispatch_semaphore_wait(*(dispatch_semaphore_t*)this->Resource, DISPATCH_TIME_FOREVER);
#else
	while (sem_wait((sem_t*)this->Resource) != 0); //Leave this in a loop. Signals can botch it.
#endif
}

void VLThreads::Semaphore::Post(void)
{
#ifdef WIN32
	ReleaseSemaphore((HANDLE)this->Resource, 1, nullptr);
#elif defined(MACOSX)
	dispatch_semaphore_signal(*(dispatch_semaphore_t*)this->Resource);
#else
	sem_post((sem_t*)this->Resource);
#endif
}

VLThreads::Semaphore::~Semaphore(void)
{
#ifdef WIN32
	CloseHandle((HANDLE)this->Resource);
#elif defined(MACOSX)
	dispatch_release(*(dispatch_semaphore_t*)this->Resource);
	delete (dispatch_semaphore_t*)this->Resource;
#else
	sem_destroy((sem_t*)this->Resource);
	delete (sem_t*)this->Resource;
#endif
}

VLThreads::Thread::Thread(Thread::EntryFunc EntryPointIn, void *DataPointerIn) : ThreadObj(), EntryPoint(EntryPointIn), DataPointer(DataPointerIn), Dead()
{
}

void VLThreads::Thread::Start(void)
{
#ifdef WIN32
	this->ThreadObj = CreateThread(nullptr, 0u, (LPTHREAD_START_ROUTINE)this->EntryPoint, this->DataPointer, 0, nullptr);
#else
	this->ThreadObj = new pthread_t;
	pthread_create((pthread_t*)this->ThreadObj, nullptr, this->EntryPoint, this->DataPointer);
#endif
}

VLThreads::Thread::~Thread(void)
{
	if (!this->ThreadObj) return;
	
#ifdef WIN32
	CloseHandle((HANDLE)this->ThreadObj);
#else
	delete (pthread_t*)this->ThreadObj;
#endif
}

void VLThreads::Thread::Join(void)
{
	if (!this->ThreadObj) return;
#ifdef WIN32
	WaitForSingleObject((HANDLE)this->ThreadObj, INFINITE);
#else
	pthread_join(*(pthread_t*)this->ThreadObj, nullptr);
#endif
}

bool VLThreads::Thread::Alive(void)
{
	if (this->Dead || !this->ThreadObj) return false; //Once we're dead once, we're dead forever.
#ifdef WIN32
	DWORD RetVal{};

	if (!GetExitCodeThread((HANDLE)this->ThreadObj, &RetVal) || RetVal != STILL_ACTIVE)
#else
	if (pthread_kill(*(pthread_t*)this->ThreadObj, 0) != 0)
#endif
	{
		this->Dead = true;
		return false;
	}

	return true;
}

bool VLThreads::Thread::Started(void) const
{
	return this->ThreadObj != nullptr;
}

bool VLThreads::Thread::Kill(void)
{
	if (!this->ThreadObj) return false; //Hasn't been started yet, fucktard.
	
	if (this->Dead) return false; //Once we're dead once, we're dead forever.
#ifdef WIN32
	return TerminateThread((HANDLE)this->ThreadObj, 0);
#else
	return pthread_cancel(*(pthread_t*)this->ThreadObj) == 0;
#endif

}
