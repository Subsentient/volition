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


#ifndef __VL_NETCORE_H__
#define __VL_NETCORE_H__

#define NET_MAX_CHUNK_SIZE 2048

#define PING_INTERVAL_TIME_SECS 60
#define PING_PINGOUT_TIME_SECS (PING_INTERVAL_TIME_SECS / 4)


#include <stddef.h>
#include <vector>
#include <time.h>
#include "common.h"

namespace Net
{
	struct ServerDescriptor
	{
		int Descriptor;
		int Family;
		operator bool(void) const { return Descriptor != 0; }
	};
	
	struct ClientDescriptor
	{
		void *Internal;
		
		ClientDescriptor(void *const Input = nullptr) : Internal(Input) {}
	};
	
	namespace Errors
	{
		class Any
		{
		};
		class InitError : public Any
		{
		};
		class IOError : public Any
		{
		};
		class BlockingError : public Any
		{
		public:
			const size_t BytesSentBeforeBlocking;
			BlockingError(size_t BytesSent) : BytesSentBeforeBlocking(BytesSent) {}
		};
	}
	
	typedef void (*NetRWStatusForEachFunc)(const int64_t TotalTransferred, const int64_t MaxData, void *PassAlongWith); //Signed is not a typo
	
	bool AcceptClient(const ServerDescriptor &ServerDesc, ClientDescriptor *const OutDescriptor, char *const OutIPAddr, const size_t IPAddrMaxLen);
	ServerDescriptor InitServer(unsigned short PortNum);
	bool Connect(const char *InHost, unsigned short PortNum, ClientDescriptor *OutDescriptor);
	bool Write(const ClientDescriptor &Descriptor, const void *const Bytes, const uint64_t ToTransfer, NetRWStatusForEachFunc StatusFunc = nullptr, void *PassAlongWith = nullptr);
	bool Read(const ClientDescriptor &Descriptor, void *const OutStream_, const uint64_t MaxLength, NetRWStatusForEachFunc StatusFunc = nullptr, void *PassAlongWith = nullptr);
	bool Close(const ClientDescriptor &Descriptor);
	bool Close(const int Descriptor);
	bool HasRealDataToRead(const ClientDescriptor &Descriptor);
	void InitNetcore(const bool Server);
	void LoadRootCert(const VLString &Certificate);
	int ToRawDescriptor(const ClientDescriptor &Desc);
	VLString GetRootCert(void);
	
	class PingTracker
	{
	private:
		time_t LastPing;
	public:
		PingTracker(void) : LastPing(time(nullptr)) {}
		
		inline void RegisterPing(void)
		{
			this->LastPing = time(nullptr);
		}
		
		inline bool CheckPingout(void)
		{ //Has the server pinged us since it last should have?
			if (!this->LastPing) return true;
			
			const time_t CurTime = time(nullptr);
			
			return CurTime - this->LastPing <= (PING_INTERVAL_TIME_SECS + PING_PINGOUT_TIME_SECS);
		}
	
	};
	
		
}

#endif //__VL_NETCORE_H__
