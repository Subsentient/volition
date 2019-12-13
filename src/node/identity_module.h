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


#ifndef _VL_NODE_IDENTITY_MODULE_H_
#define _VL_NODE_IDENTITY_MODULE_H_
#include "../libvolition/include/common.h"

namespace IdentityModule
{
	void SetNodeIdentity(const char *NewIdentity);
	void SetServerAddr(const char *NewIdentity);
	VLString GetNodeIdentity(void);
	VLString GetNodeRevision(void);
	VLString GetNodePlatformString(void);
	VLString GetNodeAuthToken(void);
	VLString GetServerAddr(void);
	VLString GetCertificate(void);
	VLString GetStartupScript(void);
	int64_t GetCompileTime(void);
}

#endif //_VL_NODE_IDENTITY_MODULE_H_
