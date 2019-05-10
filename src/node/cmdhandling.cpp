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


#include "../libvolition/include/netcore.h"
#include "../libvolition/include/utils.h"
#include "../libvolition/include/conation.h"

#include "interface.h"
#include "cmdhandling.h"
#include "jobs.h"
#include "identity_module.h"
#include "updates.h"
#include "main.h"

#define ADMIN_STR "ADMIN"

void CmdHandling::HandleRequest(Conation::ConationStream *Stream)
{

#ifdef DEBUG
	puts(VLString("Entered CmdHandling::HandleRequest() with stream command code ") + CommandCodeToString(Stream->GetCommandCode()) + " and flags " + Utils::ToBinaryString(Stream->GetCmdIdentFlags()));
#endif
	const Conation::ConationStream::StreamHeader &Hdr = Stream->GetHeader();

	vlassert(Hdr.CmdIdentFlags == Stream->GetCmdIdentFlags());
	
	
	switch (Hdr.CmdCode)
	{
		case CMDCODE_ANY_PING:
			Main::PingTrack.RegisterPing();
			//Fall through
		case CMDCODE_ANY_ECHO:
		{
			Conation::ConationStream::StreamHeader NewHeader = Hdr;
			NewHeader.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
			
			Conation::ConationStream *Response = new Conation::ConationStream(NewHeader, Stream->GetArgData());

			Main::PushStreamToWriteQueue(Response);
			
			break;
		}
		case CMDCODE_A2C_MOD_GENRESP:
			/*This command code is only read by scripts/modules running in another thread,
			* It has already been forwarded to all such scripts by Interface::HandleServerInterface()*/
			break;
		case CMDCODE_ANY_NOOP:
			break;
		case CMDCODE_A2C_SLEEP:
		{ //Server asks us to terminate.
			Conation::ConationStream Response(Hdr.CmdCode, true, Hdr.CmdIdent);
#ifdef DEBUG
			puts("CmdHandling::HandleRequest(): Executing sleep command");
#endif
			Response.Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
			Response.Push_NetCmdStatus(true);

			Main::PushStreamToWriteQueue(Response);
			
			///Fall through
		}
		case CMDCODE_ANY_DISCONNECT:
		{
			Main::ForceReleaseMutexes();
			
			exit(0);

#ifdef DEBUG
			puts("CmdHandling::HandleRequest(): Unexpected failure, did not expect to reach here");
#endif
			break;
		}
		case CMDCODE_B2C_USEUPDATE:
		{ //Server's ordering us to self-update.
			if (!Stream->VerifyArgTypes(Conation::ARGTYPE_FILE, Conation::ARGTYPE_NONE))
			{ //corrupted, I guess just ignore it.
				break;
			}
			
			Conation::ConationStream::FileArg File = Stream->Pop_File();

			//THIS SHOULD NEVER RETURN!!!
			const NetCmdStatus WhatFailed = Updates::AttemptUpdate(File);

			Conation::ConationStream FailResponse(CMDCODE_B2C_USEUPDATE, true, 0u);

			FailResponse.Push_NetCmdStatus(WhatFailed);

			Main::InitNetQueues(); //Restart queues after failed update.
			
			Main::PushStreamToWriteQueue(FailResponse);
			break;
		}
			
		///Command from the administrator.
		case CMDCODE_A2C_CHDIR:
		case CMDCODE_A2C_GETCWD:
		case CMDCODE_A2C_EXECCMD:
		case CMDCODE_A2C_FILES_COPY:
		case CMDCODE_A2C_FILES_MOVE:
		case CMDCODE_A2C_FILES_DEL:
		case CMDCODE_A2C_FILES_PLACE:
		case CMDCODE_A2C_FILES_FETCH:
		case CMDCODE_A2C_GETPROCESSES:
		case CMDCODE_A2C_KILLPROCESS:
		case CMDCODE_A2C_MOD_EXECFUNC:
		case CMDCODE_A2C_MOD_LOADSCRIPT:
		case CMDCODE_A2C_MOD_UNLOADSCRIPT:
		case CMDCODE_A2C_HOSTREPORT:
		case CMDCODE_A2C_LISTDIRECTORY:
		{
#ifdef DEBUG
			puts(VLString("CmdHandling::HandleRequest(): Spawning ") + ((Stream->GetCmdIdentFlags() & Conation::IDENT_ISROUTINE_BIT) ? "routine" : "admin ordered") + " job " + CommandCodeToString(Hdr.CmdCode));
#endif
			Jobs::StartJob(Hdr.CmdCode, Stream);
			break;
		}
		case CMDCODE_B2C_GETJOBSLIST:
		{
			Conation::ConationStream::StreamHeader NewHdr = Hdr;
			NewHdr.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
			NewHdr.StreamArgsSize = 0;
			
			Conation::ConationStream *JobsListStream = Jobs::BuildRunningJobsReport(NewHdr);
			
			Main::PushStreamToWriteQueue(JobsListStream);
			break;
		}
		case CMDCODE_B2C_KILLJOBBYCMDCODE:
		{
			Conation::ConationStream::StreamHeader NewHdr = Hdr;
			NewHdr.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
			NewHdr.StreamArgsSize = 0;
			
			Stream->Pop_ODHeader(); //Useless
			
			Conation::ConationStream Response(NewHdr, nullptr);
			
			Response.Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
			
			const NetCmdStatus Result = Jobs::KillJobByCmdCode(static_cast<CommandCode>(Stream->Pop_Uint32()));
			
			Response.Push_NetCmdStatus(Result);
			
			Main::PushStreamToWriteQueue(Response);
			break;
		}
		case CMDCODE_B2C_KILLJOBID:
		{
			Conation::ConationStream::StreamHeader NewHdr = Hdr;
			NewHdr.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
			NewHdr.StreamArgsSize = 0;
			
			Stream->Pop_ODHeader(); //Useless

			Conation::ConationStream Response(NewHdr, nullptr);
			
			Response.Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);

			const NetCmdStatus Result = Jobs::KillJobByID(Stream->Pop_Uint64());
			
			Response.Push_NetCmdStatus(Result);
			
			Main::PushStreamToWriteQueue(Response);
			break;
		}
		default:
		{ //Something we don't understand.
			Conation::ConationStream Response(Hdr.CmdCode, true, Hdr.CmdIdent);

			Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED
#ifdef DEBUG
										, "Command sent from wrong direction by server."
#endif
										));

			Main::PushStreamToWriteQueue(Response);
			break;
		}
	}

}

void CmdHandling::HandleReport(Conation::ConationStream *Stream)
{
	
}
