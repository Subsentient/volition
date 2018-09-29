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
#include <stddef.h>
#include <vector>
#include "common.h"
namespace Net
{
	struct ServerDescriptor
	{
		int Descriptor;
		int Family;
		operator bool(void) const { return Descriptor != 0; }
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
	
	bool AcceptClient(const ServerDescriptor &ServerDesc, int *const OutDescriptor, char *const OutIPAddr, const size_t IPAddrMaxLen);
	ServerDescriptor InitServer(unsigned short PortNum);
	bool Connect(const char *InHost, unsigned short PortNum, int *OutDescriptor);
	bool Write(const int SockDescriptor, const void *const Bytes, const uint64_t ToTransfer, NetRWStatusForEachFunc StatusFunc = nullptr, void *PassAlongWith = nullptr);
	bool Read(const int SockDescriptor, void *const OutStream_, const uint64_t MaxLength, NetRWStatusForEachFunc StatusFunc = nullptr, void *PassAlongWith = nullptr);
	bool Close(const int SockDescriptor);
	bool HasRealDataToRead(const int SockDescriptor);
	void InitNetcore(const bool Server);
	void LoadRootCert(const VLString &Certificate);
}

#endif //__VL_NETCORE_H__
