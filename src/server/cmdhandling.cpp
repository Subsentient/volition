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


#include <stdio.h>
#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/brander.h"
#include "../libvolition/include/utils.h"

#include "cmdhandling.h"
#include "db.h"
#include "core.h"
#include "logger.h"
#include "nodeupdates.h"

#include <map>
#include <list>
#include <time.h>

//Globals

//Prototypes for static functions
static Conation::ConationStream *HandleNodeInfoRequest(Clients::ClientObj *Client, Conation::ConationStream *Stream,
														const char *RequestedID);

//Function definitions
void CmdHandling::HandleReport(Clients::ClientObj *Client, Conation::ConationStream *Stream)
{
#ifdef DEBUG
	puts("Entered CmdHandling::HandleReport()");
#endif

	uint8_t Flags = 0;
	uint64_t Ident = 0u;
	
	Stream->GetCommandIdent(&Flags, &Ident);

	const bool IsCmdReport = Flags & Conation::IDENT_ISREPORT_BIT;
	
	const bool IsAdmin = Client == Clients::LookupCurAdmin();

	if (!IsCmdReport)
	{
		Logger::WriteLogLine(Logger::LOGITEM_SYSERROR, "Internal error: Stream passed to CmdHandling::HandleReport is not a report.");
		return;
	}
	
	///Do stuff now
	switch (Stream->GetCommandCode())
	{
		case CMDCODE_ANY_NOOP: //Instant discard.
		case CMDCODE_ANY_ECHO: //We don't do anything on the receive side.
			break;
		case CMDCODE_ANY_PING:
		{
			Client->CompletePing();
			if (Client != Clients::LookupCurAdmin())
			{
				NotifyAdmin_NodeChange(Client->GetID(), true);
			}
			break;
		}
		case CMDCODE_B2C_USEUPDATE:
		{
			if (IsAdmin)
			{ //Wat
#ifdef DEBUG
				puts("CmdHandling::HandleReport(): Update report came from admin! Killing malicious admin client.");
#endif //DEBUG
				ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			VLScopedPtr<std::vector<Conation::ArgType>*> ArgTypes = Stream->GetArgTypes();
			
			//Sanity check
			if (!ArgTypes || ArgTypes->at(0) != Conation::ARGTYPE_NETCMDSTATUS)
			{
#ifdef DEBUG
				puts(VLString("CmdHandling::HandleReport(): Update report provides no argument types or first argument is not NetCmdStatus"));
#endif //DEBUG
				break;
			}
			
			NetCmdStatus Result = Stream->Pop_NetCmdStatus();
			
			if (!Result)
			{
#ifdef DEBUG
				puts(VLString("CmdHandling::HandleReport(): Report specifies failure with node update."));
#endif //DEBUG
				break;
			}
			
			if (ArgTypes->size() != 2 || ArgTypes->at(1) != Conation::ARGTYPE_STRING)
			{ //We were expecting a string afterwards. O.o
#ifdef DEBUG
				puts(VLString("CmdHandling::HandleReport(): Update report malformed, has no string argument after success"));
#endif //DEBUG
				break;
			}
			
			const VLString &NewRevision = Stream->Pop_String();
			
			DB::UpdateNodeDB(Client->GetID(), nullptr, NewRevision, nullptr, 0);
			
			Client->SetRevision(NewRevision);
			
			if (!(Flags & Conation::IDENT_ISROUTINE_BIT))
			{ //I have no idea why this would be useful.
				NotifyAdmin_NodeChange(Client->GetID(), true);
			}
			
			break;
		}
		default:
		{
			if (Stream->GetCommandCode() >= CMDCODE_MAX)
			{ //Unrecognized command code.
				VLString RejectMsg(4096);
				RejectMsg += VLString("CmdHandling::HandleReport(): Rejecting stream with unknown command code integer ") + VLString::UintToString(Stream->GetCommandCode()) + " and arguments ";
			
				RejectMsg += Logger::ReportArgsToText(Stream);

				Stream->Rewind(); //Cuz ReportArgsToText() increments the index pointer to get arguments.
				
				Logger::WriteLogLine(Logger::LOGITEM_SECUREWARN, RejectMsg);
				
				break;
			}
			
			//Reports from clients back to the admin.
			VLScopedPtr<std::vector<Conation::ArgType>*> Types = Stream->GetArgTypes();
			
			if (!Types || Types->at(0) != Conation::ARGTYPE_ODHEADER)
			{ //This is garbage.
				
				Logger::WriteLogLine(Logger::LOGITEM_SECUREWARN, VLString("CmdHandling::HandleReport(): Stream with command code ") + CommandCodeToString(Stream->GetCommandCode())
																+ " was rejected for lacking an ODHeader.");
				break;
			}

			Conation::ConationStream::ODHeader ODObj = Stream->Pop_ODHeader();

			Stream->Rewind(); //Probably unnecessary but let's cover our asses.
			if (ODObj.Origin != Client->GetID() || ODObj.Destination == ODObj.Origin) break; //Probably malicious, their ID doesn't match what we have on file.
			
			
			if (Flags & Conation::IDENT_ISROUTINE_BIT)
			{ //Routines are processed differently.
				VLString Msg = VLString("Node ") + Client->GetID() + "reports results of routine #" + VLString::UintToString(Stream->GetCmdIdentOnly())
								+ " (" + CommandCodeToString(Stream->GetCommandCode()) + "): " + Logger::ReportArgsToText(Stream);
				
				Stream->Rewind();
				
				Logger::WriteLogLine(Logger::LOGITEM_INFO, Msg);
				break;
			}
			
			//Regular data, goes to admin.
			Clients::ClientObj *Target = Clients::LookupCurAdmin();

			if (!Target) break; //Couldn't find the target.

			Target->SendStream(*Stream); //Forward to the correct client.
			
			break;
		}
	}
	
	
}

void CmdHandling::HandleRequest(Clients::ClientObj *Client, Conation::ConationStream *Stream)
{
	uint8_t Flags = 0u;
	uint64_t Ident = 0u;
	
	Stream->GetCommandIdent(&Flags, &Ident);
	
	const bool IsCmdReport = Flags & Conation::IDENT_ISREPORT_BIT;
	
	if (IsCmdReport)
	{
		fputs("\nInternal error: Stream passed to CmdHandling::HandleRequest is a report, not a request.\n", stderr);
		return;
	}

	const bool IsAdmin = Client == Clients::LookupCurAdmin();

	DB::AuthTokenPermissions NodePermissions{};

	if (!IsAdmin)
	{ ///Node? Get the permissions for it.
		VLScopedPtr<DB::AuthTokensDBEntry*> Entry { DB::LookupAuthToken(Client->GetAuthToken()) };

		if (!Entry)
		{ //Shouldn't happen, but I'd be stupid not to check for it.
			Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_BADAUTHTOKEN);
			return;
		}

		NodePermissions = Entry->Permissions;
	}

	
	switch (Stream->GetCommandCode())
	{
		case CMDCODE_ANY_NOOP:
			return;
		case CMDCODE_ANY_ECHO:
		{
			Conation::ConationStream::StreamHeader Header = Stream->GetHeader();
			Header.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
			
			Conation::ConationStream *EchoBack = new Conation::ConationStream(Header, Stream->GetArgData());
			
			Client->SendStream(EchoBack);

			return;
		}
		case CMDCODE_ANY_DISCONNECT:
		{
			auto Hdr = Stream->GetHeader();
			
			Hdr.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;

			Conation::ConationStream *Response = new Conation::ConationStream(Hdr, nullptr);
			
			Response->Push_NetCmdStatus(NetCmdStatus(true));
			
			Client->SendStream(Response);
			
			Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_AUTO);
			return;
		}
		case CMDCODE_ANY_PING:
		{
			Conation::ConationStream::StreamHeader Hdr = Stream->GetHeader();
			Hdr.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
			
			Conation::ConationStream *Reply = new Conation::ConationStream(Hdr, Stream->GetArgData());
			
			Client->SendStream(Reply);
			return;
		}
		case CMDCODE_A2S_INDEXDL:
		{
			if (!IsAdmin)
			{ //You're evil, asking us for this and you're not the admin...
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				return;
			}

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_NONE}))
			{
				Conation::ConationStream MisusedMsg(CMDCODE_A2S_INDEXDL, Conation::IDENT_ISREPORT_BIT, Ident);
				MisusedMsg.Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));

				Client->SendStream(MisusedMsg);
				return;
			}

			Conation::ConationStream *Out = HandleIndexRequest(Stream);
			assert(Client == Clients::LookupCurAdmin());

			Client->SendStream(Out);
			break;
		}
		case CMDCODE_A2S_INFO:
		{
			if (!IsAdmin)
			{ //Not allowed to ask us this, you're just a node! Now you die.
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				return;
			}
			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Conation::ConationStream MisusedMsg(CMDCODE_A2S_INFO, Conation::IDENT_ISREPORT_BIT, Ident);
				MisusedMsg.Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));

				Client->SendStream(MisusedMsg);
				return;
			}
			
			const VLString &RequestedID = Stream->Pop_String();
			
			VLScopedPtr<Conation::ConationStream*> Result;
			
			if (!(Result = HandleNodeInfoRequest(Client, Stream, RequestedID)))
			{
				Conation::ConationStream NotFoundMsg(CMDCODE_A2S_INFO, Conation::IDENT_ISREPORT_BIT, Ident);
				NotFoundMsg.Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISSING, "Node not found."));

				Client->SendStream(NotFoundMsg);
				return;
			}

			Client->SendStream(Result);
			return;
		}
		case CMDCODE_A2S_CHGNODEGROUP:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_STRING}))
			{
				Conation::ConationStream Response(CMDCODE_A2S_CHGNODEGROUP, Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

				NetCmdStatus ErrorMsg(false, STATUS_MISUSED);
				Response.Push_NetCmdStatus(ErrorMsg);
				
				Client->SendStream(Response);
				break;
			}
			
			Conation::ConationStream::ODHeader ODObj = Stream->Pop_ODHeader();
			
			if (ODObj.Origin != "ADMIN" || ODObj.Destination == "ADMIN")
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			VLString NewGroup = Stream->Pop_String();
			
			if (!DB::UpdateNodeDB(ODObj.Destination, nullptr, nullptr, NewGroup, 0))
			{
				Conation::ConationStream *Response = new Conation::ConationStream(CMDCODE_A2S_CHGNODEGROUP, Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
				
				NetCmdStatus ErrorMsg(false, STATUS_MISSING, "Unable to locate specified node to change group.");
				
				Response->Push_NetCmdStatus(ErrorMsg);
				
				Client->SendStream(Response);
				break;
			}
			
			if (Clients::ClientObj *Lookup = Clients::LookupClient(ODObj.Destination))
			{
				Lookup->SetGroup(NewGroup);
				NotifyAdmin_NodeChange(Lookup->GetID(), true);
			}
			else
			{
				NotifyAdmin_NodeChange(ODObj.Destination, false);
			}
			
			Conation::ConationStream Response(CMDCODE_A2S_CHGNODEGROUP, Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
			
			Response.Push_NetCmdStatus(true);
			
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_PROVIDEUPDATE:
		{ //Admin giving us a new binary for the node.
			if (!IsAdmin)
			{ //WTF? Nodes aren't allowed to send this! Evilllll
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_FILE}))
			{
				Conation::ConationStream Response(CMDCODE_A2S_PROVIDEUPDATE, Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
				
				NetCmdStatus ErrorMsg(false, STATUS_MISUSED);
				Response.Push_NetCmdStatus(ErrorMsg);
				
				Client->SendStream(Response);
				break;
			}
			
			const Conation::ConationStream::FileArg File = Stream->Pop_File();
			
			VLString PlatformString;
			VLString Revision;
			
			if (!NodeUpdates::GetNodeBinaryBrandInfo(File.Data, File.DataSize, &PlatformString, &Revision, nullptr))
			{
				Conation::ConationStream Response(CMDCODE_A2S_PROVIDEUPDATE, Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
				
				Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_FAILED, "Binary lacks required branding information"));
				
				Client->SendStream(Response);
				break;
			}
			
			const bool Result = (File.Data && File.DataSize) ? DB::UpdatePlatformBinaryDB(PlatformString, Revision, File.Data, File.DataSize) : false; 
			
			Conation::ConationStream Response(CMDCODE_A2S_PROVIDEUPDATE, Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
			
			Response.Push_NetCmdStatus(Result);
			
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_B2C_USEUPDATE:
		{ //Admin forcing an update for a node.
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream Response(CMDCODE_B2C_USEUPDATE, Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
			
			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_FILE}))
			{
				
				Response.Push_NetCmdStatus({false, STATUS_MISUSED, "Invalid arguments provided to force update function."});
				
				Client->SendStream(Response);
				break;
			}
			
			const Conation::ConationStream::ODHeader &ODObj = Stream->Pop_ODHeader();
			
			if (ODObj.Origin != "ADMIN")
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream::FileArg File = Stream->Pop_File();
			
			Clients::ClientObj *Lookup = Clients::LookupClient(ODObj.Destination);
			
			
			if (!Lookup)
			{
				Response.Push_NetCmdStatus({false, STATUS_MISSING, VLString("Unable to locate client ") + ODObj.Destination});
				Client->SendStream(Response);
				break;
			}
			
			std::map<Brander::AttributeTypes, Brander::AttrValue> Values;
			Values.emplace(Brander::AttributeTypes::IDENTITY, Lookup->GetID());
			Values.emplace(Brander::AttributeTypes::AUTHTOKEN, Lookup->GetAuthToken());
			
			if (!Brander::BrandBinaryViaBuffer(File.Data, File.DataSize, Values))
			{ //We're altering the file data in the ConationStream itself, and yes, this is safe.
				Response.Push_NetCmdStatus({false, STATUS_IERR, "Failed to brand intercepted binary with target ID!"});
				Client->SendStream(Response);
				break;
			}
			
			Conation::ConationStream *NodeOrder = new Conation::ConationStream(Stream->GetCommandCode(), 0, Stream->GetCmdIdentOnly());
			
			NodeOrder->Push_File(VLString(), File.Data, File.DataSize);
			
			Lookup->SendStream(NodeOrder);
			
			Response.Push_NetCmdStatus({true, STATUS_OK, "Server successfully intercepted, branded, and forwarded new binary to node."});
			Client->SendStream(Response);
			
			break;
		}
		case CMDCODE_B2S_VAULT_ADD:
		{
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING, Conation::ARGTYPE_FILE}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Incorrect arguments"});
				Client->SendStream(Response);
				break;
			}

			const VLString &Key = Stream->Pop_String();

			VLScopedPtr<DB::VaultDBEntry*> Exists { DB::LookupVaultDBEntry(Key, "Key") };

			if (Exists || (!IsAdmin && !(NodePermissions & DB::ATP_VAULT_ADD)))
			{
				Response->Push_NetCmdStatus(false);
				Client->SendStream(Response);
				break;
			}

			const Conation::ConationStream::FileArg File = Stream->Pop_File();

			DB::VaultDBEntry Entry{};
			Entry.Key = Key;
			Entry.OriginNode = IsAdmin ? "ADMIN" : Client->GetID();
			Entry.StoredTime = time(nullptr);

			Entry.Binary.resize(File.DataSize);

			memcpy(&Entry.Binary[0], File.Data, File.DataSize);
			
			const NetCmdStatus Result = DB::UpdateVaultDB(&Entry);

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_B2S_VAULT_FETCH:
		{
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
			
			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, VLString("Incorrect arguments")});
				Client->SendStream(Response);
				break;
			}

			const VLString &ItemName = Stream->Pop_String();
			
			VLScopedPtr<DB::VaultDBEntry*> Entry { DB::LookupVaultDBEntry(ItemName, "OriginNode") };
			
			if (!Entry)
			{
				Response->Push_NetCmdStatus(false); //We don't want anyone to know whether this key exists or not.
				Client->SendStream(Response);
				break;
			}
			
			const bool BelongsToNode = Entry->OriginNode == Client->GetID();
			
			Entry = DB::LookupVaultDBEntry(ItemName); //Now get all of it.
			
			if (IsAdmin || (BelongsToNode && (NodePermissions & DB::ATP_VAULT_READ_OUR)) || (NodePermissions & DB::ATP_VAULT_READ_ANY))
			{
				Response->Push_File(Entry->Key, Entry->Binary.data(), Entry->Binary.size());
			}
			else
			{
				Response->Push_NetCmdStatus(false);
			}
			
			Client->SendStream(Response);
			
			break;
		}
		case CMDCODE_B2S_VAULT_UPDATE:
		{
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
			
			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING, Conation::ARGTYPE_FILE}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, VLString("Incorrect arguments")});
				Client->SendStream(Response);
				break;
			}

			const VLString &Filename = Stream->Pop_String();

			VLScopedPtr<DB::VaultDBEntry*> Find { DB::LookupVaultDBEntry(Filename, "OriginNode") };

			if (!Find)
			{
				Response->Push_NetCmdStatus(false);
				Client->SendStream(Response);
				break;
			}

			const bool BelongsToNode = Find->OriginNode == Client->GetID();
			
			Find.Release();
			
			if (IsAdmin || (BelongsToNode && (NodePermissions & DB::ATP_VAULT_CHG_OUR)) || (NodePermissions & DB::ATP_VAULT_CHG_ANY))
			{
				const Conation::ConationStream::FileArg &File { Stream->Pop_File() };
	
				DB::VaultDBEntry Entry{};
				Entry.Key = Filename;
				Entry.OriginNode = IsAdmin ? "ADMIN" : Client->GetID();
				Entry.StoredTime = time(nullptr);
	
				Entry.Binary.resize(File.DataSize);
	
				memcpy(&Entry.Binary[0], File.Data, File.DataSize);
				
				const NetCmdStatus Result = DB::UpdateVaultDB(&Entry);
	
				Response->Push_NetCmdStatus(Result);
			}
			else
			{
				Response->Push_NetCmdStatus(false);
			}
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_B2S_VAULT_DROP:
		{
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Incorrect arguments"});
				Client->SendStream(Response);
				break;
			}

			const VLString &Key = Stream->Pop_String();
			
			VLScopedPtr<DB::VaultDBEntry*> Entry { DB::LookupVaultDBEntry(Key, "OriginNode") };

			///We're careful not to let them know whether such an item exists or not in case the node has been seized by a hostile force.
			if (!Entry)
			{
				Response->Push_NetCmdStatus({false, STATUS_FAILED});
				Client->SendStream(Response);
				break;
			}

			const bool BelongsToNode = Entry->OriginNode == Client->GetID();

			if (IsAdmin || (BelongsToNode && (NodePermissions & DB::ATP_VAULT_CHG_OUR)) || (NodePermissions & DB::ATP_VAULT_CHG_ANY))
			{
				const bool Result = DB::DeleteVaultDBEntry(Key);
				
				Response->Push_NetCmdStatus(Result);
			}
			else
			{
				Response->Push_NetCmdStatus(false); //We're not telling them access denied, cuz that would confirm the existence of that key.
			}
			
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_VAULT_LIST:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
			
			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_NONE}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Takes no arguments"});
				Client->SendStream(Response);
				break;
			}
			
			VLScopedPtr<std::vector<DB::VaultDBEntry>*> VaultItems { DB::GetVaultItemsInfo() };
			
			VLString ReportBuf(VaultItems->size() * 256);
			
			ReportBuf = VLString::UintToString(VaultItems->size()) + " items in vault.\n----\n";
			
			for (size_t Inc = 0u; Inc < VaultItems->size(); ++Inc)
			{
				const DB::VaultDBEntry &Item = VaultItems->at(Inc);
				
				VLString TimeBuf(512);
				struct tm TimeStruct = *localtime(&Item.StoredTime);
				
				strftime(TimeBuf.GetBuffer(), TimeBuf.GetCapacity(), "%Y-%m-%d %I:%M:%S %p", &TimeStruct);
				
				ReportBuf += VLString("\"") + Item.Key + "\", created by " + Item.OriginNode + " on " + TimeBuf + '\n';
			}
			
			ReportBuf.ShrinkToFit();
			
			Response->Push_String(ReportBuf);
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_GETSCONFIG:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Takes one string, the key name"});
				Client->SendStream(Response);
				break;
			}

			VLScopedPtr<DB::GlobalConfigDBEntry*> Entry { DB::LookupGlobalConfigDBEntry(Stream->Pop_String()) };

			if (!Entry)
			{
				Response->Push_NetCmdStatus({false, STATUS_MISSING, "No such key"});
				Client->SendStream(Response);
				break;
			}

			Response->Push_String(Entry->Value);

			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_SETSCONFIG:
		{			
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING, Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Takes one string, the key name"});
				Client->SendStream(Response);
				break;
			}

			const VLString &KeyName = Stream->Pop_String();
			const VLString &NewValue = Stream->Pop_String();

			const DB::GlobalConfigDBEntry Entry = { KeyName, NewValue };

			const NetCmdStatus Result = DB::UpdateGlobalConfigDB(&Entry);

			Response->Push_NetCmdStatus(Result);
			
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_DROPCONFIG:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Takes one string, the key name"});
				Client->SendStream(Response);
				break;
			}

			const VLString &KeyName = Stream->Pop_String();

			const bool Result = DB::DeleteGlobalConfigEntry(KeyName);

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_FORGETNODE:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Takes a node ID, stored as the destination in an ODHeader, as the only argument"});
				Client->SendStream(Response);
				break;
			}

			const VLString NodeID = Stream->Pop_ODHeader().Destination;
			
			Clients::ClientObj *Node = Clients::LookupClient(NodeID);

			if (Node)
			{
				Conation::ConationStream DisconnectCmd(CMDCODE_ANY_DISCONNECT, 0, 0);
				
				Node->SendStream(DisconnectCmd);
				Clients::ProcessNodeDisconnect(Node, Clients::NODE_DEAUTH_ORDER);
				
				Node = nullptr;
			}

			const bool Result = DB::DeleteNode(NodeID);

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);

			Client->SendStream(HandleIndexRequest(nullptr)); //Force the admin to redraw the node list
			
			break;
		}
		case CMDCODE_A2S_REMOVEUPDATE:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Takes a platform string as the only argument"});
				Client->SendStream(Response);
				break;
			}

			const bool Result = DB::DeletePlatformBinaryEntry(Stream->Pop_String());

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_AUTHTOKEN_ADD:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING, Conation::ARGTYPE_UINT32}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Incorrect arguments."});
				Client->SendStream(Response);
				break;
			}

			const VLString &Token = Stream->Pop_String();
			const DB::AuthTokenPermissions Permissions = static_cast<DB::AuthTokenPermissions>(Stream->Pop_Uint32());
			
			VLScopedPtr<DB::AuthTokensDBEntry*> Lookup { DB::LookupAuthToken(Token) };

			if (Lookup)
			{
				Response->Push_NetCmdStatus({false, STATUS_FAILED, "Authentication token already present"});
				Client->SendStream(Response);
				break;
			}

			DB::AuthTokensDBEntry Entry = { Token, Permissions };

			const bool Result = DB::UpdateAuthTokensDB(&Entry);

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);
			
			break;
		}
		case CMDCODE_A2S_AUTHTOKEN_REVOKE:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING, Conation::ARGTYPE_BOOL}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Incorrect arguments"});
				Client->SendStream(Response);
				break;
			}

			const VLString &Token = Stream->Pop_String();
			const bool KillAffectedNodes = Stream->Pop_Bool();

			const bool Result = DB::DeleteAuthToken(Token);

			if (Result && KillAffectedNodes)
			{
				std::list<Clients::ClientObj> &List = const_cast<std::list<Clients::ClientObj>&>(Clients::GetList());

			ResetRevokeLoop:
				for (auto Iter = List.begin(); Iter != List.end(); ++Iter)
				{
					if (Iter->GetAuthToken() == Token)
					{
						const VLString ID = Iter->GetID();
						
						Clients::ProcessNodeDisconnect(&*Iter, Clients::NODE_DEAUTH_BADAUTHTOKEN);
						DB::DeleteNode(ID);
						goto ResetRevokeLoop;
					}
				}
			}

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_AUTHTOKEN_CHG:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING, Conation::ARGTYPE_UINT32}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Incorrect arguments."});
				Client->SendStream(Response);
				break;
			}

			const VLString &Token = Stream->Pop_String();
			const uint32_t NewPermissions = Stream->Pop_Uint32();
			
			VLScopedPtr<DB::AuthTokensDBEntry*> Lookup { DB::LookupAuthToken(Token) };

			if (!Lookup)
			{
				Response->Push_NetCmdStatus({false, STATUS_MISSING, "No such token is in the database."});
				Client->SendStream(Response);
				break;
			}

			Lookup->Permissions = static_cast<DB::AuthTokenPermissions>(NewPermissions);

			const bool Result = DB::UpdateAuthTokensDB(Lookup);

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_AUTHTOKEN_LIST:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_NONE}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Takes no arguments"});
				Client->SendStream(Response);
				break;
			}

			VLScopedPtr<std::vector<DB::AuthTokensDBEntry>*> Tokens { DB::GetAllAuthTokens() };

			VLString Collated(16384);

			Collated = VLString::UintToString(Tokens->size()) + " authentication tokens found.\n----\n\n";
			
			for (size_t Inc = 0; Inc < Tokens->size(); ++Inc)
			{
				Collated += Tokens->at(Inc).Token + ": " + Utils::ToBinaryString(Tokens->at(Inc).Permissions) + '\n';
			}

			Collated.ShrinkToFit();

			Response->Push_String(Collated);
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_AUTHTOKEN_USEDBY:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED, "Incorrect arguments"});
				Client->SendStream(Response);
				break;
			}

			const VLString &Token = Stream->Pop_String();

			std::list<Clients::ClientObj> &List = const_cast<std::list<Clients::ClientObj>&>(Clients::GetList());

			VLString Collated(List.size() * 64); //Reasonable guess as to the max size of each node name's max length

			bool FoundOne = false;
			
			for (auto Iter = List.begin(); Iter != List.end(); ++Iter)
			{
				if (&*Iter == Clients::LookupCurAdmin()) continue;
				
				if (Iter->GetAuthToken() == Token)
				{
					Collated += Iter->GetID() + '\n';
					FoundOne = true;
				}
			}
			
			VLScopedPtr<DB::AuthTokensDBEntry*> Find { DB::LookupAuthToken(Token) };

			if (!Find)
			{
				if (FoundOne)
				{
					Collated += "----\nThe supplied token has been revoked but is still in use by the above connected nodes.";
				}
				else
				{
					Collated = "No such token found, and no such token in use by any node.";
				}
			}

			Collated.ShrinkToFit();

			Response->Push_String(Collated);
			Client->SendStream(Response);
			
			break;
		}
		case CMDCODE_A2S_SRVLOG_WIPE:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_NONE}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED });
				Client->SendStream(Response);
				break;
			}

			const bool Result = Logger::WipeLog();

			Response->Push_NetCmdStatus(Result);
			Client->SendStream(Response);
			
			break;
		}
		case CMDCODE_A2S_SRVLOG_SIZE:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_NONE}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED });
				Client->SendStream(Response);
				break;
			}

			Response->Push_Uint64(Logger::GetLogSize());
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_SRVLOG_TAIL:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_UINT32}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED });
				Client->SendStream(Response);
				break;
			}

			Response->Push_String(Logger::TailLog(Stream->Pop_Uint32()));
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_ROUTINE_ADD:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING, //Name
										Conation::ARGTYPE_STRING, //Timing format
										Conation::ARGTYPE_UINT32, //Flags
										Conation::ARGTYPE_BINSTREAM, //Routine stream
										Conation::ARGTYPE_STRING, //Target nodes, comma separated list
										}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED});
				Client->SendStream(Response);
				break;
			}

			DB::RoutineDBEntry Entry{};
			Entry.Name = Stream->Pop_String();
			Entry.Schedule = Stream->Pop_String();
			Entry.Flags = static_cast<uint8_t>(Stream->Pop_Uint32());
			Entry.Stream = Conation::ConationStream { Stream->Pop_BinStream().Data };
			
			VLScopedPtr<std::vector<VLString> *> TargetNodes = Utils::SplitTextByCharacter(Stream->Pop_String(), ',');
			Entry.Targets = *TargetNodes;
			
			Routines::AddRoutine(&Entry);
			
			Response->Push_NetCmdStatus(DB::UpdateRoutineDB(&Entry));
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_ROUTINE_DEL:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED});
				Client->SendStream(Response);
				break;
			}

			const VLString &RoutineName = Stream->Pop_String();
			
			Response->Push_NetCmdStatus(Routines::DeleteRoutine(RoutineName) && DB::DeleteRoutineDBEntry(RoutineName));
			Client->SendStream(Response);
			break;
		}
		case CMDCODE_A2S_ROUTINE_LIST:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}
			
			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_NONE}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED});
				Client->SendStream(Response);
				break;
			}
			
			uint32_t NumRoutines = 0;
			const VLString &RoutinesReport = Routines::CollateRoutineList(&NumRoutines);
			
			Response->Push_Uint32(NumRoutines);
			Response->Push_String(RoutinesReport);
			Client->SendStream(Response);
			break;
			
		}
		case CMDCODE_A2S_ROUTINE_CHG_FLAG:
		case CMDCODE_A2S_ROUTINE_CHG_SCHD: ///These two have identical requirements, therefore we can reuse most of this.
		case CMDCODE_A2S_ROUTINE_CHG_TGT:
		{
			if (!IsAdmin)
			{
				Clients::ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
				break;
			}

			Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

			if (!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING,
										Conation::ARGTYPE_STRING})
										&&
				!Stream->VerifyArgTypes({Conation::ARGTYPE_STRING,
										Conation::ARGTYPE_UINT32}))
			{
				Response->Push_NetCmdStatus({false, STATUS_MISUSED});
				Client->SendStream(Response);
				break;
			}

			VLScopedPtr<DB::RoutineDBEntry*> Entry = DB::LookupRoutineDBEntry(Stream->Pop_String());
			Routines::RoutineInfo *InfoEntry = Entry ? Routines::LookupRoutine(Entry->Name) : nullptr;

			VLScopedPtr<Conation::ConationStream::BaseArg *> Value = Stream->PopArgument();
			
			if (!Entry || !InfoEntry)
			{
				Response->Push_NetCmdStatus({false, Entry ? STATUS_IERR : STATUS_MISSING});
				Client->SendStream(Response);
				break;
			}

			switch (Stream->GetCommandCode())
			{
				case CMDCODE_A2S_ROUTINE_CHG_SCHD:
				{
					if (Value->Type != Conation::ARGTYPE_STRING)
					{
						Response->Push_NetCmdStatus({false, STATUS_MISUSED});
						Client->SendStream(Response);
						goto TripleRoutineAlterExit;
					}
					Entry->Schedule = Value->ReadAs<Conation::ConationStream::StringArg>().String;
					break;
				}
				case CMDCODE_A2S_ROUTINE_CHG_TGT:
				{
					if (Value->Type != Conation::ARGTYPE_STRING)
					{
						Response->Push_NetCmdStatus({false, STATUS_MISUSED});
						Client->SendStream(Response);
						goto TripleRoutineAlterExit;
					}
					
					VLScopedPtr<std::vector<VLString> *> Targets = Utils::SplitTextByCharacter(Value->ReadAs<Conation::ConationStream::StringArg>().String, ',');
					
					Entry->Targets = *Targets;
					break;
				}
				case CMDCODE_A2S_ROUTINE_CHG_FLAG:
				{
					if (Value->Type != Conation::ARGTYPE_UINT32)
					{
						Response->Push_NetCmdStatus({false, STATUS_MISUSED});
						Client->SendStream(Response);
						goto TripleRoutineAlterExit;
					}

					Entry->Flags = Value->ReadAs<Conation::ConationStream::Uint32Arg>().Value;
					break;
				}
				default: //Will never happen
					break;
			}

			if (!DB::UpdateRoutineDB(Entry))
			{
				Response->Push_NetCmdStatus({false, STATUS_FAILED});
				Client->SendStream(Response);
				break;
			}

			//Now that it worked for on-disk, update the in-memory version.
			*InfoEntry = *Entry;

			Response->Push_NetCmdStatus(true);
			
			Client->SendStream(Response);
		TripleRoutineAlterExit:
			break;
		}
		default:
		{
			VLScopedPtr<std::vector<Conation::ArgType>*> Types { Stream->GetArgTypes() };
			
			if (Stream->GetCommandCode() >= CMDCODE_MAX)
			{ //Invalid or unrecognized command code.
				VLString RejectMsg = "CmdHandling::HandleRequest(): Rejecting stream with command code integer " + VLString::UintToString(Stream->GetCommandCode());
				RejectMsg += " and argument types ";
				
				for (size_t Inc = 0; Inc < Types->size(); ++Inc)
				{
					RejectMsg += ArgTypeToString(Types->at(Inc));
					if (Inc + 1 < Types->size())
					{
						RejectMsg += ", ";
					}
				}
				Logger::WriteLogLine(Logger::LOGITEM_SECUREWARN, RejectMsg);
				break;
			}
			

			if (!Types || Types->at(0) != Conation::ARGTYPE_ODHEADER)
			{ //This is garbage.
				break;
			}
			
			Conation::ConationStream::ODHeader ODObj = Stream->Pop_ODHeader();

			Stream->Rewind(); //Probaby pointless, but leave it just in case.
			
			//Permission denied, either malformed or malicious, do nothing.
			if (Client != Clients::LookupCurAdmin() || ODObj.Origin != "ADMIN" || ODObj.Destination == "ADMIN") break;

			Clients::ClientObj *Target = Clients::LookupClient(ODObj.Destination);

			if (!Target) break;
			Target->SendStream(*Stream); //Important it's dereferenced so we make a copy of it.
			break;

		}
	}
}

static Conation::ConationStream *HandleNodeInfoRequest(Clients::ClientObj *Client, Conation::ConationStream *Stream, const char *RequestedID)
{
	if (!*RequestedID || VLString(RequestedID) == "ADMIN")
	{ //Misused.
		return nullptr;
	}

	Clients::ClientObj *const Lookup = Clients::LookupClient(RequestedID);

	if (!Lookup)
	{
		return nullptr;
	}

	uint64_t Ident = 0;

	Stream->GetCommandIdent(nullptr, &Ident);

	Conation::ConationStream *const Result = new Conation::ConationStream(CMDCODE_A2S_INFO, Conation::IDENT_ISREPORT_BIT, Ident);

	//What OS are they running on
	Result->Push_String(Lookup->GetPlatformString());

	//What revision of node they are
	Result->Push_String(Lookup->GetNodeRevision());
	
	//Time they connected.
	Result->Push_Int64(Lookup->GetConnectedTime());

	//Their IP address.
	Result->Push_String(Lookup->GetIPAddr());

	return Result;
}

Conation::ConationStream *CmdHandling::HandleIndexRequest(Conation::ConationStream *Stream)
{
	uint64_t Ident = 0;

	if (Stream)
	{ //We can pass null here.
		Stream->GetCommandIdent(nullptr, &Ident);
	}

	Conation::ConationStream *const Result = new Conation::ConationStream(CMDCODE_A2S_INDEXDL, Conation::IDENT_ISREPORT_BIT, Ident);
	
	const std::list<Clients::ClientObj> &OnlineList = Clients::GetList();


	assert(Clients::LookupCurAdmin());
	
	for (auto Iter = OnlineList.begin(); Iter != OnlineList.end(); ++Iter)
	{
		const Clients::ClientObj *Cur = &*Iter;

		//Is it an admin.
		if (Cur == Clients::LookupCurAdmin())
		{
#ifdef DEBUG
			puts(VLString("Detected admin in HandleIndexRequest iteration."));
#endif
			continue;
		}
		//Node IDs
		Result->Push_String(Cur->GetID());
#ifdef DEBUG
		puts(VLString("Transmitting \"") + Cur->GetID() + "\" from HandleIndexRequest()");
#endif
		
		//OS it's running on.
		Result->Push_String(Cur->GetPlatformString());

		//Node software revision
		Result->Push_String(Cur->GetNodeRevision());
		
		//Group if applicable.
		Result->Push_String(Cur->GetGroup());
		
		//Time they connected.
		Result->Push_Int64(Cur->GetConnectedTime());

		//IP address
		Result->Push_String(Cur->GetIPAddr());

		//Ping latency in milliseconds.
		Result->Push_Int64(Cur->GetPingLatency());
		
		//This node is online.
		Result->Push_Bool(true);
		
	}
	
	std::list<DB::NodeDBEntry> OfflineList;
	
	if (!DB::GetOfflineNodes(OfflineList)) goto OfflineFucky;
	
	for (auto Iter = OfflineList.begin(); Iter != OfflineList.end(); ++Iter)
	{
		DB::NodeDBEntry *Cur = &*Iter;
		
		//Node IDs
		Result->Push_String(Cur->ID);
		
		//OS it's running on.
		Result->Push_String(Cur->PlatformString);

		//Node software revision
		Result->Push_String(Cur->NodeRevision);
		
		//Group if applicable.
		Result->Push_String(Cur->Group);
		
		//Time they connected.
		Result->Push_Int64(Cur->LastConnectedTime);

		//IP address
		Result->Push_String("");

		//Ping latency in microseconds
		Result->Push_Int64(0LL);
		
		//This node is offline.
		Result->Push_Bool(false);
	}
OfflineFucky:

	return Result;
}

bool CmdHandling::NotifyAdmin_NodeChange(const char *NodeID, const bool Online)
{
	if (!Clients::LookupCurAdmin()) return false;
	
	VLScopedPtr<Conation::ConationStream*> Result { new Conation::ConationStream(CMDCODE_S2A_NOTIFY_NODECHG, 0, 0u) };

	Clients::ClientObj *const Client = Online ? Clients::LookupClient(NodeID) : nullptr;

	assert(Client || !Online);
	
	if (Client && Client == Clients::LookupCurAdmin()) return false; //Why the hell did you ask for this?!?
	
	//For offline we get information from the DB.
	VLScopedPtr<DB::NodeDBEntry*> Entry { DB::LookupNodeInfo(NodeID) };
	
	if (!Entry)
	{
		return false;
	}
	
	//Node IDs
	Result->Push_String(Entry->ID);
	
	//OS it's running on.
	Result->Push_String(Entry->PlatformString);

	//Node software revision
	Result->Push_String(Entry->NodeRevision);
	
	//Group if applicable.
	Result->Push_String(Entry->Group);
	
	//Time they connected.
	Result->Push_Int64(Entry->LastConnectedTime);

	//IP address
	Result->Push_String(Client ? Client->GetIPAddr() : "");

	//Ping latency in microseconds
	Result->Push_Int64(Client ? Client->GetPingLatency() : 0LL);
	
	//This node is online.
	Result->Push_Bool(Online);

	Clients::LookupCurAdmin()->SendStream(Result.Forget());
	
	return true;
	
}

void CmdHandling::HandleN2N(Clients::ClientObj *Client, Conation::ConationStream *Stream)
{
	VLScopedPtr<std::vector<Conation::ArgType>*> Types { Stream->GetArgTypes() };
	
	if (!Types || Types->at(0) != Conation::ARGTYPE_ODHEADER)
	{
		Logger::WriteLogLine(Logger::LOGITEM_SECUREWARN, VLString{"Invalid node-to-node message sent from node "} + Client->GetID());
		
		return;
	}
	
	const Conation::ConationStream::ODHeader &ODObj { Stream->Pop_ODHeader() };
	
	if (ODObj.Origin != Client->GetID())
	{ ///If you *really* want to, I see no reason not to let you send shit to yourself, so there's no checks for that here.
		Logger::WriteLogLine(Logger::LOGITEM_SECUREERROR, VLString{"Node "} + Client->GetID() + " attempted to masquerade as node \"" + ODObj.Origin + "\". Killing node.");
		ProcessNodeDisconnect(Client, Clients::NODE_DEAUTH_EVIL);
		return;
	}
	
	Clients::ClientObj *const DestClient = Clients::LookupClient(ODObj.Destination);
	
	if (!DestClient)
	{ //Eat the message since it has no home.
		return;
	}
	
	const VLScopedPtr<DB::AuthTokensDBEntry*> OurToken { DB::LookupAuthToken(Client->GetAuthToken()) };
	const VLScopedPtr<DB::AuthTokensDBEntry*> TheirToken { DB::LookupAuthToken(DestClient->GetAuthToken()) };
	
	if (!OurToken || !TheirToken)
	{ //Umm, just don't let them send messages, I guess?
		return;
	}
	
	if ((!(OurToken->Permissions & DB::ATP_N2NCOMM_ANY) || !(TheirToken->Permissions & DB::ATP_N2NCOMM_ANY)) &&
		((!(OurToken->Permissions & DB::ATP_N2NCOMM_GROUP) || !(TheirToken->Permissions & DB::ATP_N2NCOMM_GROUP)) ||
		Client->GetGroup() != DestClient->GetGroup()))
	{
		Logger::WriteLogLine(Logger::LOGITEM_SECUREWARN,
							VLString{"Node "} + ODObj.Origin +
							" attempted to send an N2N to " + ODObj.Destination +
							" but they lack the required permissions.");
		return;
	}
	
	//Now we're ready to send it.
	DestClient->SendStream(*Stream);
}
