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


#ifndef _VL_SERVER_NODEUPDATES_H_
#define _VL_SERVER_NODEUPDATES_H_
#include "../libvolition/include/common.h"
#include "clients.h"

namespace NodeUpdates
{
	
	namespace Errors
	{
		struct CorruptedBinary
		{
			VLString BinarysPlatformString;
			VLString BinarysRevision;
			VLString BinarysServerAddr;
			
			CorruptedBinary(const char *PlatformString, const char *Revision, const char *ServerAddr)
							: BinarysPlatformString(PlatformString), BinarysRevision(Revision), BinarysServerAddr(ServerAddr) {}
		};
	}
			
	void HandleUpdatesForNode(Clients::ClientObj *Client);
	bool GetNodeBinaryBrandInfo(const void *Buf, const size_t BufSize, VLString *PlatformStringOut, VLString *RevisionOut, VLString *ServerOut);
}
#endif //_VL_SERVER_NODEUPDATES_H_
