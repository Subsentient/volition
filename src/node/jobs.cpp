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

#include "../libvolition/include/conation.h"
#include "../libvolition/include/utils.h"
#include "../libvolition/include/vlthreads.h"


#include "jobs.h"
#include "files.h"
#include "processes.h"
#include "identity_module.h"
#include "main.h"
#include "script.h"

#include <list>

#define ADMIN_STR "ADMIN"
///Types

struct JobFuncLookupStruct
{
	const CommandCode ID;
	void *(*const Function)(Jobs::Job*);
};

static struct JobWorkingDirectoryStruct
{
	VLString Path;
	VLThreads::Mutex Mutex;
	operator VLString (void) { return this->Path; }
	JobWorkingDirectoryStruct(void);
} JobWorkingDirectory;

///Prototypes

static void *(*LookupJobFunction(const CommandCode ID))(void*);
static void InitJobEnv(void);

static void *StartupScriptFunc(Jobs::Job *OurJob);
static void *JOB_CHDIR_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_FILES_MOVE_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_FILES_PLACE_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_FILES_FETCH_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_FILES_COPY_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_FILES_DEL_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_EXECCMD_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_EXECSNIPPET_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_GETCWD_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_GETPROCESSES_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_KILLPROCESS_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_MOD_LOADSCRIPT_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_MOD_UNLOADSCRIPT_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_MOD_EXECFUNC_ThreadFunc(Jobs::Job *OurJob);
static void *JOB_LISTDIRECTORY_ThreadFunc(Jobs::Job *OurJob);

///Globals
static uint64_t JobIDCounter;
static std::list<Jobs::Job> JobsList;
static JobFuncLookupStruct JobFunctions[] = {
												{ CMDCODE_INVALID, StartupScriptFunc },
												{ CMDCODE_A2C_CHDIR, JOB_CHDIR_ThreadFunc },
												{ CMDCODE_A2C_GETCWD, JOB_GETCWD_ThreadFunc},
												{ CMDCODE_A2C_GETPROCESSES, JOB_GETPROCESSES_ThreadFunc },
												{ CMDCODE_A2C_KILLPROCESS, JOB_KILLPROCESS_ThreadFunc },
												{ CMDCODE_A2C_EXECCMD, JOB_EXECCMD_ThreadFunc },
												{ CMDCODE_A2C_EXECSNIPPET, JOB_EXECSNIPPET_ThreadFunc },
												{ CMDCODE_A2C_FILES_DEL, JOB_FILES_DEL_ThreadFunc },
												{ CMDCODE_A2C_FILES_PLACE, JOB_FILES_PLACE_ThreadFunc },
												{ CMDCODE_A2C_FILES_FETCH, JOB_FILES_FETCH_ThreadFunc },
												{ CMDCODE_A2C_FILES_COPY, JOB_FILES_COPY_ThreadFunc },
												{ CMDCODE_A2C_FILES_MOVE, JOB_FILES_MOVE_ThreadFunc },
												{ CMDCODE_A2C_MOD_EXECFUNC, JOB_MOD_EXECFUNC_ThreadFunc },
												{ CMDCODE_A2C_MOD_LOADSCRIPT, JOB_MOD_LOADSCRIPT_ThreadFunc },
												{ CMDCODE_A2C_MOD_UNLOADSCRIPT, JOB_MOD_UNLOADSCRIPT_ThreadFunc },
												{ CMDCODE_A2C_LISTDIRECTORY, JOB_LISTDIRECTORY_ThreadFunc },
											};

///Function definitions

JobWorkingDirectoryStruct::JobWorkingDirectoryStruct(void) : Path(Files::GetWorkingDirectory()) {}

static void *(*LookupJobFunction(const CommandCode ID))(void*)
{
	for (size_t Inc = 0; Inc < sizeof JobFunctions / sizeof *JobFunctions; ++Inc)
	{
		//Type punning because fuck you.
		if (JobFunctions[Inc].ID == ID) return (void*(*)(void*))JobFunctions[Inc].Function;
	}

	return nullptr;
}

static void *StartupScriptFunc(Jobs::Job *OurJob)
{ //Special job thread function just for startup scripts.
	InitJobEnv();
	
	try
	{
		Script::ExecuteStartupScript(OurJob);
	}
	catch (Script::ScriptError &Err)
	{
		VLWARN("Failed to execute startup script, got error" + Err.Msg);
	}

	return nullptr;
}

static void *JOB_FILES_MOVE_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
	
	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_FILEPATH, Conation::ARGTYPE_FILEPATH}))
	{
		Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
		Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}

	Stream->Pop_ODHeader(); //We don't give a fuck.

	const VLString &Source = Stream->Pop_FilePath();
	const VLString &Destination = Stream->Pop_FilePath();
	
	const NetCmdStatus &RetVal = Files::Move(Source, Destination);

	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_NetCmdStatus(RetVal);

	Main::PushStreamToWriteQueue(Response);
	return nullptr;
}

static void *JOB_LISTDIRECTORY_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();

	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };
	
	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);

	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_FILEPATH}))
	{
		Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
		Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}

	Stream->Pop_ODHeader(); //Derp

	const VLString &Path = Stream->Pop_FilePath();
	
	VLScopedPtr<std::list<Files::DirectoryEntry>*> Results = Files::ListDirectory(Path);

	if (!Results)
	{
		Response->Push_NetCmdStatus(false);
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}

	VLString RetVal = VLString("Listing for path \"") + Path + (const char*)"\":" + '\n';

	for (auto &Entry : *Results)
	{
		RetVal += Entry.IsDirectory ? "d " : "f ";
		RetVal += Entry.Path;
		RetVal += '\n';
	}

	Response->Push_String(RetVal);

	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}

static void *JOB_EXECSNIPPET_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	Conation::ConationStream *Stream = OurJob->Read_Queue.Pop(false); //Cuz the snippet might want this

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream Response(OurJob->CmdCode, Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, OurJob->CmdIdent);
	Response.Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);

	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_STRING}))
	{
		Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		
		Main::PushStreamToWriteQueue(Response);
		
		return nullptr;
	}
	
	Stream->Pop_ODHeader();
	
	const VLString ScriptData { Stream->Pop_String() };
	
	if (!Script::LoadScript(ScriptData, "VLTMPSNIPPET", true))
	{
		Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_IERR));
		Main::PushStreamToWriteQueue(Response);
		
		return nullptr;
	}
	
	try
	{
		if (!Script::ExecuteScriptFunction("VLTMPSNIPPET", nullptr, nullptr, OurJob))
		{
			Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_FAILED, "Generic failure with no details"));
			Main::PushStreamToWriteQueue(Response);
			return nullptr;
		}
	}
	catch (const Script::ScriptError &Err)
	{
		Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_FAILED, Err.Msg));
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	
	Response.Push_NetCmdStatus(true);
	Main::PushStreamToWriteQueue(Response);
	return nullptr;
}
	
static void *JOB_MOD_EXECFUNC_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	Conation::ConationStream *Stream = OurJob->Read_Queue.Pop(false); //Cuz the scripts might want this

	//Make sure nothing's fucky.
	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);
	
	Conation::ConationStream Response(OurJob->CmdCode, Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, OurJob->CmdIdent);
	Response.Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);

	VLScopedPtr<std::vector<Conation::ArgType>*> ArgTypes { Stream->GetArgTypes() };
	
	if (ArgTypes->at(0) != Conation::ARGTYPE_ODHEADER ||
		ArgTypes->at(1) != Conation::ARGTYPE_STRING ||
		ArgTypes->at(2) != Conation::ARGTYPE_STRING)
	{
		Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		
		Main::PushStreamToWriteQueue(Response);
		
		return nullptr;
	}
	
	Stream->Pop_ODHeader(); //We don't care, just get rid of it.
	
	const VLString &ScriptName = Stream->Pop_String();
	const VLString &FuncName = Stream->Pop_String();
	
	bool Result = false;

#ifdef DEBUG
	printf("JOB_MOD_EXECFUNC_ThreadFunc(): Attempting to execute job %s::%s\n", +ScriptName, +FuncName);
#endif

	try
	{
		Result = Script::ExecuteScriptFunction(ScriptName, FuncName, Stream, OurJob);
	}
	catch (Script::ScriptError &Err)
	{
		Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_IERR, Err.Msg));
		
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	
	Response.Push_NetCmdStatus(Result);
	
	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}

static void *JOB_MOD_LOADSCRIPT_ThreadFunc(Jobs::Job *OurJob)
{
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);
	
	Conation::ConationStream Response(OurJob->CmdCode, Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, OurJob->CmdIdent);
	
	Response.Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	
	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER,
								Conation::ARGTYPE_STRING,
								Conation::ARGTYPE_SCRIPT,
								Conation::ARGTYPE_BOOL}))
	{
		Response.Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	
	Stream->Pop_ODHeader(); //Useless to us.
	
	const VLString ScriptName = Stream->Pop_String();
	const VLString ScriptText = Stream->Pop_Script();
	const bool OverwritePermissible = Stream->Pop_Bool();
	
	if (!OverwritePermissible && Script::ScriptIsLoaded(ScriptName)) //We're trying to reload it, but it's already loaded!
	{
#ifdef DEBUG
		puts(VLString("Script \"") + ScriptName + "\" already loaded. Aborting.");
#endif
		Response.Push_NetCmdStatus(false);
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	
	const bool Result = Script::LoadScript(ScriptText, ScriptName, OverwritePermissible);
	
	Response.Push_NetCmdStatus(Result);
	
#ifdef DEBUG
	puts(VLString(OverwritePermissible ? "Reload" : "Load") + " of script \"" + ScriptName + "\" " + (Result ? "succeeded" : "failed"));
#endif
	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}

static void *JOB_MOD_UNLOADSCRIPT_ThreadFunc(Jobs::Job *OurJob)
{
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };
	
	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);
	
	Conation::ConationStream Response(OurJob->CmdCode, Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, OurJob->CmdIdent);
	
	Response.Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	
	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER,
								Conation::ARGTYPE_STRING}))
	{
		Response.Push_NetCmdStatus(false, STATUS_MISUSED);
		
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	
	Stream->Pop_ODHeader();
	
	const VLString &ScriptName = Stream->Pop_String();
	const bool Result = Script::UnloadScript(ScriptName);
	
#ifdef DEBUG
	puts(VLString("Unload of script \"") + ScriptName + "\" " + (Result ? "succeeded" : "failed"));
#endif

	Response.Push_NetCmdStatus({Result});
	
	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}

static void *JOB_FILES_FETCH_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };
	
	
	printf("Composite: %s\nJob version: %s\n",
		+Utils::ToBinaryString((Stream->GetCmdIdentComposite() & ~Conation::IDENT_ISREPORT_BIT)),
		+Utils::ToBinaryString(OurJob->CmdIdent));
		
	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());
	
	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);

	if (!Stream->VerifyArgTypePattern(1, {Conation::ARGTYPE_FILEPATH}))
	{
		Response->Push_NetCmdStatus({false, STATUS_MISUSED});
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	
	Stream->Pop_ODHeader(); //Useless to us.
	const size_t NumPaths = Stream->CountArguments() - 1;
	
	bool OneFailed = false;
	bool OneSucceeded = false;
	
	for (size_t Inc = 0; Inc < NumPaths; ++Inc)
	{
		const VLString &Path = Stream->Pop_FilePath();
		
		if (!Utils::FileExists(Path) || Utils::IsDirectory(Path))
		{
			OneFailed = true;
			continue;
		}
		
		Response->Push_File(Path);
		OneSucceeded = true;
	}
	
	Response->Push_NetCmdStatus({OneSucceeded, OneFailed ? STATUS_WARN : STATUS_OK});
	
	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}

static void *JOB_FILES_PLACE_ThreadFunc(Jobs::Job *OurJob)
{ /*Would it just have been easier to put the file path argument first?
	* Sure, but it'd make me reorder the dialogs in the control program, so fuck you, have some complicated code.
	* Aesthetics matter. */
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

	goto Safe;

BadArgs:
	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
	Main::PushStreamToWriteQueue(Response);
	return nullptr;
	
Safe:
	Stream->Pop_ODHeader(); //We don't give a fuck.


	VLScopedPtr<std::vector<Conation::ArgType>* > ArgTypes { Stream->GetArgTypes() };
	std::vector<Conation::ConationStream::FileArg> FilesReceived;
	
	VLString OutputDirectory;
	
	for (size_t Inc = 1u; Inc < ArgTypes->size(); ++Inc)
	{
		switch (ArgTypes->at(Inc))
		{
			case Conation::ARGTYPE_FILE:
			{
				if (Inc == ArgTypes->size() - 1)
				{
					goto BadArgs;
				}
				
				FilesReceived.push_back(Conation::ConationStream::FileArg());
				FilesReceived.back() = Stream->Pop_File();
				break;
			}
			case Conation::ARGTYPE_FILEPATH:
			{
				if (Inc != ArgTypes->size() - 1)
				{
					goto BadArgs;
				}
				
				OutputDirectory = Stream->Pop_FilePath();
				break;
			}
			default:
				goto BadArgs;
				break;
		}
	}
	
	
	if (!OutputDirectory || FilesReceived.empty())
	{
		goto BadArgs;
	}
	
	
	bool AllSucceeded = true;
	bool OneSucceeded = false;
	for (size_t Inc = 0u; Inc < FilesReceived.size(); ++Inc)
	{
		const bool Result = Utils::WriteFile(OutputDirectory + FilesReceived[Inc].Filename, FilesReceived[Inc].Data, FilesReceived[Inc].DataSize);
		
		if (!Result) AllSucceeded = false;
		else OneSucceeded = true;
	}
	
	
	NetCmdStatus Result(OneSucceeded, OneSucceeded ? (AllSucceeded ? STATUS_OK : STATUS_WARN) : STATUS_FAILED);
	
	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_NetCmdStatus(Result);
	Main::PushStreamToWriteQueue(Response);

	return nullptr;
}
static void *JOB_FILES_COPY_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();

	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_FILEPATH, Conation::ARGTYPE_FILEPATH}))
	{
		Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
		Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	Stream->Pop_ODHeader(); //We don't give a fuck.

	const VLString &Source = Stream->Pop_FilePath();
	const VLString &Destination = Stream->Pop_FilePath();
	
	const NetCmdStatus &Result = Files::Copy(Source, Destination);

	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_NetCmdStatus(Result);
	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}
static void *JOB_FILES_DEL_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };
	
	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

	VLScopedPtr<std::vector<Conation::ArgType>* >ArgTypes { Stream->GetArgTypes() };

	for (size_t Inc = 1; Inc < ArgTypes->size(); ++Inc)
	{
		if (ArgTypes->at(Inc) != Conation::ARGTYPE_FILEPATH)
		{
			Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
			Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
			Main::PushStreamToWriteQueue(Response);
			return nullptr;
		}
	}

	const size_t NumArgs = ArgTypes->size();

	bool SucceededOnce = false;
	bool SucceededAll = true;

	Stream->Pop_ODHeader(); //We don't give a fuck.

	for (size_t Inc = 0; Inc < NumArgs - 1; ++Inc)
	{
		if (!Files::Delete(Stream->Pop_FilePath()))
		{
			SucceededAll = false;
		}
		else SucceededOnce = true;
	}

	NetCmdStatus RetVal = SucceededAll ? NetCmdStatus(true) : (SucceededOnce ? NetCmdStatus(true, STATUS_WARN) : NetCmdStatus(false, STATUS_FAILED));

	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_NetCmdStatus(RetVal);
	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}
static void *JOB_EXECCMD_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_STRING}))
	{
		Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
		Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));

		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}
	Stream->Pop_ODHeader(); //We don't give a fuck.

	VLString CmdOutput;
	
	const NetCmdStatus &RetVal = Processes::ExecuteCmd(Stream->Pop_String(), CmdOutput);

	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	if (CmdOutput) Response->Push_String(CmdOutput);
	Response->Push_NetCmdStatus(RetVal);

	Main::PushStreamToWriteQueue(Response);
	return nullptr;
}
static void *JOB_GETCWD_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER}))
	{
		Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
		Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		
		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}

	Stream->Pop_ODHeader(); //We don't give a fuck.


	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_FilePath(Files::GetWorkingDirectory());

	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}
static void *JOB_GETPROCESSES_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly());

	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_BOOL}))
	{
		Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
		Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		
		Main::PushStreamToWriteQueue(Response);

		return nullptr;
	}

	Stream->Pop_ODHeader();
	
	const bool FilterKernelProcesses = !Stream->Pop_Bool();
	
	std::list<Processes::ProcessListMember> List;

	const NetCmdStatus &Status = Processes::GetProcessesList(&List);

	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);

	if (!Status)
	{
		Response->Push_NetCmdStatus(Status);

		Main::PushStreamToWriteQueue(Response);
		return nullptr;
	}

	size_t ToReserve = 0;
	for (auto Iter = List.begin(); Iter != List.end(); ++Iter)
	{
		ToReserve += Iter->ProcessName.size();
		ToReserve += Iter->User.size();
		ToReserve += 64;
	}

	VLString OutputString;
	OutputString.reserve(ToReserve);
	
	for (auto Iter = List.begin(); Iter != List.end(); ++Iter)
	{
		if (Iter->KernelProcess && FilterKernelProcesses) continue;
		
		char StringPID[128];
		snprintf(StringPID, sizeof StringPID, "%lld", (long long)Iter->PID);
		
		OutputString += VLString("PID: ") + (const char*)StringPID + " || ";
		OutputString += VLString("Name: ") + (Iter->KernelProcess ? VLString("K:[") + Iter->ProcessName + "]" : Iter->ProcessName) + " || ";
		OutputString += VLString("User: ") + Iter->User;
		OutputString += "\n";
	}

	Response->Push_String(OutputString);
	Main::PushStreamToWriteQueue(Response);
	return nullptr;
}
static void *JOB_KILLPROCESS_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetHeader().CmdIdent);

	VLScopedPtr<std::vector<Conation::ArgType>*>Args { Stream->GetArgTypes() };

	size_t Inc = 1;
	
	if (Args->size() <= 1) goto Borked;
	
	for (; Inc < Args->size(); ++Inc)
	{ //We only accept strings.
		if (Args->at(Inc) != Conation::ARGTYPE_STRING)
		{
		Borked:
			Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
			Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));

			Main::PushStreamToWriteQueue(Response);


			return nullptr;
		}
	}

	const size_t ArgCount = Args->size();

	Stream->Pop_ODHeader();
	
	NetCmdStatus RetVal = true;

	bool OneFailed{};
	bool OneSucceeded{};
	
	for (size_t Inc = 0; Inc < ArgCount - 1; ++Inc)
	{
		const VLString &ProcessName = Stream->Pop_String();
#ifdef DEBUG
		puts(VLString("JOB_KILLPROCESS_ThreadFunc(): Killing process \"") + ProcessName + "\".");
#endif
		if (!Processes::KillProcessByName(ProcessName)) OneFailed = true;
		else OneSucceeded = true;
#ifdef DEBUG
		puts(VLString("JOB_KILLPROCESS_ThreadFunc(): Killed process \"") + ProcessName + "\".");
#endif
	}

	if (OneSucceeded)
	{
		if (OneFailed)
		{
			RetVal = NetCmdStatus(true, STATUS_WARN);
		}
		else
		{
			RetVal = true;
		}
	}
	else RetVal = NetCmdStatus(false, STATUS_FAILED);

	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_NetCmdStatus(RetVal);
	
	Main::PushStreamToWriteQueue(Response);
	
	return nullptr;
}
static void *JOB_CHDIR_ThreadFunc(Jobs::Job *OurJob)
{
	InitJobEnv();

	//We need this until we're done with this job. Everything else must wait.
	VLThreads::MutexKeeper Keeper { &JobWorkingDirectory.Mutex };
	
	VLScopedPtr<Conation::ConationStream*> Stream { OurJob->Read_Queue.Pop() };

	VLASSERT(Conation::BuildIdentComposite(Conation::GetIdentFlags(Stream->GetCmdIdentComposite()) & ~Conation::IDENT_ISREPORT_BIT, Stream->GetCmdIdentOnly()) == OurJob->CmdIdent && Stream->GetCommandCode() == OurJob->CmdCode);

	Conation::ConationStream *Response = new Conation::ConationStream(Stream->GetCommandCode(), Conation::GetIdentFlags(OurJob->CmdIdent) | Conation::IDENT_ISREPORT_BIT, Stream->GetHeader().CmdIdent);

	if (!Stream->VerifyArgTypes({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_FILEPATH}))
	{
		Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
		Response->Push_NetCmdStatus(NetCmdStatus(false, STATUS_MISUSED));
		
		Main::PushStreamToWriteQueue(Response);

		return nullptr;
	}

	Stream->Pop_ODHeader();
	
	const VLString &Target = Stream->Pop_FilePath();

	NetCmdStatus Code = Files::Chdir(Target);

	JobWorkingDirectory.Path = Target;

	VLDEBUG("Altering future job's working directories to " + JobWorkingDirectory.Path);


	Response->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	Response->Push_NetCmdStatus(Code);

	Main::PushStreamToWriteQueue(Response);

	return nullptr;
}
	
bool Jobs::StartJob(const CommandCode ID, const Conation::ConationStream *Data)
{
	//Create job object
	JobsList.emplace_back();

	//Get reference to it
	Job &New = JobsList.back();
	
	New.JobID = ++JobIDCounter; //Unsigned, doesn't matter if it overflows. First is one.

	if (ID != CMDCODE_INVALID && Data != nullptr)
	{ //If these arguments aren't specified, we're firing a raw init script.
		const Conation::ConationStream::StreamHeader &Hdr = Data->GetHeader();
	
		//Should never be a report.
		VLASSERT((Hdr.CmdIdentFlags & Conation::IDENT_ISREPORT_BIT) == 0);
		
		New.CmdIdent = Conation::BuildIdentComposite(Hdr.CmdIdentFlags, Hdr.CmdIdent);
		
		New.CmdCode = Hdr.CmdCode;
		New.Read_Queue.Push(*Data); //Copy it.
	}

	//Get function we use as thread entry point.
	auto FuncPtr = LookupJobFunction(ID);

	//Begin execution.
	New.JobThread = new VLThreads::Thread(FuncPtr, &New);
	New.JobThread->Start();
	
	return true;
	
}

void Jobs::ProcessCompletedJobs(void)
{
	std::list<Job>::iterator Iter = JobsList.begin();

	for (; Iter != JobsList.end(); ++Iter)
	{
		Job *CurJob = &*Iter;

		if (CurJob->JobThread->Alive()) continue; ///Still alive, do nothing.

		//Job's dead. Deal with it.

		//Get rid of this job now that we're done with it.
		auto IterNext = Iter;
		++IterNext;
		JobsList.erase(Iter);
		Iter = IterNext;

		VLDEBUG("Job completed.");
	}
}

static void InitJobEnv(void)
{
	const VLThreads::MutexKeeper Keeper { &JobWorkingDirectory.Mutex };

	Files::Chdir(JobWorkingDirectory.Path);

	VLDEBUG("Changed working directory to " + JobWorkingDirectory.Path);
}

VLString Jobs::GetWorkingDirectory(void)
{ //Returns the working directory that jobs use, which might be different from the main thread's working directory.
	const VLThreads::MutexKeeper Keeper { &JobWorkingDirectory.Mutex };
	
	return JobWorkingDirectory;
}


void Jobs::ForwardN2N(Conation::ConationStream *const Stream)
{
	bool HasJobIDs = false;
	
	if ( !(HasJobIDs = Stream->VerifyArgTypesStartWith({Conation::ARGTYPE_ODHEADER, Conation::ARGTYPE_UINT64, Conation::ARGTYPE_UINT64})) &&
		!Stream->VerifyArgTypesStartWith({Conation::ARGTYPE_ODHEADER}))
	{ //Origin-destination job IDs as well as node IDs.
		VLWARN("Invalid N2N message received!");
		return;
	}
	
	const Conation::ConationStream::ODHeader &ODObj { Stream->Pop_ODHeader() };
	const uint64_t OriginJob = HasJobIDs ? Stream->Pop_Uint64() : 0ul;
	const uint64_t DestinationJob = HasJobIDs ? Stream->Pop_Uint64() : 0ul;
	
	(void)OriginJob; //Not really useful atm.
	
	Stream->Rewind();
	
	for (Job &Ref : JobsList)
	{
		if (DestinationJob && Ref.JobID != DestinationJob) continue;
		
		if (!Ref.ReceiveN2N) continue; //Must be explicitly enabled, probably by a script job
		
		Ref.N2N_Queue.Push(*Stream);
	}
}

void Jobs::ForwardToScriptJobs(Conation::ConationStream *const Stream)
{
	for (auto Iter = JobsList.begin(); Iter != JobsList.end(); ++Iter)
	{
		if (Iter->CaptureIncomingStreams) //Script jobs should be given access to EVERYTHING that comes from the server.
		{
			Iter->Read_Queue.Push(*Stream);
		}
	}
}

void Jobs::KillAllJobs(void)
{
	for (auto Iter = JobsList.begin(); Iter != JobsList.end(); ++Iter)
	{
		if (Iter->JobThread)
		{
			Iter->JobThread->Kill();
			Iter->JobThread->Join();
		}
	}
	
	JobsList.clear();
}

bool Jobs::KillJobByID(const uint64_t JobID)
{
	for (auto Iter = JobsList.begin(); Iter != JobsList.end(); ++Iter)
	{
		if (Iter->JobID == JobID)
		{
			Iter->JobThread->Kill();
			Iter->JobThread->Join();
			JobsList.erase(Iter);
			return true;
		}
	}
	return false;
}

bool Jobs::KillJobByCmdCode(const CommandCode CmdCode)
{
	bool FoundOne = false;
ResetLoop:
	for (auto Iter = JobsList.begin(); Iter != JobsList.end(); ++Iter)
	{
		if (Iter->CmdCode == CmdCode)
		{
			Iter->JobThread->Kill();
			Iter->JobThread->Join();
			JobsList.erase(Iter);
			FoundOne = true;
			goto ResetLoop;
		}
	}
	
	return FoundOne;
}

Conation::ConationStream *Jobs::BuildRunningJobsReport(const Conation::ConationStream::StreamHeader &Hdr)
{
	VLDEBUG("Executing");
	
	Conation::ConationStream *RetVal = new Conation::ConationStream(Hdr, nullptr);
	
	RetVal->Push_ODHeader(IdentityModule::GetNodeIdentity(), ADMIN_STR);
	
	for (auto Iter = JobsList.begin(); Iter != JobsList.end(); ++Iter)
	{
		RetVal->Push_Uint64(Iter->JobID);
		RetVal->Push_Uint32(Iter->CmdCode);
		RetVal->Push_Uint64(Iter->CmdIdent);
	}
	
	return RetVal;
}

