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


#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"
#include <string.h>

#include "identity_module.h"

//Delimiter lengths
static constexpr size_t IdentityDelimLength = NODE_IDENTITY_DELIMSTART_LENGTH;

static constexpr size_t RevisionDelimLength = NODE_REVISION_DELIMSTART_LENGTH;

static constexpr size_t PlatformStringDelimLength = NODE_PLATFORM_STRING_DELIMSTART_LENGTH;

static constexpr size_t ServerAddrDelimLength = NODE_SERVERADDR_DELIMSTART_LENGTH;

static constexpr size_t CompileTimeDelimLength = NODE_COMPILETIME_DELIMSTART_LENGTH;

static constexpr size_t AuthTokenDelimLength = NODE_AUTHTOKEN_DELIMSTART_LENGTH;

static constexpr size_t CertDelimLength = NODE_CERT_DELIMSTART_LENGTH;


//Buffers holding our data.

/**This is literally the only time I have ever had a use for the volatile keyword,
* and here it's just to keep the compiler from stripping these symbols if they
* don't look useful. The data can't actually change at runtime, but we don't want
* the compiler to know that.
**/
static volatile char Identity[NODE_IDENTITY_CAPACITY] = NODE_IDENTITY_DELIMSTART;

static volatile char Revision[NODE_REVISION_CAPACITY] = NODE_REVISION_DELIMSTART;

static volatile char PlatformString[NODE_PLATFORM_STRING_CAPACITY] = NODE_PLATFORM_STRING_DELIMSTART;

static volatile char ServerAddr[NODE_SERVERADDR_CAPACITY] = NODE_SERVERADDR_DELIMSTART;

static volatile char AuthToken[NODE_AUTHTOKEN_CAPACITY] = NODE_AUTHTOKEN_DELIMSTART;

static volatile char CertBuffer[NODE_CERT_CAPACITY] = NODE_CERT_DELIMSTART;

static volatile uint8_t CompileTimeBuf[NODE_COMPILETIME_CAPACITY] = NODE_COMPILETIME_DELIMSTART;


VLString IdentityModule::GetNodeIdentity(void)
{
	VLString Value(NODE_IDENTITY_CAPACITY);

	Value = (const char*)Identity + IdentityDelimLength + 1;

	return Value;
}

void IdentityModule::SetNodeIdentity(const char *NewIdentity)
{
	volatile char *Begin = Identity + IdentityDelimLength + 1;
	
	strncpy((char*)Begin, NewIdentity, sizeof Identity - IdentityDelimLength - 2);
	Begin[sizeof Identity - IdentityDelimLength - 2] = '\0';
}

VLString IdentityModule::GetNodeRevision(void)
{
	VLString Value(NODE_REVISION_CAPACITY);

	Value = (const char*)Revision + RevisionDelimLength + 1;

	return Value;
}

VLString IdentityModule::GetNodePlatformString(void)
{
	VLString Value(NODE_PLATFORM_STRING_CAPACITY);

	Value = (const char*)PlatformString + PlatformStringDelimLength + 1;

	return Value;
}
VLString IdentityModule::GetNodeAuthToken(void)
{
	VLString Value(NODE_AUTHTOKEN_CAPACITY);

	Value = (const char*)AuthToken + AuthTokenDelimLength + 1;

	return Value;
}

VLString IdentityModule::GetServerAddr(void)
{
	VLString Value(NODE_SERVERADDR_CAPACITY);

	Value = (const char*)ServerAddr + ServerAddrDelimLength + 1;

	return Value;
}

VLString IdentityModule::GetCertificate(void)
{
	VLString Value(NODE_CERT_CAPACITY);

	Value = (const char*)CertBuffer + CertDelimLength + 1;

	return Value;
}
	
void IdentityModule::SetServerAddr(const char *NewServerAddr)
{
	char *Begin = (char*)ServerAddr + ServerAddrDelimLength + 1;
	
	strncpy(Begin, NewServerAddr, sizeof ServerAddr - ServerAddrDelimLength - 2);
	Begin[sizeof ServerAddr - ServerAddrDelimLength - 2] = '\0';

}

int64_t IdentityModule::GetCompileTime(void)
{
	int64_t RetVal = 0;
	
	memcpy(&RetVal, (uint8_t*)CompileTimeBuf + CompileTimeDelimLength + 1, sizeof(int64_t));
	*(uint64_t*)&RetVal = Utils::vl_ntohll(*(uint64_t*)&RetVal);
		
	return RetVal;
}
