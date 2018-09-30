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
#include "../libvolition/include/conation.h"

#include <set>

#include "cmdhandling.h"
#include "clienttracker.h"
#include "gui_base.h"
#include "gui_mainwindow.h"
#include "ticker.h"
#include "mediarecv.h"
#include "config.h"
#include "main.h"

static std::map<StatusCode, VLString> TickerStatusCodeText =
														{
															{ STATUS_INVALID, "(invalid value)" },
															{ STATUS_OK, "<span foreground=\"#00dd00\">OK</span>" },
															{ STATUS_FAILED, "<span foreground=\"#dd0000\">FAILURE</span>" },
															{ STATUS_WARN, "<span foreground=\"#FF8800\">Warning</span>" },
															{ STATUS_IERR, "<span foreground=\"#dd0000\">Internal error</span>" },
															{ STATUS_UNSUPPORTED, "<span foreground=\"#dd0000\">Unsupported operation</span>" },
															{ STATUS_ACCESSDENIED, "<span foreground=\"#dd0000\">Access denied</span>" },
															{ STATUS_MISSING, "<span foreground=\"#dd0000\">File or other resource missing</span>" },
															{ STATUS_MISUSED, "<span foreground=\"#dd0000\">Command misused, argument sequence incorrect</span>" },
														};

static void ProcessIndex(Conation::ConationStream *Stream);
static void ProcessNodeChange(Conation::ConationStream *Stream);
static void AddNodeCommandStatusReport(Conation::ConationStream *Stream);
static void AddServerCommandStatusReport(Conation::ConationStream *Stream);
static void ProcessJobsList(Conation::ConationStream *Stream);

void CmdHandling::HandleReport(Conation::ConationStream *Stream)
{
	Conation::ConationStream::StreamHeader Hdr = Stream->GetHeader();

	if ((Hdr.CmdIdentFlags & Conation::IDENT_ISREPORT_BIT) == 0) return;

	switch (Hdr.CmdCode)
	{
		case CMDCODE_A2S_INDEXDL:
			ProcessIndex(Stream);
			break;
		case CMDCODE_A2C_CHDIR:
		case CMDCODE_A2C_EXECCMD:
		case CMDCODE_A2C_GETCWD:
		case CMDCODE_A2C_FILES_COPY:
		case CMDCODE_A2C_FILES_DEL:
		case CMDCODE_A2C_FILES_MOVE:
		case CMDCODE_A2C_FILES_PLACE:
		case CMDCODE_A2C_FILES_FETCH:
		case CMDCODE_A2C_GETPROCESSES:
		case CMDCODE_A2C_KILLPROCESS:
		case CMDCODE_A2C_SLEEP:
		case CMDCODE_A2C_MOD_EXECFUNC:
		case CMDCODE_A2C_MOD_LOADSCRIPT:
		case CMDCODE_A2C_MOD_UNLOADSCRIPT:
		case CMDCODE_A2C_MOD_GENRESP:
		case CMDCODE_B2C_KILLJOBBYCMDCODE:
		case CMDCODE_B2C_KILLJOBID:
		case CMDCODE_A2C_LISTDIRECTORY:
		{
#ifdef DEBUG
			puts(VLString("CmdHandling::HandleReport(): Processing node report with command code ") + CommandCodeToString(Hdr.CmdCode)
				+ ", ident is " + Utils::ToBinaryString(Stream->GetCmdIdentComposite()));
#endif
			AddNodeCommandStatusReport(Stream);
			break;
		}
		case CMDCODE_B2C_USEUPDATE:
		case CMDCODE_A2S_FORGETNODE:
		case CMDCODE_A2S_PROVIDEUPDATE:
		case CMDCODE_A2S_REMOVEUPDATE:
		case CMDCODE_A2S_CHGNODEGROUP:
		case CMDCODE_B2S_VAULT_ADD:
		case CMDCODE_B2S_VAULT_DROP:
		case CMDCODE_B2S_VAULT_FETCH:
		case CMDCODE_B2S_VAULT_UPDATE:
		case CMDCODE_A2S_VAULT_LIST:
		case CMDCODE_A2S_GETSCONFIG:
		case CMDCODE_A2S_SETSCONFIG:
		case CMDCODE_A2S_DROPCONFIG:
		case CMDCODE_A2S_AUTHTOKEN_ADD:
		case CMDCODE_A2S_AUTHTOKEN_CHG:
		case CMDCODE_A2S_AUTHTOKEN_REVOKE:
		case CMDCODE_A2S_AUTHTOKEN_LIST:
		case CMDCODE_A2S_AUTHTOKEN_USEDBY:
		case CMDCODE_A2S_SRVLOG_SIZE:
		case CMDCODE_A2S_SRVLOG_TAIL:
		case CMDCODE_A2S_SRVLOG_WIPE:
		case CMDCODE_A2S_ROUTINE_ADD:
		case CMDCODE_A2S_ROUTINE_DEL:
		case CMDCODE_A2S_ROUTINE_LIST:
		case CMDCODE_A2S_ROUTINE_CHG_SCHD:
		case CMDCODE_A2S_ROUTINE_CHG_TGT:
		case CMDCODE_A2S_ROUTINE_CHG_FLAG:
			AddServerCommandStatusReport(Stream);
			break;
		case CMDCODE_B2C_GETJOBSLIST:
			ProcessJobsList(Stream);
			break;
		default:
			break;
	}
}

void CmdHandling::HandleRequest(Conation::ConationStream *Stream)
{
	Conation::ConationStream::StreamHeader Hdr = Stream->GetHeader();

	if (Hdr.CmdIdentFlags & Conation::IDENT_ISREPORT_BIT) return;

	switch (Hdr.CmdCode)
	{
		case CMDCODE_ANY_PING:
		{
			auto NewHeader = Hdr;
			NewHeader.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
			
			Conation::ConationStream *Response = new Conation::ConationStream(NewHeader, Stream->GetArgData());

			Main::GetWriteQueue().Push(Response);
			break;
		}
		case CMDCODE_S2A_NOTIFY_NODECHG:
		{
			ProcessNodeChange(Stream);
			break;
		}
		default:
			break;
	}
}

static void ProcessJobsList(Conation::ConationStream *Stream)
{
	const Conation::ConationStream::StreamHeader &Hdr = Stream->GetHeader();
	
	const Conation::ConationStream::ODHeader &ODObj = Stream->Pop_ODHeader();
	
	const size_t NumArgs = Stream->CountArguments() - 1; //Minus 1 for the ODHeader.
	
	const size_t NumPieces = 3;
	
	if (NumArgs % NumPieces)
	{
#ifdef DEBUG
		puts("ProcessJobsList(): Uneven number of arguments provided!");
#endif
		return;
	}
	
	VLString Msg(8192);
	
	Msg = VLString::UintToString(NumArgs / NumPieces) + " jobs running on node.\n----\n";
	
	for (size_t Inc = 0; Inc < NumArgs / NumPieces; ++Inc)
	{
		Msg += VLString("Job ID: ") + VLString::UintToString(Stream->Pop_Uint64()) + '\n';
		Msg += VLString("Command Code: ") + CommandCodeToString(static_cast<CommandCode>(Stream->Pop_Uint32())) + '\n';
		
		const uint64_t Ident = Stream->Pop_Uint64();
		Msg += VLString("Command identifer: ") + VLString::UintToString(Ident)
						+ ", flags: " + Utils::ToBinaryString(Conation::GetIdentFlags(Ident)) + '\n';
		Msg += "----\n";
	}

	Ticker::AddNodeMessage(ODObj.Origin, Hdr.CmdCode, Hdr.CmdIdent, 
							VLString("Received jobs report from node. ") + VLString::UintToString(NumArgs / NumPieces) + " running.", Msg);
}

static void ProcessNodeChange(Conation::ConationStream *Stream)
{
#ifdef DEBUG
	puts("Entered ProcessNodeChange()");
#endif
	static const size_t ArgPieces = 8;

	assert(Stream->GetCommandCode() == CMDCODE_S2A_NOTIFY_NODECHG);
	
	if (Stream->CountArguments() != ArgPieces ||
		!Stream->VerifyArgTypes( Conation::ARGTYPE_STRING,
								Conation::ARGTYPE_STRING,
								Conation::ARGTYPE_STRING,
								Conation::ARGTYPE_STRING, 
								Conation::ARGTYPE_INT64, 
								Conation::ARGTYPE_STRING, 
								Conation::ARGTYPE_INT64, 
								Conation::ARGTYPE_BOOL,
								Conation::ARGTYPE_NONE))
	{
		throw (Conation::ConationStream::Err_CorruptStream());
	}
	
	ClientTracker::ClientRecord Record{};

	Record.ID = Stream->Pop_String();

	Record.PlatformString = Stream->Pop_String();
	
	Record.NodeRevision = Stream->Pop_String();

	Record.Group = Stream->Pop_String();
	
	Record.ConnectedTime = Stream->Pop_Int64();

	Record.IPAddr = Stream->Pop_String();

	Record.LastPingLatency = Stream->Pop_Int64();

	Record.Online = Stream->Pop_Bool();
	

	//Add message to ticker.
	VLString Msg;

	ClientTracker::ClientRecord *Lookup = ClientTracker::Lookup(Record.ID);
	
	if (Lookup != nullptr && Record.Online == Lookup->Online)
	{
		goto SkipTicker;
	}
	
	if (Record.Online)
	{
		Msg = VLString("Node ") + Record.ID + " has <b><span foreground=\"#00dd00\">joined</span></b> the network.";
	}
	else
	{
		Msg = VLString("Node ") + Record.ID + " has gone <b><span foreground=\"#dd0000\">offline</span></b>.";
	}
	
	Ticker::AddServerMessage(Msg);
SkipTicker:

	const bool ForceGroupCheck = !ClientTracker::GroupHasMember(Record.Group) || (Lookup && Lookup->Group != Record.Group);
	
	ClientTracker::Add(Record);

	GuiMainWindow::MainWindowScreen *Screen = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));

	if (!Screen) return;

	Screen->AddNodeToTree(&Record, ForceGroupCheck);

}

static void ProcessIndex(Conation::ConationStream *Stream)
{
#ifdef DEBUG
	puts("Entered ProcessIndex()");
#endif

	static const size_t PiecesPerNode = 8;

	const size_t ArgCount = Stream->CountArguments();

	if (ArgCount % PiecesPerNode) //BAAAAADDDD
	{
		throw (Conation::ConationStream::Err_CorruptStream());
	}
	
	GuiBase::LoadingScreen *Scr = static_cast<GuiBase::LoadingScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::LOADING));

	const double PulseFraction = Scr ? ((ArgCount / PiecesPerNode) / (1.0 - Scr->GetProgressBarFraction())) : 0.0;
	
	std::set<VLString> CheckAfter;
	
	for (size_t Inc = 0; Inc < ArgCount / PiecesPerNode; ++Inc)
	{
		ClientTracker::ClientRecord Record{};

		Record.ID = Stream->Pop_String();

		Record.PlatformString = Stream->Pop_String();
		
		Record.NodeRevision = Stream->Pop_String();

		Record.Group = Stream->Pop_String();
		
		Record.ConnectedTime = Stream->Pop_Int64();

		Record.IPAddr = Stream->Pop_String();

		Record.LastPingLatency = Stream->Pop_Int64();

		Record.Online = Stream->Pop_Bool();
		
		ClientTracker::Add(Record);
		
		CheckAfter.insert(Record.ID);
		
		if (Scr != nullptr)
		{
			if (Scr->GetProgressBarFraction() + PulseFraction > 1.0)
			{
				Scr->SetProgressBarFraction(1.0);
			}
			else
			{
				Scr->IncreaseProgressBarFraction(PulseFraction);
			}
			gtk_main_iteration_do(false);
		}
	}

	//Remove nodes that are already in the database that have been removed by the server.
ResetCheckLoop:
	for (ClientTracker::RecordIter Iter = ClientTracker::GetIterator(); Iter; ++Iter)
	{
		if (!CheckAfter.count(Iter->ID))
		{
			ClientTracker::Delete(Iter->ID);
			goto ResetCheckLoop;
		}
	}
	
	if (Scr)
	{
		Scr->SetProgressBarFraction(1.0);
		Scr->SetStatusText("Index download completed");
		gtk_main_iteration_do(false);
		
		GuiBase::PurgeScreen(GuiBase::ScreenObj::ScreenType::LOADING);
		
		auto Screen = new GuiMainWindow::MainWindowScreen;
		Screen->Show();
	}
	else
	{ //We're having to do this ourselves.
		GuiMainWindow::MainWindowScreen *Ptr = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));

		Ptr->LoadNodeTree();
	}
}

static void AddServerCommandStatusReport(Conation::ConationStream *Stream)
{
#ifdef DEBUG
	puts("Entered AddServerCommandStatusReport()");
#endif
	VLString Msg;
	
	if (Stream->VerifyArgTypes(Conation::ARGTYPE_NETCMDSTATUS, Conation::ARGTYPE_NONE))
	{
		NetCmdStatus Status = Stream->Pop_NetCmdStatus();
		
		Msg = VLString("Order to server ") + CommandCodeToString(Stream->GetCommandCode())
			+ (Status.WorkedAtAll ? " succeeded" : " failed") + " with status code " + TickerStatusCodeText[Status.Status] + ": \""
			+ Status.Msg + "\"";
		Ticker::AddServerMessage(Msg, Status.Msg);
		return;
	}

	std::vector<Conation::ArgType> *ArgTypes = Stream->GetArgTypes();

	if (!ArgTypes)
	{
		return;
	}

	Msg = VLString("Server order ") + CommandCodeToString(Stream->GetCommandCode()) + ": ";
	
	for (size_t Inc = 0u; Inc < ArgTypes->size(); ++Inc)
	{
		switch (ArgTypes->at(Inc))
		{
			case Conation::ARGTYPE_FILE:
			{
				Conation::ConationStream::FileArg File = Stream->Pop_File();
				
				MediaRecv::ReceiveFile(File.Filename, File.Data, File.DataSize);

				Msg = VLString("Received file \"") + File.Filename + "\" from server, saved locally.";

				Ticker::AddServerMessage(Msg, VLString("File saved to directory ") + Config::GetKey("DownloadDirectory"));
				
				break;
			}
			case Conation::ARGTYPE_STRING:
			{
				VLString Data = Stream->Pop_String();
				
				Msg = strchr(Data, '\n') ? VLString("Received multi-line text string from server.") : VLString("Received text string \"") + Data + "\" from server.";

				Ticker::AddServerMessage(Msg, Data);
				break;
			}
			case Conation::ARGTYPE_FILEPATH:
			{
				VLString Data = Stream->Pop_FilePath();

				Msg = VLString("Received file path \"") + Data + "\" from server.";
				
				Ticker::AddServerMessage(Msg, Data);
				break;
			}
			case Conation::ARGTYPE_INT32:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%d", (int)Stream->Pop_Int32());

				Msg = VLString ("Received int32_t ") + (const char*)NumBuf + " from server.";
				
				Ticker::AddServerMessage(Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_UINT32:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%u", (unsigned)Stream->Pop_Uint32());

				Msg = VLString ("Received uint32_t ") + (const char*)NumBuf + " from server.";
				
				Ticker::AddServerMessage(Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_INT64:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%lld", (long long)Stream->Pop_Int64());

				Msg = VLString ("Received int64_t ") + (const char*)NumBuf + " from server.";
				
				Ticker::AddServerMessage(Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_UINT64:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%llu", (unsigned long long)Stream->Pop_Uint64());

				Msg = VLString ("Received uint64_t ") + (const char*)NumBuf + " from server.";
				
				Ticker::AddServerMessage(Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_BOOL:
			{
				Msg = VLString("Received boolean \"") + (Stream->Pop_Bool() ? "true" : "false") + "\" from node.";
				
				Ticker::AddServerMessage(Msg, Msg);
				break;
			}
			case Conation::ARGTYPE_NETCMDSTATUS:
			{
				NetCmdStatus Status = Stream->Pop_NetCmdStatus();

				Msg = VLString("Received command status report: ")
						+ ((Status.Status == STATUS_OK || Status.Status == STATUS_WARN)  ? "succeeded " : "failed ")
						+ "with status code " + TickerStatusCodeText[Status.Status] + ": \"" + Status.Msg + "\"";
		
				Ticker::AddServerMessage(Msg, Status.Msg);
				break;
			}
			default:
				delete Stream->PopArgument();
				break;
		}
	}
}


static void AddNodeCommandStatusReport(Conation::ConationStream *Stream)
{
#ifdef DEBUG
	puts("Entered AddNodeCommandStatusReport()");
#endif

	VLString Msg;
	
	//Simple report.
	if (Stream->VerifyArgTypes(Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_NETCMDSTATUS, Conation::ARGTYPE_NONE))
	{
		const Conation::ConationStream::ODHeader &Hdr = Stream->Pop_ODHeader();

		NetCmdStatus Status = Stream->Pop_NetCmdStatus();

		Msg = VLString("Order ")
				+ ((Status.Status == STATUS_OK || Status.Status == STATUS_WARN)  ? "succeeded " : "failed ")
				+ "with status code " + TickerStatusCodeText[Status.Status] + ": \"" + Status.Msg + "\"";

		Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, Status.Msg);
#ifdef DEBUG
		puts(VLString("AddNodeCommandStatusReport(): Added generic report for ") + CommandCodeToString(Stream->GetCommandCode()) + ':'
			+ VLString::UintToString(Stream->GetCmdIdentOnly()));
#endif
		return;
	}

	std::vector<Conation::ArgType> *ArgTypes = Stream->GetArgTypes();

	if (!ArgTypes)
	{
		return; //I mean, what can we do?
	}

	if (ArgTypes->at(0) != Conation::ARGTYPE_ODHEADER)
	{
#ifdef DEBUG
		puts("AddNodeCommandStatusReport(): No ODHeader for node report! Returning.");
#endif
		return;
	}
	
	const Conation::ConationStream::ODHeader &Hdr = Stream->Pop_ODHeader();

	//We start at 1 to skip the ODHeader.
	for (size_t Inc = 1u; Inc < ArgTypes->size(); ++Inc)
	{
		switch (ArgTypes->at(Inc))
		{
			case Conation::ARGTYPE_FILE:
			{
				Conation::ConationStream::FileArg File = Stream->Pop_File();
				
				MediaRecv::ReceiveFile(File.Filename, File.Data, File.DataSize);

				Msg = VLString("Received file \"") + File.Filename + "\" from node, saved locally.";

				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, VLString("File saved to directory ") + Config::GetKey("DownloadDirectory"));
				
				break;
			}
			case Conation::ARGTYPE_STRING:
			{
				VLString Data = Stream->Pop_String();
				
				Msg = strchr(Data, '\n') ? VLString("Received multi-line text string from node.") : VLString("Received text string \"") + Data + "\" from node.";

				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, Data);
				break;
			}
			case Conation::ARGTYPE_FILEPATH:
			{
				VLString Data = Stream->Pop_FilePath();

				Msg = VLString("Received file path \"") + Data + "\" from node.";
				
				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, Data);
				break;
			}
			case Conation::ARGTYPE_INT32:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%d", (int)Stream->Pop_Int32());

				Msg = VLString ("Received int32_t ") + (const char*)NumBuf + " from node.";
				
				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_UINT32:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%u", (unsigned)Stream->Pop_Uint32());

				Msg = VLString ("Received uint32_t ") + (const char*)NumBuf + " from node.";
				
				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_INT64:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%lld", (long long)Stream->Pop_Int64());

				Msg = VLString ("Received int64_t ") + (const char*)NumBuf + " from node.";
				
				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_UINT64:
			{
				char NumBuf[256];
				snprintf(NumBuf, sizeof NumBuf, "%llu", (unsigned long long)Stream->Pop_Uint64());

				Msg = VLString ("Received uint64_t ") + (const char*)NumBuf + " from node.";
				
				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, NumBuf);
				break;
			}
			case Conation::ARGTYPE_BOOL:
			{
				Msg = VLString("Received boolean \"") + (Stream->Pop_Bool() ? "true" : "false") + "\" from node.";
				
				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, Msg);
				break;
			}
			case Conation::ARGTYPE_NETCMDSTATUS:
			{
				NetCmdStatus Status = Stream->Pop_NetCmdStatus();

				Msg = VLString("Received command status report: ")
						+ ((Status.Status == STATUS_OK || Status.Status == STATUS_WARN)  ? "succeeded " : "failed ")
						+ "with status code " + TickerStatusCodeText[Status.Status] + ": \"" + Status.Msg + "\"";
		
				Ticker::AddNodeMessage(Hdr.Origin, Stream->GetCommandCode(), Stream->GetCmdIdentOnly(), Msg, Status.Msg);
				break;
			}
			default:
				delete Stream->PopArgument();
				break;
		}
	}
}
