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
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/utils.h"
#include "../libvolition/include/conation.h"

#include "interface.h"
#include "cmdhandling.h"
#include "identity_module.h"
#include "main.h"
#include "jobs.h"

#include <stdio.h>

Net::ClientDescriptor Interface::Establish(const char *Hostname)
{ //We know what server we want, now we communicate with it.
	//Hack to fix ping registration on disconnect
	Main::PingTrack.RegisterPing();
	
	Net::ClientDescriptor Connection{};
	
	VLDEBUG("Connecting to server " + Hostname + "...");

	if (!Net::Connect(Hostname, MASTER_PORT, &Connection))
	{
		return 0;
	}
	VLDEBUG("Network connection open. Prepare to authenticate.");
	
	Conation::ConationStream LoginStream(CMDCODE_B2S_AUTH, 0, 0u);

	LoginStream.Push_Int32(Conation::PROTOCOL_VERSION);
	LoginStream.Push_String(IdentityModule::GetNodeIdentity());
	LoginStream.Push_String(IdentityModule::GetNodeAuthToken());
	LoginStream.Push_String(IdentityModule::GetNodePlatformString());
	LoginStream.Push_String(IdentityModule::GetNodeRevision());
	
	//Transmit the login request.
	if (!LoginStream.Transmit(Connection))
	{
		Net::Close(Connection);
		return 0;
	}
	
	VLDEBUG("Authentication transmit succeeded. Downloading response.");
	
	VLScopedPtr<Conation::ConationStream*> ResponseStream;

	//Get the response.
	try
	{
		ResponseStream = new Conation::ConationStream(Connection);
	}
	catch (const Conation::ConationStream::Err_StreamDownloadFailure &)
	{
		Net::Close(Connection);
		return 0;
	}

	VLDEBUG("Server response downloaded.");
	
	//Check if the server likes us.
	if (!ResponseStream->VerifyArgTypes(Conation::ARGTYPE_NETCMDSTATUS, Conation::ARGTYPE_NONE))
	{
		VLDEBUG("Argument is not ARGTYPE_NETCMDSTATUS or argument missing. Aborting.");
		Net::Close(Connection);
		return 0;
	}
	
	if (ResponseStream->Pop_NetCmdStatus().Status != STATUS_OK)
	{
		VLDEBUG("Server reports our authentication failed.");
		Net::Close(Connection);
		return 0;
	}
	
	VLDEBUG("Authentication succeeded.");

	return Connection;
}
bool Interface::HandleServerInterface(Conation::ConationStream *Stream)
{
	uint8_t Flags = 0;
	
	Stream->GetCommandIdent(&Flags, nullptr);

	VLASSERT(Flags == Stream->GetCmdIdentFlags());

	VLDEBUG("Processing stream with command code " + CommandCodeToString(Stream->GetCommandCode()) + " and flags " + Utils::ToBinaryString(Flags));

	//Give the scripts whatever they want.
	Jobs::ForwardToScriptJobs(Stream);
	
	if (Flags & Conation::IDENT_ISREPORT_BIT)
	{
		CmdHandling::HandleReport(Stream);
	}
	else
	{
		CmdHandling::HandleRequest(Stream);
	}

	return true;
}
