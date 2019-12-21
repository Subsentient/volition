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
#include "../libvolition/include/utils.h"
#include "orders.h"
#include "gui_dialogs.h"
#include "gui_base.h"
#include "gui_mainwindow.h"
#include "gui_menus.h"
#include "main.h"
#include "ticker.h"
#include "scriptscanner.h"

//Types

struct DialogEntry
{
	GuiDialogs::DialogType DialogType;
	Conation::ArgType SendAs;
	VLString Title;
	VLString Prompt;
	uint64_t Integer;
	void *Ptr;
	
	//Functions
	DialogEntry(const GuiDialogs::DialogType InDialogType,
				const Conation::ArgType InSendAs,
				const char *InTitle,
				const char *InPrompt,
				uint64_t InInteger = 0ull,
				void *InPtr = nullptr)
					:
				DialogType(InDialogType),
				SendAs(InSendAs),
				Title(InTitle),
				Prompt(InPrompt),
				Integer(InInteger),
				Ptr(InPtr) {}
};

struct DialogSpecStruct
{
	CommandCode CmdCode;
	enum SpecFlagsEnum
	{
		FLAG_NONE = 0,
		FLAG_CONCATNT_COMMA = 1 << 0, //Add the node targets to a comma delimited string at the end of the stream arguments, rather than compile a stream for each node. For server orders mostly.
		FLAG_MAX
	};
	
	SpecFlagsEnum SpecFlags;
	std::vector<DialogEntry> Dialogs;
};

enum ArgDialogFlags
{
	ADF_CHG_CMDCODE = 1,
	ADF_WIPE_ARGS = 1 << 1,
	ADF_AS_BINSTREAM = 1 << 2,
};

const DialogSpecStruct DialogCommandMap[] =
	{ //At the time of writing this, I have no idea how C++11's std::vector initializer lists work. :^S
		{ CMDCODE_INVALID, DialogSpecStruct::FLAG_NONE,					{ DialogEntry(GuiDialogs::DIALOG_ARGSELECTOR, Conation::ARGTYPE_ANY, "Build stream", "Specify the arguments for the stream.", ADF_CHG_CMDCODE | ADF_WIPE_ARGS) } },
		{ CMDCODE_A2C_CHDIR, DialogSpecStruct::FLAG_NONE,				{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_FILEPATH, "Enter new working directory", "Enter new working directory path.") } },
		{ CMDCODE_A2C_EXECCMD, DialogSpecStruct::FLAG_NONE,				{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Specify command", "Specify the command for node execution.") } },
		{ CMDCODE_A2C_FILES_COPY, DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_FILEPATH, "Specify source", "Specify source for copy operation."),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_FILEPATH, "Specify destination", "Specify destination for copy operation.") } },
		{ CMDCODE_A2C_FILES_MOVE,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_FILEPATH, "Specify source", "Specify source for move operation."),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_FILEPATH, "Specify destination", "Specify destination for move operation.") } },
		{ CMDCODE_A2C_FILES_DEL,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_LARGETEXT, Conation::ARGTYPE_FILEPATH, "Specify files to delete", "Enter a newline delimited list of file paths to delete.", true) } },
		{ CMDCODE_A2C_FILES_FETCH,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_LARGETEXT, Conation::ARGTYPE_FILEPATH, "Specify files to fetch", "Enter a newline delimited list of file paths to download.", true) } },
		{ CMDCODE_A2C_FILES_PLACE,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_FILECHOOSER, Conation::ARGTYPE_FILE, "Select files", "Select files to upload.", true),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_FILEPATH, "Enter destination directory", "Enter a destination directory for the selected files.") } },
		{ CMDCODE_A2C_LISTDIRECTORY,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_FILEPATH, "Enter path", "Enter the path of a directory on the host system to list.") } },
		{ CMDCODE_A2C_KILLPROCESS,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_LARGETEXT, Conation::ARGTYPE_STRING, "Enter process names or PIDs", "Enter a newline delimited list of process names and/or PIDs to kill.", true) } },
		{ CMDCODE_A2C_GETPROCESSES,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_YESNODIALOG, Conation::ARGTYPE_BOOL, "Input needed", "Show kernel processes if present?") } },
		{ CMDCODE_A2C_GETCWD,DialogSpecStruct::FLAG_NONE,				{ } },
		{ CMDCODE_A2C_SLEEP, DialogSpecStruct::FLAG_NONE,				{ } },
		{ CMDCODE_A2S_PROVIDEUPDATE,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_FILECHOOSER, Conation::ARGTYPE_FILE, "Select new binary", "Select a new node binary.\nThe platform string and revision\nwill be read from branding by the server.") } },
		{ CMDCODE_A2S_CHGNODEGROUP,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter new group", "Enter new group for selected nodes.") } },
		{ CMDCODE_B2C_GETJOBSLIST,DialogSpecStruct::FLAG_NONE,			{ } },
		{ CMDCODE_B2C_KILLJOBBYCMDCODE,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_UINT32, "Enter command code", "Enter a command code integer.") } },
		{ CMDCODE_B2C_KILLJOBID,DialogSpecStruct::FLAG_NONE,		 	{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_UINT64, "Enter Job ID", "Enter a job ID to kill.") } },
		{ CMDCODE_B2C_USEUPDATE,DialogSpecStruct::FLAG_NONE,		 	{ DialogEntry(GuiDialogs::DIALOG_FILECHOOSER, Conation::ARGTYPE_FILE, "Select new node binary", "Select a new node binary. It will be automatically rebranded to the appropriate ID(s).") } },
		{ CMDCODE_B2S_VAULT_ADD,DialogSpecStruct::FLAG_NONE,		 	{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter vault key", "Enter new vault item's key"),
																		  DialogEntry(GuiDialogs::DIALOG_FILECHOOSER, Conation::ARGTYPE_FILE, "Select file", "Select file to upload to vault") } },
		{ CMDCODE_B2S_VAULT_FETCH,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter vault key", "Enter vault item's key for download") } },
		{ CMDCODE_B2S_VAULT_UPDATE,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter vault key", "Enter vault item's key for update"),
																		  DialogEntry(GuiDialogs::DIALOG_FILECHOOSER, Conation::ARGTYPE_FILE, "Select file", "Select file to upload to vault") } },
		{ CMDCODE_B2S_VAULT_DROP,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter vault key", "Enter the key of the vault item to delete.") } },
		{ CMDCODE_A2S_VAULT_LIST,DialogSpecStruct::FLAG_NONE,			{ } },
		{ CMDCODE_A2S_SETSCONFIG,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter config key name", "Enter the name of the global configuration key you wish to change."),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter new value", "Enter a new value for the variable referenced by your key.") } },
		{ CMDCODE_A2S_GETSCONFIG,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter config key name", "Enter the name of the global configuration key you wish to read.") } },
		{ CMDCODE_A2S_DROPCONFIG,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter config key name", "Enter the name of the global configuration key you wish to delete.") } },
		{ CMDCODE_A2S_REMOVEUPDATE,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter platform string", "Enter the platform string for which to remove the update entry.") } },
		{ CMDCODE_A2S_FORGETNODE,DialogSpecStruct::FLAG_NONE,			{ } },
		{ CMDCODE_A2S_AUTHTOKEN_ADD,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter token", "Enter the new authorization token's string."),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_UINT32, "Enter permissions", "Enter decimal value for the permissions for this token.") } },
		{ CMDCODE_A2S_AUTHTOKEN_CHG,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter token", "Enter the authorization token's string."),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_UINT32, "Enter permissions", "Enter decimal value for the updated permissions for this token.") } },
		{ CMDCODE_A2S_AUTHTOKEN_REVOKE,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter token", "Enter the string of the authorization token to revoke."),
																		  DialogEntry(GuiDialogs::DIALOG_YESNODIALOG, Conation::ARGTYPE_BOOL, "Input needed", "Disconnect any nodes using this authorization token?") } },
		{ CMDCODE_A2S_AUTHTOKEN_LIST,DialogSpecStruct::FLAG_NONE,		{ } },
		{ CMDCODE_A2S_AUTHTOKEN_USEDBY,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter token", "Enter the string of an authorization token.") } },
		{ CMDCODE_A2S_SRVLOG_TAIL,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_UINT32, "Enter number of lines", "Enter the number of lines to fetch from the log.") } },
		{ CMDCODE_A2S_SRVLOG_SIZE,DialogSpecStruct::FLAG_NONE,			{ } },
		{ CMDCODE_A2S_SRVLOG_WIPE,DialogSpecStruct::FLAG_NONE,			{ } },
		{ CMDCODE_A2S_ROUTINE_ADD,DialogSpecStruct::FLAG_CONCATNT_COMMA,{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter name for routine", "Enter a name for this routine."),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter timing format.", "Enter pseudo-cron timing formatting for routine."),
																		  DialogEntry(GuiDialogs::DIALOG_BINARYFLAGS, Conation::ARGTYPE_UINT32, "Select flags", "Select the flags to apply to this routine.", 0, new std::vector<std::tuple<VLString, uint64_t, bool> > { std::tuple<VLString, uint64_t, bool>{ "Run once", 1, false }, std::tuple<VLString, uint64_t, bool>{ "On node connect", 1 << 1, false }, std::tuple<VLString, uint64_t, bool>{ "Disabled", 1 << 2, false } }),
																		  DialogEntry(GuiDialogs::DIALOG_ARGSELECTOR, Conation::ARGTYPE_ANY, "Build stream for routine", "Create a stream to send to the selected nodes.", ADF_CHG_CMDCODE | ADF_AS_BINSTREAM) } },
		{ CMDCODE_A2S_ROUTINE_DEL,DialogSpecStruct::FLAG_NONE,			{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter routine name", "Enter the name of the routine to delete.") } },
		{ CMDCODE_A2S_ROUTINE_LIST,DialogSpecStruct::FLAG_NONE,			{ } },
		{ CMDCODE_A2S_ROUTINE_CHG_TGT,DialogSpecStruct::FLAG_CONCATNT_COMMA,
																		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter routine name", "Enter the name of the routine to change targets for.") } },
		{ CMDCODE_A2S_ROUTINE_CHG_SCHD,DialogSpecStruct::FLAG_NONE, 	{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter routine name", "Enter the name of the routine to change the schedule for."),
																		  DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter schedule", "Enter the new schedule timing format.") } },
		{ CMDCODE_A2S_ROUTINE_CHG_FLAG,DialogSpecStruct::FLAG_NONE,		{ DialogEntry(GuiDialogs::DIALOG_SIMPLETEXT, Conation::ARGTYPE_STRING, "Enter routine name", "Enter the name of the routine to set flags for."),
																		  DialogEntry(GuiDialogs::DIALOG_BINARYFLAGS, Conation::ARGTYPE_UINT32, "Select flags", "Select the flags to apply to this routine.", 0, new std::vector<std::tuple<VLString, uint64_t, bool> > { std::tuple<VLString, uint64_t, bool>{ "Run once", 1, false }, std::tuple<VLString, uint64_t, bool>{ "On node connect", 1 << 1, false }, std::tuple<VLString, uint64_t, bool>{ "Disabled", 1 << 2, false } }) } },


	};


Orders::CurrentOrderStruct Orders::CurrentOrder;

//Prototypes
static const DialogSpecStruct *LookupCmdCodeDialogs(const CommandCode CmdCode);
static Conation::ArgType GetDialogArgType(const CommandCode CmdCode, const size_t DialogNumber);
static void GenericDialogCallback(const void *Dialog);

//Functions


static Conation::ArgType GetDialogArgType(const CommandCode CmdCode, const size_t DialogNumber)
{
	const DialogSpecStruct *Lookup = LookupCmdCodeDialogs(CmdCode);

	if (!Lookup) return Conation::ARGTYPE_NONE;

	if (Lookup->Dialogs.size() <= DialogNumber) return Conation::ARGTYPE_NONE;
	
	return Lookup->Dialogs[DialogNumber].SendAs;
}

static const DialogSpecStruct *LookupCmdCodeDialogs(const CommandCode CmdCode)
{
	for (size_t Inc = 0; Inc < sizeof DialogCommandMap / sizeof *DialogCommandMap; ++Inc)
	{
		if (DialogCommandMap[Inc].CmdCode == CmdCode) return &DialogCommandMap[Inc];
	}

	return nullptr;
}

		
static bool CompileCurrentOrderToStreams(const char *NodeID, Conation::ConationStream **Output)
{
	Conation::ConationStream *Stream = new Conation::ConationStream(Orders::CurrentOrder.CmdCode, false, Orders::CurrentOrder.CmdIdent);

	if (NodeID != nullptr)
	{ //This is an order for nodes
		Stream->Push_ODHeader("ADMIN", NodeID);
	}
	
	for (size_t Inc = 0u; Inc < Orders::CurrentOrder.Dialogs.size(); ++Inc)
	{
		GuiDialogs::DialogBase *Dialog = Orders::CurrentOrder.Dialogs[Inc];

		assert(Dialog != nullptr);
		
		switch (Dialog->GetDialogType())
		{
			case GuiDialogs::DIALOG_YESNODIALOG:
			{
				GuiDialogs::YesNoDialog *ChoiceDialog = static_cast<GuiDialogs::YesNoDialog*>(Dialog);
				
				if (GetDialogArgType(Orders::CurrentOrder.CmdCode, Inc) != Conation::ARGTYPE_BOOL) break; //Well fuck that.
				
				Stream->Push_Bool(ChoiceDialog->GetValue());
				break;
			}
			case GuiDialogs::DIALOG_BINARYFLAGS:
			{
				GuiDialogs::BinaryFlagsDialog *FlagsDialog = static_cast<GuiDialogs::BinaryFlagsDialog*>(Dialog);

				switch (GetDialogArgType(Orders::CurrentOrder.CmdCode, Inc))
				{
					case Conation::ARGTYPE_UINT32:
						Stream->Push_Uint32(FlagsDialog->GetFlagValues());
						break;
					case Conation::ARGTYPE_UINT64:
						Stream->Push_Uint64(FlagsDialog->GetFlagValues());
						break;
					default:
						assert(0); //Phucked
				}

				break;
			}	
			case GuiDialogs::DIALOG_ARGSELECTOR:
			{
				GuiDialogs::ArgSelectorDialog *ArgDialog = static_cast<GuiDialogs::ArgSelectorDialog*>(Dialog);
				const DialogSpecStruct *Specs = LookupCmdCodeDialogs(Orders::CurrentOrder.CmdCode);

				const uint64_t Flags = Specs->Dialogs[Inc].Integer;
				
				if ((Flags & ADF_CHG_CMDCODE) && !(Flags & ADF_AS_BINSTREAM))
				{
					Conation::ConationStream::StreamHeader &&Hdr = Stream->GetHeader();
					Hdr.CmdCode = ArgDialog->GetHeader().CmdCode;
					Stream->AlterHeader(Hdr);
				}

				if (Flags & ADF_WIPE_ARGS)
				{
					Stream->WipeArgs();
				}

				VLScopedPtr<Conation::ConationStream*> NewStream { ArgDialog->CompileToStream() };

				if (Flags & ADF_AS_BINSTREAM)
				{
					Stream->Push_BinStream(NewStream->GetData().data(), NewStream->GetData().size());
				}
				else
				{
					Stream->AppendArgData(*NewStream);
				}
				break;
			}
			case GuiDialogs::DIALOG_FILECHOOSER:
			{
				if (GetDialogArgType(Orders::CurrentOrder.CmdCode, Inc) != Conation::ARGTYPE_FILE) break; //WTF else could we even do with this?
				
				GuiDialogs::FileSelectionDialog *Selector = static_cast<GuiDialogs::FileSelectionDialog*>(Dialog);

				const std::vector<VLString> &Paths = Selector->GetPaths();

				for (size_t Inc = 0u; Inc < Paths.size(); ++Inc)
				{
					uint64_t Size{};
					
					if (!Utils::GetFileSize(Paths[Inc], &Size))
					{
						goto EasyError;
					}

					if ((Stream->GetStreamArgsSize() + Size) >= Stream->GetMaxStreamArgsSize())
					{
#ifdef DEBUG
						puts("CompileCurrentOrderToStreams(): Stream->GetStreamArgsSize() + Size >= MaxStreamArgsSize");
#endif //DEBUG
						goto EasyError;
					}
					
					Stream->Push_File(Paths[Inc]);
				}
				break;
			}
			case GuiDialogs::DIALOG_SIMPLETEXT:
			{
				const Conation::ArgType Type = GetDialogArgType(Orders::CurrentOrder.CmdCode, Inc);
				
				if (Type != Conation::ARGTYPE_STRING && Type != Conation::ARGTYPE_FILEPATH)
				{ //Gotta be an integer then, huh?
					const VLString String = static_cast<GuiDialogs::SimpleTextDialog*>(Dialog)->GetTextData();
					
					switch (Type)
					{
						case Conation::ARGTYPE_INT32:
							Stream->Push_Int32(VLString::StringToInt(String));
							break;
						case Conation::ARGTYPE_UINT32:
							Stream->Push_Uint32(VLString::StringToUint(String));
							break;
						case Conation::ARGTYPE_INT64:
							Stream->Push_Int64(VLString::StringToInt(String));
							break;
						case Conation::ARGTYPE_UINT64:
							Stream->Push_Uint64(VLString::StringToUint(String));
							break;
						default:
							break;
					}
					break;
				}
				
				///FALL THROUGH! This is not a typo!!!
			}
			case GuiDialogs::DIALOG_LARGETEXT:
			{
				void (Conation::ConationStream::*Ptr)(const char *String, const size_t ExtraSpaceAfter){};

				switch (GetDialogArgType(Orders::CurrentOrder.CmdCode, Inc))
				{
					case Conation::ARGTYPE_FILEPATH:
						Ptr = (decltype(Ptr))&Conation::ConationStream::Push_FilePath;
						break;
					case Conation::ARGTYPE_STRING:
						Ptr = (decltype(Ptr))&Conation::ConationStream::Push_String;
						break;
					default:
						break;
				}

				VLString TextData;

				switch (Dialog->GetDialogType())
				{
					case GuiDialogs::DIALOG_SIMPLETEXT:
						TextData = static_cast<GuiDialogs::SimpleTextDialog*>(Dialog)->GetTextData();
						break;
					case GuiDialogs::DIALOG_LARGETEXT:
						TextData = static_cast<GuiDialogs::LargeTextDialog*>(Dialog)->GetTextData();
						break;
					default:
						break;
				}
				const DialogSpecStruct *Specs = LookupCmdCodeDialogs(Orders::CurrentOrder.CmdCode);

				if (!Specs) break;

				if (Dialog->GetDialogType() == GuiDialogs::DIALOG_SIMPLETEXT
					|| Specs->Dialogs[Inc].Integer == 0ull)
				{ //We want all lines as a single string.
					(Stream->*Ptr)(TextData, 10);
					break;
				}

				//We want it as each line.
				std::vector<VLString> *Paths = Utils::SplitTextByCharacter(TextData, '\n');

				if (!Paths || Paths->empty())
				{
					delete Paths;
					break;
				}

				for (size_t Inc = 0u; Inc < Paths->size(); ++Inc)
				{
#ifdef DEBUG
					puts(VLString("CompileCurrentOrderToStreams(): Sending text line ") + Paths->at(Inc));
#endif
					(Stream->*Ptr)(Paths->at(Inc), 10);
				}

				delete Paths;
				break;
			}
			case GuiDialogs::DIALOG_NONE:
				break;
			default:
				break; //There's only so many of these dialogs we use for input.
		}
	}

	*Output = Stream; //We're giving it a pointer, don't delete Stream!

	return true;
	
EasyError:
	delete Stream;
	return false;
}

void Orders::CurrentOrderStruct::Clear(void)
{
	this->CmdCode = CMDCODE_INVALID;

	for (size_t Inc = 0u; Inc < this->Dialogs.size(); ++Inc)
	{
		delete this->Dialogs[Inc];
	}

	this->Dialogs.clear();
	this->DestinationNodes.clear();
}
	
bool Orders::CurrentOrderStruct::Finalize(void)
{ //What's called when all dialogs have been dismissed.
	std::vector<Conation::ConationStream*> Outputs;

	const DialogSpecStruct *Spec = LookupCmdCodeDialogs(this->CmdCode);

	//Might be a server order.
	Outputs.resize((this->DestinationNodes.size() && !(Spec->SpecFlags & DialogSpecStruct::FLAG_CONCATNT_COMMA)) ? this->DestinationNodes.size() : 1);

	//Create order streams for each node
	if (this->DestinationNodes.size() > 0)
	{

		if (Spec && (Spec->SpecFlags & DialogSpecStruct::FLAG_CONCATNT_COMMA))
		{
			CompileCurrentOrderToStreams(nullptr, &Outputs[0]);

			VLString Targets(4096);

			for (const VLString &Value : DestinationNodes)
			{
				Targets += Value + ',';
			}
			
			Targets.StripTrailing(", ");
			
			//Append list of targets to end.
			Outputs[0]->Push_String(Targets);
		}
		else
		{
			auto Iter = Outputs.begin();
			
			for (const VLString &Value : this->DestinationNodes)
			{
				CompileCurrentOrderToStreams(Value, &*Iter);
				++Iter;
			}
		}
	}
	else
	{
		CompileCurrentOrderToStreams(nullptr, &Outputs[0]);
	}
	
	//In some circumstances the result stream can differ from what we expected.
	const CommandCode RealCmdCode = Outputs[0]->GetCommandCode();
	
	//Transmit order streams and delete them afterwards.
	for (size_t Inc = 0u; Inc < Outputs.size(); ++Inc)
	{
#ifdef DEBUG
		printf("Orders::CurrentOrderStruct::Finalize(): Transmitting output stream %llu\n", (unsigned long long)Inc);
#endif
		Main::GetWriteQueue().Push(Outputs[Inc]);
	}

	//Add messages to ticker.
	if (GuiMenus::IsNodeOrder(this->CmdCode))
	{ //It's a bunch of nodes.
		for (const VLString &Value : this->DestinationNodes)
		{
			VLString Summary = VLString("Sent order of ") + CommandCodeToString(RealCmdCode) + ", awaiting response.";
			
			Ticker::AddNodeMessage(Value, this->CmdCode, this->CmdIdent, Summary);
		}
	}
	else
	{ //It's a server command.
		const size_t Count = this->DestinationNodes.size() ? this->DestinationNodes.size() : 1;
		
		for (size_t Inc = 0; Inc < Count; ++Inc)
		{
			VLString Summary = VLString("Sent order of ") + CommandCodeToString(RealCmdCode) + " to server, awaiting reponse.";
		
			Ticker::AddServerMessage(Summary);
		}
	}
		
	
	//Wipe the CurrentOrder structure.
	this->Clear();

	//Display the current window once more.
	static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW))->Set_Clickable(true);

	//Let 'Outputs' fall out of scope and deallocate itself.
	return true;
}

bool Orders::CurrentOrderStruct::Init(const CommandCode CmdCode, const std::set<VLString> *DestinationNodes)
{
	this->CmdCode = CmdCode;
	this->CmdIdent = Ticker::NewNodeMsgIdent();
	if (DestinationNodes) this->DestinationNodes = *DestinationNodes; //We're copying the whole vector element-for-element.

	const DialogSpecStruct *Lookup = LookupCmdCodeDialogs(this->CmdCode);

	if (!Lookup)
	{
		this->Clear();
		return false;
	}

	const std::vector<DialogEntry> &DialogInfo = Lookup->Dialogs;
	
	//Reserving the space in advance is very important for our empirical calculation of what a future element's address will be.
	//If we don't do this, then when the vector expands beyond its capacity, it could have a new address altogether, making us wrong.
	this->Dialogs.reserve(Lookup->Dialogs.size());
	
	for (size_t Inc = 0u; Inc < DialogInfo.size(); ++Inc)
	{
		const DialogEntry &Entry = DialogInfo[Inc];

		const void *const OurFutureAddress = this->Dialogs.data() + this->Dialogs.size();
		
		switch (DialogInfo[Inc].DialogType)
		{
			case GuiDialogs::DIALOG_FILECHOOSER:
			{
				this->Dialogs.push_back(new GuiDialogs::FileSelectionDialog(Entry.Title,
																			Entry.Prompt,
																			Entry.Integer,
																			(GuiDialogs::FileSelectionDialog::OnCompleteFunc)GenericDialogCallback,
																			OurFutureAddress));
				break;
			}
			case GuiDialogs::DIALOG_LARGETEXT:
			{
				this->Dialogs.push_back(new GuiDialogs::LargeTextDialog(Entry.Title, Entry.Prompt, (GuiDialogs::LargeTextDialog::OnCompleteFunc)GenericDialogCallback, OurFutureAddress));
				break;
			}
			case GuiDialogs::DIALOG_SIMPLETEXT:
			{
				this->Dialogs.push_back(new GuiDialogs::SimpleTextDialog(Entry.Title, Entry.Prompt, (GuiDialogs::SimpleTextDialog::OnCompleteFunc)GenericDialogCallback, OurFutureAddress));
				break;
			}
			case GuiDialogs::DIALOG_YESNODIALOG:
			{
				this->Dialogs.push_back(new GuiDialogs::YesNoDialog(Entry.Title, Entry.Prompt, false, (GuiDialogs::YesNoDialog::OnCompleteFunc)GenericDialogCallback, OurFutureAddress));
				break;
			}
			case GuiDialogs::DIALOG_ARGSELECTOR:
			{
				this->Dialogs.push_back(new GuiDialogs::ArgSelectorDialog(Entry.Title, Entry.Prompt, nullptr, nullptr, Conation::ARGTYPE_ANY, (GuiDialogs::ArgSelectorDialog::OnCompleteFunc)GenericDialogCallback, OurFutureAddress, DialogInfo[Inc].Integer & ADF_CHG_CMDCODE) );
				break;
			}
			case GuiDialogs::DIALOG_BINARYFLAGS:
			{
				assert(Entry.Ptr != nullptr); //Cuz we're gonna dereference it right below
				
				this->Dialogs.push_back(new GuiDialogs::BinaryFlagsDialog(Entry.Title, Entry.Prompt, *static_cast<std::vector<std::tuple<VLString, uint64_t, bool> >* >(Entry.Ptr), (GuiDialogs::BinaryFlagsDialog::OnCompleteFunc)GenericDialogCallback, OurFutureAddress));
				break;
			}
			default:
				break;
		}

	}
	
	if (this->Dialogs.size())
	{
		//Hide the main window while we wait for input.
		assert(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW) != nullptr);
		static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW))->Set_Clickable(false);
		
		this->Dialogs[0]->Show();
	}
	else //Doesn't require a dialog to transmit.
	{
		return this->Finalize();
	}
	
	return true;
}

static void GenericDialogCallback(const void *Dialog)
{
	const size_t DialogsAheadOfZero = static_cast<GuiDialogs::DialogBase *const*>(Dialog) - Orders::CurrentOrder.Dialogs.data();

#ifdef DEBUG
	printf("GenericDialogCallback(): DialogsAheadOfZero == %u\n", (unsigned)DialogsAheadOfZero);
#endif

	//Used to check for possible memory corruption.
	assert(DialogsAheadOfZero < Orders::CurrentOrder.Dialogs.size());
	
	if (DialogsAheadOfZero + 1 == Orders::CurrentOrder.Dialogs.size())
	{
		Orders::CurrentOrder.Finalize();
	}
	else
	{
		Orders::CurrentOrder.Dialogs[DialogsAheadOfZero + 1]->Show();
	}
	
}

static bool SlurpyScripty(const char *Path, VLString *Output)
{
	if (!Output) return false;
	
	uint64_t ScriptSize = 0;
	
	if (!Utils::GetFileSize(Path, &ScriptSize))
	{
		return false;
	}
	
	Output->Wipe(false);
	Output->ChangeCapacity(ScriptSize + 1);
	
	if (!Utils::Slurp(Path, Output->GetBuffer(), Output->GetCapacity()))
	{
		return false;
	}
	
	return true;
}

bool Orders::SendNodeScriptLoadOrder(const char *ScriptName, const std::set<VLString> *DestinationNodes)
{
	ScriptScanner::ScriptInfo *Info = ScriptScanner::GetKnownScripts().at(ScriptName);
	
	if (!Info)
	{
#ifdef DEBUG
		puts("Orders::SendNodeScriptLoadOrder(): Info is a null pointer!?!? How did this happen??");
#endif
		return false;
	}
	
	if (!DestinationNodes || !DestinationNodes->size())
	{
#ifdef DEBUG
		puts("Orders::SendNodeScriptLoadOrder(): DestinationNodes is empty or a null pointer!");
#endif	
		return false; //Wtf
	}
	VLString ScriptText;
	
	if (!SlurpyScripty(Info->ScriptPath, &ScriptText))
	{
#ifdef DEBUG
		puts(VLString() + "Orders::SendNodeScriptLoadOrder(): Failed to slurp script! Path is \"" + Info->ScriptPath + "\".");
#endif	
		return false;
	}
	
	const uint64_t Ident = Ticker::NewNodeMsgIdent();
	
	for (const VLString &Value : *DestinationNodes)
	{
		Conation::ConationStream *Output = new Conation::ConationStream(CMDCODE_A2C_MOD_LOADSCRIPT, 0, Ident);
		
		Output->Push_ODHeader("ADMIN", Value);
		Output->Push_String(ScriptName);
		Output->Push_Script(ScriptText);
		Output->Push_Bool(false);
				
		VLString Summary = VLString("Sent order of ") + CommandCodeToString(Output->GetCommandCode()) + ", awaiting response.";
			
		Ticker::AddNodeMessage(Value, Output->GetCommandCode(), Output->GetCmdIdentOnly(), Summary);
		
		Main::GetWriteQueue().Push(Output);

	}
	
	return true;
}

bool Orders::SendNodeScriptUnloadOrder(const char *ScriptName, const std::set<VLString> *DestinationNodes)
{
	ScriptScanner::ScriptInfo *Info = ScriptScanner::GetKnownScripts().at(ScriptName);
	
	if (!Info) return false;
	
	if (!DestinationNodes || !DestinationNodes->size()) return false; //Wtf
	
	const uint64_t Ident = Ticker::NewNodeMsgIdent();
	
	for (const VLString &Value : *DestinationNodes)
	{
		Conation::ConationStream *Output = new Conation::ConationStream(CMDCODE_A2C_MOD_UNLOADSCRIPT, 0, Ident);
		
		Output->Push_ODHeader("ADMIN", Value);
		Output->Push_String(ScriptName);
		
		VLString Summary = VLString("Sent order of ") + CommandCodeToString(Output->GetCommandCode()) + ", awaiting response.";
			
		Ticker::AddNodeMessage(Value, Output->GetCommandCode(), Output->GetCmdIdentOnly(), Summary);
		
		Main::GetWriteQueue().Push(Output);
	}
	
	return true;
}

bool Orders::SendNodeScriptReloadOrder(const char *ScriptName, const std::set<VLString> *DestinationNodes)
{
	ScriptScanner::ScriptInfo *Info = ScriptScanner::GetKnownScripts().at(ScriptName);
	
	if (!Info) return false;
	
	if (!DestinationNodes || !DestinationNodes->size()) return false; //Wtf
	
	VLString ScriptText;
	
	if (!SlurpyScripty(Info->ScriptPath, &ScriptText))
	{
		return false;
	}
	
	
	const uint64_t Ident = Ticker::NewNodeMsgIdent();
	
	for (const VLString &Value : *DestinationNodes)
	{
		Conation::ConationStream *Output = new Conation::ConationStream(CMDCODE_A2C_MOD_LOADSCRIPT, 0, Ident);
		
		Output->Push_ODHeader("ADMIN", Value);
		Output->Push_String(ScriptName);
		Output->Push_Script(ScriptText);
		Output->Push_Bool(true);
		
		VLString Summary = VLString("Sent order of ") + CommandCodeToString(Output->GetCommandCode()) + ", awaiting response.";
			
		Ticker::AddNodeMessage(Value, Output->GetCommandCode(), Output->GetCmdIdentOnly(), Summary);
		
		Main::GetWriteQueue().Push(Output);
	}
	
	return true;
}

static void ScriptDialogDoneCallback(void **Stuff, const GuiDialogs::ArgSelectorDialog *Dialog)
{
	Conation::ConationStream::StreamHeader &&Hdr = Dialog->GetHeader();
	Hdr.StreamArgsSize = 0;
	
	const ScriptScanner::ScriptInfo::ScriptFunctionInfo *FuncInfo = static_cast<ScriptScanner::ScriptInfo::ScriptFunctionInfo*>(*Stuff);
	VLScopedPtr<const std::vector<VLString>*> DestinationNodes = static_cast<std::vector<VLString>*>(Stuff[1]);
	
	delete[] Stuff;
	
	VLScopedPtr<std::vector<Conation::ConationStream::BaseArg*>*> Values = Dialog->GetValues();

	for (size_t Inc = 0; Inc < DestinationNodes->size(); ++Inc)
	{
		Conation::ConationStream *Stream = new Conation::ConationStream(Hdr, nullptr);
		
		Stream->Push_ODHeader("ADMIN", DestinationNodes->at(Inc));
		Stream->Push_String(FuncInfo->Parent->ScriptName);
		Stream->Push_String(FuncInfo->Name);
		
		for (size_t Inc = 0; Inc < Values->size(); ++Inc)
		{
			Stream->PushArgument(Values->at(Inc));
		}
		
		VLString Summary = VLString("Sent order of ") + CommandCodeToString(Hdr.CmdCode) + ", awaiting response.";
			
		Ticker::AddNodeMessage(DestinationNodes->at(Inc), Hdr.CmdCode, Hdr.CmdIdent, Summary);
		
		Main::GetWriteQueue().Push(Stream);
	}

	if (GuiMainWindow::MainWindowScreen *Scr = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW)))
	{
		Scr->Set_Clickable(true);
	}

	delete Dialog->GetOriginalStream();
}

bool Orders::SendNodeScriptFuncOrder(ScriptScanner::ScriptInfo::ScriptFunctionInfo *FuncInfo, const std::set<VLString> *DestinationNodes)
{
	
	Conation::ConationStream *Stream = new Conation::ConationStream(CMDCODE_A2C_MOD_EXECFUNC, 0, Ticker::NewNodeMsgIdent());
	
	GuiDialogs::ArgSelectorDialog *Dialog = new GuiDialogs::ArgSelectorDialog("Choose arguments for function call",
																			"Select the arguments for the script function call.",
																			Stream,
																			&FuncInfo->RequiredParameters,
																			FuncInfo->AllowedExtraParameters,
																			(GuiDialogs::ArgSelectorDialog::OnCompleteFunc)ScriptDialogDoneCallback,
																			new void*[2]{ FuncInfo, (void*)DestinationNodes });
	Dialog->Show();

	if (GuiMainWindow::MainWindowScreen *Scr = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW)))
	{
		Scr->Set_Clickable(false);
	}
	
	return true;
}
