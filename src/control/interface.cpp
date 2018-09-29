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
#include "../libvolition/include/conation.h"
#include "../libvolition/include/netcore.h"

#include "interface.h"
#include "config.h"
#include "cmdhandling.h"
#include "main.h"

bool Interface::Establish(const VLString &Username, const VLString &Password, const VLString &Hostname, int *OutDescriptor)
{
	int Descriptor = 0;
	if (!Net::Connect(Hostname, MASTER_PORT, &Descriptor))
	{
		return false;
	}

	Conation::ConationStream LoginRequest(CMDCODE_B2S_AUTH, false, 0u);

	/*Server just wants the login.
	 * It determines we're an admin trying to connect by
	 * looking at our argument types and counts.
	 */
	LoginRequest.Push_Int32(Conation::PROTOCOL_VERSION);
	LoginRequest.Push_String(Username);
	LoginRequest.Push_String(Password);

	if (!LoginRequest.Transmit(Descriptor))
	{
		return false;
	}

	Conation::ConationStream *Response = nullptr;

	try
	{
		Response = new Conation::ConationStream(Descriptor);
	}
	catch (...)
	{
		return false;
	}

	if (!Response->VerifyArgTypes(Conation::ARGTYPE_NETCMDSTATUS, Conation::ARGTYPE_NONE))
	{
		delete Response;
		return false;
	}

	NetCmdStatus Code = Response->Pop_NetCmdStatus();

	delete Response;

	*OutDescriptor = Descriptor;
	return Code;
}


bool Interface::RequestIndex(void)
{
	Conation::ConationStream *Request = new Conation::ConationStream(CMDCODE_A2S_INDEXDL, false, 0u);

	Main::GetWriteQueue().Push(Request);
	
	return true;
}


bool Interface::HandleServerInterface(Conation::ConationStream *Stream)
{
	if (!Stream) goto Done;

	uint8_t Flags;
	Stream->GetCommandIdent(&Flags, nullptr);

	if (Flags & Conation::IDENT_ISREPORT_BIT)
	{
		CmdHandling::HandleReport(Stream);
	}
	else
	{
		CmdHandling::HandleRequest(Stream);
	}

Done:
	return true;
}
