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

#include "include/commandcodes.h"
#include "include/common.h"
#include "include/conation.h"

static struct
{
	const CommandCode CmdCode;
	const char *const Text;
} CommandCodeNames[] =	{
							{ TOKEN_KEYVALPAIR(CMDCODE_INVALID) },
							{ TOKEN_KEYVALPAIR(CMDCODE_ANY_NOOP) },
							{ TOKEN_KEYVALPAIR(CMDCODE_ANY_ECHO) },
							{ TOKEN_KEYVALPAIR(CMDCODE_ANY_PING) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2S_AUTH) },
							{ TOKEN_KEYVALPAIR(CMDCODE_ANY_DISCONNECT) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_SLEEP) },
							{ TOKEN_KEYVALPAIR(CMDCODE_S2C_KILL) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_INFO) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_INDEXDL) },
							{ TOKEN_KEYVALPAIR(CMDCODE_S2A_ADMINDEAUTH) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_CHDIR) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_GETCWD) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_GETPROCESSES) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_KILLPROCESS) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_EXECCMD) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_FILES_DEL) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_FILES_PLACE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_FILES_FETCH) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_FILES_COPY) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_FILES_MOVE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_S2A_NOTIFY_NODECHG) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2C_USEUPDATE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_PROVIDEUPDATE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_CHGNODEGROUP) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_WEB_FETCH) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_MOD_LOADSCRIPT) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_MOD_UNLOADSCRIPT) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_MOD_EXECFUNC) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_MOD_GENRESP) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2C_GETJOBSLIST) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2C_KILLJOBID) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2C_KILLJOBBYCMDCODE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_SETSCONFIG) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_GETSCONFIG) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_DROPCONFIG) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2S_VAULT_ADD) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2S_VAULT_FETCH) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2S_VAULT_UPDATE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_B2S_VAULT_DROP) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_VAULT_LIST) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_FORGETNODE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_AUTHTOKEN_ADD) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_AUTHTOKEN_CHG) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_AUTHTOKEN_REVOKE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_AUTHTOKEN_LIST) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_AUTHTOKEN_USEDBY) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_REMOVEUPDATE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_SRVLOG_TAIL) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_SRVLOG_WIPE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_SRVLOG_SIZE) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_ROUTINE_ADD) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_ROUTINE_DEL) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_ROUTINE_LIST) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_ROUTINE_CHG_TGT) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_ROUTINE_CHG_SCHD) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2S_ROUTINE_CHG_FLAG) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_HOSTREPORT) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_LISTDIRECTORY) },
							{ TOKEN_KEYVALPAIR(CMDCODE_N2N_GENERIC) },
							{ TOKEN_KEYVALPAIR(CMDCODE_A2C_EXECSNIPPET) },
						};

static struct
{
	const Conation::ArgType Type;
	const char *const Text;
} ArgTypeNames[] =	{
						{ Conation::ARGTYPE_NONE, "ARGTYPE_NONE" },
						{ Conation::ARGTYPE_NETCMDSTATUS, "ARGTYPE_NETCMDSTATUS" },
						{ Conation::ARGTYPE_INT32, "ARGTYPE_INT32" },
						{ Conation::ARGTYPE_UINT32, "ARGTYPE_UINT32" },
						{ Conation::ARGTYPE_INT64, "ARGTYPE_INT64" },
						{ Conation::ARGTYPE_UINT64, "ARGTYPE_UINT64" },
						{ Conation::ARGTYPE_BINSTREAM, "ARGTYPE_BINSTREAM" },
						{ Conation::ARGTYPE_STRING, "ARGTYPE_STRING" },
						{ Conation::ARGTYPE_FILEPATH, "ARGTYPE_FILEPATH" },
						{ Conation::ARGTYPE_SCRIPT, "ARGTYPE_SCRIPT" },
						{ Conation::ARGTYPE_FILE, "ARGTYPE_FILE" },
						{ Conation::ARGTYPE_BOOL, "ARGTYPE_BOOL" },
						{ Conation::ARGTYPE_ODHEADER, "ARGTYPE_ODHEADER" },
						{ Conation::ARGTYPE_ANY, "ARGTYPE_ANY" },
					};
static struct
{
	const StatusCode Code;
	const char *const Text;
} StatusCodeNames[] = 	{
							{ TOKEN_KEYVALPAIR(STATUS_INVALID) },
							{ TOKEN_KEYVALPAIR(STATUS_OK) },
							{ TOKEN_KEYVALPAIR(STATUS_FAILED) },
							{ TOKEN_KEYVALPAIR(STATUS_WARN) },
							{ TOKEN_KEYVALPAIR(STATUS_IERR) }, 
							{ TOKEN_KEYVALPAIR(STATUS_UNSUPPORTED) },
							{ TOKEN_KEYVALPAIR(STATUS_ACCESSDENIED) },
							{ TOKEN_KEYVALPAIR(STATUS_MISSING) },
							{ TOKEN_KEYVALPAIR(STATUS_MISUSED) },
						};

VLString CommandCodeToString(const CommandCode CmdCode)
{
	for (size_t Inc = 0u; Inc < sizeof CommandCodeNames / sizeof *CommandCodeNames; ++Inc)
	{
		if (CommandCodeNames[Inc].CmdCode == CmdCode)
		{
			return CommandCodeNames[Inc].Text;
		}
	}
	
	char Buf[128];
	snprintf(Buf, sizeof Buf, "UNKNOWN::%hhu", CmdCode);
	
	return Buf;

	
}

VLString ArgTypeToString(const Conation::ArgType Type)
{
	for (size_t Inc = 0u; Inc < sizeof ArgTypeNames / sizeof *ArgTypeNames; ++Inc)
	{
		if (ArgTypeNames[Inc].Type == Type)
		{
			return ArgTypeNames[Inc].Text;
		}
	}
	
	char Buf[128];
	snprintf(Buf, sizeof Buf, "UNKNOWN::%hu", Type);
	
	return Buf;
}

VLString StatusCodeToString(const StatusCode Code)
{
	for (size_t Inc = 0u; Inc < sizeof StatusCodeNames / sizeof *StatusCodeNames; ++Inc)
	{
		if (StatusCodeNames[Inc].Code == Code)
		{
			return StatusCodeNames[Inc].Text;
		}
	}
	
	char Buf[128];
	snprintf(Buf, sizeof Buf, "UNKNOWN::%hhu", Code);
	
	return Buf;
}

CommandCode StringToCommandCode(const VLString &String)
{
	for (size_t Inc = 0u; Inc < sizeof CommandCodeNames / sizeof *CommandCodeNames; ++Inc)
	{
		if (VLString(CommandCodeNames[Inc].Text) == String)
		{
			return CommandCodeNames[Inc].CmdCode;
		}
	}
	
	return CMDCODE_INVALID;
}

Conation::ArgType StringToArgType(const VLString &String)
{
	for (size_t Inc = 0u; Inc < sizeof ArgTypeNames / sizeof *ArgTypeNames; ++Inc)
	{
		if (VLString(ArgTypeNames[Inc].Text) == String)
		{
			return ArgTypeNames[Inc].Type;
		}
	}

	return Conation::ARGTYPE_NONE;
}

StatusCode StringToStatusCode(const VLString &String)
{
	for (size_t Inc = 0u; Inc < sizeof StatusCodeNames / sizeof *StatusCodeNames; ++Inc)
	{
		if (VLString(StatusCodeNames[Inc].Text) == String)
		{
			return StatusCodeNames[Inc].Code;
		}
	}
	return STATUS_INVALID;
}

