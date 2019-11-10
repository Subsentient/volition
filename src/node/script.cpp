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


/*
 * "If I'm too negative, you're too ignorant not to be!" -Foamy The Squirrel
 */

#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"
#include "../libvolition/include/vlthreads.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/netcore.h"

#include "script.h"
#include "processes.h"
#include "identity_module.h"
#include "main.h"
#include "files.h"
#include "web.h"

extern "C"
{ //Lua headers don't provide this.
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

///For htons/htonl etc
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif //WIN32

#include <string.h> //For memcpy

#include <map>

#ifndef LUA_OK
#define LUA_OK 0
#endif

//Prototypes
static bool LoadVLAPI(lua_State *State);
static bool VerifyLuaFuncArgs(lua_State *State, const std::vector<decltype(LUA_TNONE)> &ToCheck);


///Lua functions we provide here
static int VLAPI_GetIdentity(lua_State *State);
static int VLAPI_GetRevision(lua_State *State);
static int VLAPI_GetPlatformString(lua_State *State);
static int VLAPI_GetServerAddr(lua_State *State);
static int VLAPI_GetSSLCert(lua_State *State);
static int VLAPI_GetAuthToken(lua_State *State);
static int VLAPI_GetInitScript(lua_State *State);
static int VLAPI_GetArgvData(lua_State *State);
static int VLAPI_GetCompileTime(lua_State *State);
static int VLAPI_GetProcesses(lua_State *State);
static int VLAPI_KillProcessByName(lua_State *State);
static int VLAPI_CopyFile(lua_State *State);
static int VLAPI_DeleteFile(lua_State *State);
static int VLAPI_MoveFile(lua_State *State);
#ifndef NOCURL
static int VLAPI_GetHTTP(lua_State *State);
#endif //NOCURL
static int VLAPI_RecvStream(lua_State *State);
static int VLAPI_SendStream(lua_State *State);
static int VLAPI_GetTempDirectory(lua_State *State);
static int VLAPI_vl_sleep(lua_State *State);
static int VLAPI_FileExists(lua_State *State);
static int VLAPI_GetFileSize(lua_State *State);
static int VLAPI_StripPathFromFilename(lua_State *State);
static int VLAPI_GetSelfBinaryPath(lua_State *State);
static int VLAPI_IntegerToByteString(lua_State *State);
static int VLAPI_ListDirectory(lua_State *State);
static int VLAPI_IsDirectory(lua_State *State);
static int VLAPI_GetCWD(lua_State *State);
static int VLAPI_Chdir(lua_State *State);
static int VLAPI_SlurpFile(lua_State *State);
static int VLAPI_WriteFile(lua_State *State);
static int VLAPI_RunningAsJob(lua_State *State);

///Lua ConationStream helper functions, the ones that don't go in VLAPIFuncs.
static void InitConationStreamBindings(lua_State *State);
static int VerifyCSArgsLua(lua_State *State);
static int CountCSArgsLua(lua_State *State);
static int PopArg_LuaConationStream(lua_State *State);
static int PushArg_LuaConationStream(lua_State *State);
static int LuaConationStreamGCFunc(lua_State *State);
static int NewLuaConationStream(lua_State *State);
static int GetCSArgsList_Lua(lua_State *State);
static int GetCSData_Lua(lua_State *State);
static int GetCSArgData_Lua(lua_State *State);
static int GetCSHeader_Lua(lua_State *State);


//Called from C++ only.
static bool CloneConationStreamToLua(lua_State *State, Conation::ConationStream *Stream);

//Globals
static std::map<VLString, VLString> LoadedScripts;

static std::map<VLString, lua_CFunction> VLAPIFuncs
{
	{ "GetIdentity", VLAPI_GetIdentity },
	{ "GetRevision", VLAPI_GetRevision },
	{ "GetPlatformString", VLAPI_GetPlatformString },
	{ "GetServerAddr", VLAPI_GetServerAddr },
	{ "GetSSLCert", VLAPI_GetSSLCert },
	{ "GetAuthToken", VLAPI_GetAuthToken },
	{ "GetInitScript", VLAPI_GetInitScript },
	{ "GetArgvData", VLAPI_GetArgvData },
	{ "GetCompileTime", VLAPI_GetCompileTime },
	{ "GetProcesses", VLAPI_GetProcesses },
	{ "KillProcessByName", VLAPI_KillProcessByName },
	{ "CopyFile", VLAPI_CopyFile },
	{ "DeleteFile", VLAPI_DeleteFile },
	{ "MoveFile", VLAPI_MoveFile },
#ifndef NOCURL
	{ "GetHTTP", VLAPI_GetHTTP },
#endif //NOCURL
	{ "RecvStream", VLAPI_RecvStream },
	{ "SendStream", VLAPI_SendStream },
	{ "GetTempDirectory", VLAPI_GetTempDirectory },
	{ "vl_sleep", VLAPI_vl_sleep },
	{ "FileExists", VLAPI_FileExists },
	{ "GetFileSize", VLAPI_GetFileSize },
	{ "StripPathFromFilename", VLAPI_StripPathFromFilename },
	{ "GetSelfBinaryPath", VLAPI_GetSelfBinaryPath },
	{ "IntegerToByteString", VLAPI_IntegerToByteString },
	{ "ListDirectory", VLAPI_ListDirectory },
	{ "IsDirectory", VLAPI_IsDirectory },
	{ "GetCWD", VLAPI_GetCWD },
	{ "Chdir", VLAPI_Chdir },
	{ "SlurpFile", VLAPI_SlurpFile },
	{ "WriteFile", VLAPI_WriteFile },
	{ "RunningAsJob", VLAPI_RunningAsJob },
};

enum IntNameMapEnum : uint8_t
{
	INTNAME_INVALID,
	INTNAME_INT8 = 1,
	INTNAME_UINT8,
	INTNAME_INT16,
	INTNAME_UINT16,
	INTNAME_INT32,
	INTNAME_UINT32,
	INTNAME_INT64,
	INTNAME_UINT64,
};

static std::map<IntNameMapEnum, VLString> IntNameMap
{
	{ INTNAME_INT8, "int8" },
	{ INTNAME_UINT8, "uint8" },
	
	{ INTNAME_INT16, "int16" },
	{ INTNAME_UINT16, "uint16" },
	
	{ INTNAME_INT32, "int32" },
	{ INTNAME_UINT32, "uint32" },
	
	{ INTNAME_INT64, "int64" },
	{ INTNAME_UINT64, "uint64" },
};

//Function definitions

static bool VerifyLuaFuncArgs(lua_State *State, const std::vector<decltype(LUA_TNONE)> &ToCheck)
{
	const size_t Provided = lua_gettop(State);
	
	if (Provided != ToCheck.size())
	{
		return false;
	}
	
	for (size_t Inc = 0; Inc < ToCheck.size(); ++Inc)
	{
		if (lua_type(State, Inc + 1) != ToCheck.at(Inc)) return false;
	}
	
	return true;
}

static int DoCSIter(lua_State *State)
{
	VLDEBUG("Entered");
	if (lua_type(State, 1) != LUA_TTABLE)
	{
		VLDEBUG("First argument not a table");
		return 0;
	}
	
	//We only want our first argument, make sure there's nothing weird just to be safe.
	lua_settop(State, 1);

	///Results table that we actually return.
	lua_newtable(State);
	const size_t ResultTableIndex = lua_gettop(State);
	
	VLDEBUG("Now calling PopArg_LuaConationStream()");
	
	//Call this directly. It puts a possible pile of variables on the stack.
	lua_pushcfunction(State, PopArg_LuaConationStream);
	lua_pushvalue(State, 1);
	
	if (lua_pcall(State, 1, LUA_MULTRET, 0) != LUA_OK)
	{ //Call NewLuaConationStream() to get a new copy for arguments.
		VLDEBUG("Call of lua function failed.");
		return 0;
	}

	
	if (lua_type(State, 3) != LUA_TNUMBER)
	{ //Failure or end of iteration. First argument is always the ARGTYPE
		VLDEBUG("First result is not a number");
		lua_settop(State, 0);
		return 0;
	}
	
	const size_t Top = lua_gettop(State);
	
	for (size_t Inc = 3, Index = 1; Inc <= Top; ++Inc, ++Index)
	{
		
		VLDEBUG("Setting table index " + VLString::IntToString(Index) + " to value " + (lua_tostring(State, Inc) ? lua_tostring(State, Inc) : "Bad") + " from stack index " + VLString::IntToString(Inc) + " with type " + VLString::IntToString(lua_type(State, Inc)));
		lua_pushinteger(State, Index);
		lua_pushvalue(State, Inc);
		lua_settable(State, ResultTableIndex);
	}
	
	lua_settop(State, ResultTableIndex);
	
	VLDEBUG("Finished successfully, returning new table.");
	
	return 1; //Don't return our ConationStream instance table.
}

static int RewindCS_Lua(lua_State *State)
{
	if (!VerifyLuaFuncArgs(State, { LUA_TTABLE }))
	{
		VLDEBUG("Parameter mismatch");
	Exit:
		lua_pushboolean(State, false);
		return 1;
	}
	
	lua_getfield(State, -1, "VL_INTRNL");
	
	if (lua_type(State, -1) != LUA_TUSERDATA)
	{
		VLDEBUG("Not a userdata");
		goto Exit;
	}
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	if (!Stream)
	{
		VLDEBUG("Null stream pointer");
		goto Exit;
	}
	
	Stream->Rewind();
	
	lua_pushboolean(State, true);
	return 1;
}
	
static int GetCSIter(lua_State *State)
{
	if (!VerifyLuaFuncArgs(State, { LUA_TTABLE }))
	{
		VLDEBUG("Invalid arguments");
		return 0;
	}
	
	lua_pushcfunction(State, DoCSIter);
	lua_pushvalue(State, -2);
	return 2;
}

static int VLAPI_GetIdentity(lua_State *State)
{
	
	lua_pushstring(State, IdentityModule::GetNodeIdentity());

	return 1;
}

static int VLAPI_GetRevision(lua_State *State)
{
	lua_pushstring(State, IdentityModule::GetNodeRevision());

	return 1;
}

static int VLAPI_GetPlatformString(lua_State *State)
{
	lua_pushstring(State, IdentityModule::GetNodePlatformString());

	return 1;
}

static int VLAPI_GetInitScript(lua_State *State)
{
	lua_pushstring(State, IdentityModule::GetInitScript());
	
	return 1;
}

static int VLAPI_GetAuthToken(lua_State *State)
{
	lua_pushstring(State, IdentityModule::GetNodeAuthToken());
	
	return 1;
}

static int VLAPI_GetSSLCert(lua_State *State)
{
	lua_pushstring(State, IdentityModule::GetCertificate());
	
	return 1;
}
static int VLAPI_GetServerAddr(lua_State *State)
{
	lua_pushstring(State, IdentityModule::GetServerAddr());
	
	return 1;
}

static int VLAPI_WriteFile(lua_State *State)
{
	if (!VerifyLuaFuncArgs(State, { LUA_TSTRING, LUA_TSTRING }))
	{
		lua_pushboolean(State, false);
		return 1;
	}
	
	const VLString Path { lua_tostring(State, 1) };
	
	size_t DataLength = 0;
	
	const void *const Data = lua_tolstring(State, 2, &DataLength);
	
	if (!Data || !Utils::WriteFile(Path, Data, DataLength))
	{
		VLDEBUG("Failure caused by " + (Data ? "Utils::WriteFile()" : "lua_tolstring()"));
		lua_pushboolean(State, false);
		return 1;
	}
	
	lua_pushboolean(State, true);
	return 1;
}

static int VLAPI_GetArgvData(lua_State *State)
{
	
	char **Argv = Main::GetArgvData(nullptr);
	
	size_t Inc = 0;
	
	for (; Argv[Inc]; ++Inc)
	{
		lua_pushstring(State, Argv[Inc]);
	}
	
	return Inc;
}

static int VLAPI_GetCompileTime(lua_State *State)
{
	lua_pushinteger(State, IdentityModule::GetCompileTime());
	return 1;
}

static int VLAPI_SlurpFile(lua_State *State)
{
	if (!VerifyLuaFuncArgs(State, { LUA_TSTRING })) return 0;
	
	uint64_t FileSize = 0;
	
	const VLString Path { lua_tostring(State, 1) };
	
	if (!Utils::GetFileSize(Path, &FileSize)) return 0;
	
	
	luaL_Buffer Buf{};
	
	void *const DataLocation = luaL_buffinitsize(State, &Buf, FileSize);
	
	if (!Utils::Slurp(Path, DataLocation, FileSize))
	{
		//Where the fuck is the deallocation function for luaL_Buffer? This will have to do.
		luaL_pushresultsize(&Buf, 0);
		lua_settop(State, 0);
		return 0;
	}
	
	luaL_pushresultsize(&Buf, FileSize);
	
	return 1;
}

static int VLAPI_RunningAsJob(lua_State *State)
{
	lua_getglobal(State, "VL_ISJOB");
	
	const bool RunningAsJob = lua_toboolean(State, -1);
	
	lua_pushboolean(State, RunningAsJob);
	
	return 1;
}


static int VLAPI_GetTempDirectory(lua_State *State)
{
	lua_pushstring(State, Utils::GetTempDirectory());
	return 1;
}

static int VLAPI_vl_sleep(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TNUMBER) return 0;
	
	Utils::vl_sleep(lua_tointeger(State, 1));
	
	return 0;
}

static int VLAPI_ListDirectory(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);

	if (ArgCount != 1 || lua_type(State, 1) != LUA_TSTRING)
	{
		lua_pushnil(State);
		return 1;
	}

	VLScopedPtr<std::list<Files::DirectoryEntry>*> Results = Files::ListDirectory(lua_tostring(State, 1));

	lua_settop(State, 0);
	
	if (!Results)
	{
		lua_pushnil(State);
		return 1;
	}

	lua_newtable(State);

	size_t Inc = 1;
	for (auto Iter = Results->begin(); Iter != Results->end(); ++Iter, ++Inc)
	{
		lua_pushinteger(State, Inc);
		lua_newtable(State);

		//Store path
		lua_pushinteger(State, 1);
		lua_pushstring(State, Iter->Path);

		lua_settable(State, -3);

		//Store boolean
		lua_pushinteger(State, 2);
		lua_pushboolean(State, Iter->IsDirectory);

		lua_settable(State, -3);

		//Store subtable in greater table.
		lua_settable(State, -3);
	}
	
	return 1;
}

static int VLAPI_IsDirectory(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TSTRING)
	{
		lua_pushnil(State);
		return 1;
	}
	
	lua_pushboolean(State, Utils::IsDirectory(lua_tostring(State, 1)));
	
	return 1;
}

static int VLAPI_FileExists(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TSTRING)
	{
		lua_pushnil(State);
		return 1;
	}
	
	lua_pushboolean(State, Utils::FileExists(lua_tostring(State, 1)));
	
	return 1;
}

static int VLAPI_GetFileSize(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TSTRING)
	{
	EasyFail:
		lua_pushnil(State);
		return 1;
	}
	
	uint64_t Size = 0;
	
	if (!Utils::GetFileSize(lua_tostring(State, 1), &Size)) goto EasyFail;
	
	lua_pushinteger(State, Size);
	return 1;
}

static int VLAPI_GetCWD(lua_State *State)
{
	lua_pushstring(State, Files::GetWorkingDirectory());
	return 1;
}
	
static int VLAPI_Chdir(lua_State *State)
{
	if (lua_gettop(State) != 1 || lua_isstring(State, 1))
	{
		lua_pushnil(State);
		return 1;
	}
	
	lua_pushboolean(State, Files::Chdir(lua_tostring(State, 1)));

	return 1;
}

static int VLAPI_StripPathFromFilename(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TSTRING)
	{
		lua_pushnil(State);
		return 1;
	}
	
	lua_pushstring(State, Utils::StripPathFromFilename(lua_tostring(State, 1)));
	return 1;
}

static int VLAPI_GetSelfBinaryPath(lua_State *State)
{
	lua_pushstring(State, Utils::GetSelfBinaryPath());
	return 1;
}

static int VLAPI_IntegerToByteString(lua_State *State)
{
	bool HasConvertFlag = false;
	if (!VerifyLuaFuncArgs(State, { LUA_TNUMBER, LUA_TNUMBER }) &&
		!(HasConvertFlag = VerifyLuaFuncArgs(State, { LUA_TNUMBER, LUA_TNUMBER, LUA_TBOOLEAN })))
	{
	EasyFail:
		lua_pushnil(State);
		return 1;
	}
	
	const IntNameMapEnum IntType = static_cast<IntNameMapEnum>(lua_tointeger(State, 1));
	const uint64_t Integer = lua_tointeger(State, 2);
	const bool ConvertToNet = HasConvertFlag ? lua_toboolean(State, 3) : false;
	
	switch (IntType)
	{
		case INTNAME_INT8:
		case INTNAME_UINT8:		
		{
			uint8_t BabyInt = Integer;
			
			lua_pushlstring(State, (const char*)&BabyInt, 1);
			return 1;
		}
		case INTNAME_INT16:
		case INTNAME_UINT16:
		{
			uint16_t Val = Integer;
			
			if (ConvertToNet) Val = htons(Val);
			
			lua_pushlstring(State, (const char*)&Val, sizeof Val);
			return 1;
		}
		case INTNAME_INT32:
		case INTNAME_UINT32:
		{
			uint32_t Val = Integer;
			
			if (ConvertToNet) Val = htonl(Val);
			
			lua_pushlstring(State, (const char*)&Val, sizeof Val);
			return 1;
		}
		case INTNAME_INT64:
		case INTNAME_UINT64:
		{
			uint64_t Val = Integer;
			
			if (ConvertToNet) Val = Utils::vl_htonll(Val);
			
			lua_pushlstring(State, (const char*)&Val, sizeof Val);
			return 1;
		}
		default:
			goto EasyFail;
	}
	
	goto EasyFail;
}
	
static int VLAPI_GetProcesses(lua_State *State)
{
	std::list<Processes::ProcessListMember> ProcessList;
	
	if (!Processes::GetProcessesList(&ProcessList)) //Might as well just call it directly since we need the header anyways.
	{ //Failed
		lua_pushnil(State);
		return 1;
	}
	
	//Wipe the stack in case the retards gave us arguments. I don't care enough to make this throw an error.
	lua_settop(State, 0);
	
	//Table that contains process results.
	lua_newtable(State);
	
	size_t LuaArrayIndex = 1;
	
	for (auto Iter = ProcessList.begin(); Iter != ProcessList.end(); ++Iter, ++LuaArrayIndex)
	{
		//Push the subtable that will contain our results.
		lua_pushinteger(State, LuaArrayIndex);
		lua_newtable(State);
		
		lua_settable(State, -3);
		
		//Now we get the subtable again.
		lua_pushinteger(State, LuaArrayIndex);
		lua_gettable(State, -2);
		
		///Populate the subtable.
		
		//Process name
		lua_pushstring(State, "name");
		lua_pushstring(State, Iter->ProcessName);
		lua_settable(State, -3);
		
		//User
		lua_pushstring(State, "user");
		lua_pushstring(State, Iter->User);
		lua_settable(State, -3);
		
		//PID
		lua_pushstring(State, "pid");
		lua_pushinteger(State, Iter->PID);
		lua_settable(State, -3);
		
		//Kernel process?
		lua_pushstring(State, "iskernel");
		lua_pushboolean(State, Iter->KernelProcess);
		lua_settable(State, -3);

		//Remove the subtable from the stack
		lua_pop(State, 1);
	}
	
	return 1;
}

static int VLAPI_KillProcessByName(lua_State *State)
{
	std::vector<VLString> ProcNames;

	const size_t Top = lua_gettop(State);
	
	ProcNames.reserve(Top);
	
	for (size_t Inc = 1; Inc <= Top; ++Inc)
	{
		ProcNames.push_back(lua_tostring(State, Inc));
	}
	
	//We got the arguments we need.
	lua_settop(State, 0);
	
	std::vector<VLString> Failed;
	
	for (size_t Inc = 0; Inc < ProcNames.size(); ++Inc)
	{
		if (!Processes::KillProcessByName(ProcNames[Inc]))
		{
			Failed.push_back(ProcNames[Inc]);
		}
	}
	
	lua_pushboolean(State, Failed.size() < ProcNames.size()); //Did at least one succeed?
	
	for (size_t Inc = 0; Inc < Failed.size(); ++Inc)
	{
		lua_pushstring(State, Failed[Inc]);
	}
	
	return Failed.size() + 1; //One for the boolean.
}

static int VLAPI_CopyFile(lua_State *State)
{
	if (lua_gettop(State) != 2 || lua_type(State, 1) != LUA_TSTRING || lua_type(State, 2) != LUA_TSTRING)
	{ //We need two strings.
		lua_pushboolean(State, false);
		
		return 1;
	}
	
	VLString Source = lua_tostring(State, 1);
	VLString Destination = lua_tostring(State, 2);
	
	lua_settop(State, 0);
	
	const bool Result = Files::Copy(+Source, +Destination);
	
	lua_pushboolean(State, Result);
	
	return 1;
}

static int VLAPI_MoveFile(lua_State *State)
{
	if (lua_gettop(State) != 2 || lua_type(State, 1) != LUA_TSTRING || lua_type(State, 2) != LUA_TSTRING)
	{ //We need two strings.
		lua_pushboolean(State, false);
		
		return 1;
	}
	
	VLString Source = lua_tostring(State, 1);
	VLString Destination = lua_tostring(State, 2);
	
	lua_settop(State, 0);
	
	const bool Result = Files::Move(+Source, +Destination);
	
	lua_pushboolean(State, Result);
	
	return 1;
}
	
static int VLAPI_DeleteFile(lua_State *State)
{
	if (lua_gettop(State) != 1 || lua_type(State, 1) != LUA_TSTRING)
	{ //We need two strings.
		lua_pushboolean(State, false);
		
		return 1;
	}
	
	VLString File = lua_tostring(State, 1);
	
	lua_settop(State, 0);
	
	const NetCmdStatus Result = Files::Delete(+File);
	
	lua_pushboolean(State, Result);
	
	if (!Result)
	{ //if it failed, give us the error message.
		lua_pushstring(State, Result.Msg);
	}
	
	const size_t Count = Result ? 1 : 2;
	
	return Count;
}

static int VLAPI_RecvStream(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	bool PopAfterwards = true;
	
	if (ArgCount > 1 || (ArgCount == 1 && lua_type(State, 1) != LUA_TBOOLEAN))
	{
#ifdef DEBUG
		puts("VLAPI_RecvStream(): Arguments mismatch, aborting.");
#endif

		goto EasyFail;

	}
	
	if (ArgCount) //They want to specify the boolean.
	{
		PopAfterwards = lua_toboolean(State, 1);
	}
	
	lua_getglobal(State, "VL_OurJob_LUSRDTA");
	
	if (lua_type(State, -1) != LUA_TLIGHTUSERDATA)
	{
#ifdef DEBUG
		puts("VLAPI_RecvStream(): Internal userdata is not actually a userdata!");
#endif
	EasyFail:
		lua_pushnil(State);
		return 1;
	}
	
	Jobs::Job *OurJob = static_cast<Jobs::Job*>(lua_touserdata(State, -1));
	
	if (!OurJob)
	{
#ifdef DEBUG
		puts("VLAPI_RecvStream(): Failed to convert userdata to a Jobs::Job*, aborting.");
#endif
		goto EasyFail;
	}
	//Nothing left in the queue.
	
	VLScopedPtr<Conation::ConationStream*> NewStream { OurJob->Read_Queue.Pop(PopAfterwards) };
	
	if (!NewStream)
	{
#ifdef DEBUG
		puts("VLAPI_RecvStream(): Failed to pop from read queue! Aborting.");
#endif
		goto EasyFail;
	}
	
	CloneConationStreamToLua(State, NewStream);
	
#ifdef DEBUG
	puts(VLString("VLAPI_RecvStream(): Have table on stack: ") + ((lua_type(State, -1) == LUA_TTABLE) ? "true" : "false"));
#endif

	return 1;
}

static int VLAPI_SendStream(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TTABLE)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushboolean(State, false);
		return 1;
	}
	
	lua_getfield(State, 1, "VL_INTRNL");
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	if (!Stream) goto EasyFail;
	
	Main::PushStreamToWriteQueue(*Stream);
	
	lua_pushboolean(State, true);
	return 1;
}

static bool CloneConationStreamToLua(lua_State *State, Conation::ConationStream *Stream)
{
	lua_pushcfunction(State, NewLuaConationStream);
	lua_pushlightuserdata(State, Stream);
	
	if (lua_pcall(State, 1, 1, 0) != LUA_OK)
	{ //Call NewLuaConationStream() to get a new copy for arguments.
		lua_close(State);
		return false;
	}
	
#ifdef DEBUG
	puts(VLString("CloneConationStreamToLua(): Result is table: ") + ((lua_type(State, -1) == LUA_TTABLE) ? "true" : "false"));
#endif

	return lua_type(State, -1) == LUA_TTABLE;
}

#ifndef NOCURL
static int VLAPI_GetHTTP(lua_State *State)
{
	const size_t NumArgs = lua_gettop(State);
	
	if (NumArgs < 1 || lua_type(State, 1) != LUA_TSTRING) //We at least need a URL.
	{
		lua_pushnil(State);
		return 1;
	}
	
	const VLString URL = lua_tostring(State, 1);
	
	int Attempts = 3;
	VLString UserAgent;
	VLString Referrer;
	
	if (NumArgs >= 2 && lua_type(State, 2) == LUA_TNUMBER)
	{ //Set number of attempts.
		Attempts = lua_tointeger(State, 2);
	}
	
	if (NumArgs >= 3 && lua_type(State, 3) == LUA_TSTRING)
	{
		UserAgent = lua_tostring(State, 3);
	}
	
	if (NumArgs >= 4 && lua_type(State, 4) == LUA_TSTRING)
	{
		Referrer = lua_tostring(State, 4);
	}
	
	lua_settop(State, 0);
	
	VLString FilePath;
	
	const bool Result = Web::GetHTTP(+URL, &FilePath, Attempts,
									UserAgent ? +UserAgent : nullptr,
									Referrer ? +Referrer : nullptr);
	if (Result && FilePath)
	{
#ifdef DEBUG
		puts("VLAPI_GetHTTP(): Succeeded in call to Web::GetHTTP().");
#endif
		lua_pushstring(State, FilePath);
	}
	else
	{
#ifdef DEBUG
		puts("VLAPI_GetHTTP(): Failed in call to Web::GetHTTP()!");
#endif
		lua_pushnil(State);
	}
	
	return 1;		
}
#endif //NOCURL

static bool LoadVLAPI(lua_State *State)
{
#ifdef DEBUG
	puts("Entered LoadVLAPI()");
#endif

	//First create the global API table.
	lua_newtable(State);
	
	VLString TableName("VL");
	
	lua_setglobal(State, TableName);
	lua_getglobal(State, TableName); //Cuz lua_setglobal popped it right off again.
	
	/**All three of these things I'm registering probably belong in InitConationStreamBindings(),
	 * but I didn't put them there because that's too much typing.
	 * Would you prefer to type VL.ConationStream.STATUS_OK, or VL.STATUS_OK?
	 **/
#ifdef DEBUG
	puts("LoadVLAPI(): Registering status codes");
#endif
	//Register status codes.
	lua_pushstring(State, StatusCodeToString(STATUS_INVALID));
	lua_pushinteger(State, STATUS_INVALID);
	lua_settable(State, -3);
	
#ifdef DEBUG
	puts("LoadVLAPI(): Registered STATUS_INVALID");
#endif

	for (uint8_t Inc = 1; Inc && Inc <= STATUS_MAXPOPULATED; Inc <<= 1)
	{
		lua_pushstring(State, StatusCodeToString(static_cast<StatusCode>(Inc)));
		lua_pushinteger(State, Inc);
		lua_settable(State, -3);
#ifdef DEBUG
		puts("LoadVLAPI(): Ending status registration loop iteration");
#endif

	}

#ifdef DEBUG
	puts("LoadVLAPI(): Registering command codes");
#endif
	//Register command codes.
	for (uint8_t Inc = 0; Inc < CMDCODE_MAX; ++Inc)
	{
		lua_pushstring(State, CommandCodeToString(static_cast<CommandCode>(Inc)));
		lua_pushinteger(State, Inc);
		lua_settable(State, -3);
	}
	
#ifdef DEBUG
	puts("LoadVLAPI(): Registering conation argument types");
#endif
	//Register argument types.
	lua_pushstring(State, ArgTypeToString(Conation::ARGTYPE_NONE));
	lua_pushinteger(State, Conation::ARGTYPE_NONE);
	lua_settable(State, -3);
	for (Conation::ArgTypeUint Inc = 1; Inc && Inc <= Conation::ARGTYPE_MAXPOPULATED; Inc <<= 1)
	{
		lua_pushstring(State, ArgTypeToString(static_cast<Conation::ArgType>(Inc)));
		lua_pushinteger(State, Inc);
		lua_settable(State, -3);
	}

#ifdef DEBUG
	puts("LoadVLAPI(): Registering API functions");
#endif
	//Register all API functions
	for (auto Iter = VLAPIFuncs.begin(); Iter != VLAPIFuncs.end(); ++Iter)
	{
		lua_pushstring(State, Iter->first);
		lua_pushcfunction(State, Iter->second);
		
		lua_settable(State, -3);
	}
	
#ifdef DEBUG
	puts("LoadVLAPI(): made it past primary bindings, attempting to initialize conation stream bindings.");
#endif

	for (auto Iter = IntNameMap.begin(); Iter != IntNameMap.end(); ++Iter)
	{
		lua_pushstring(State, Iter->second);
		lua_pushinteger(State, Iter->first);
		lua_settable(State, -3);
	}

	///Some constants we want. Technically belong in VL.ConationStream, but again, too much typing.
	//Report bit
	lua_pushstring(State, "IDENT_ISREPORT_BIT");
	lua_pushinteger(State, Conation::IDENT_ISREPORT_BIT);

	lua_settable(State, -3);
	
	//Routine bit
	lua_pushstring(State, "IDENT_ISROUTINE_BIT");
	lua_pushinteger(State, Conation::IDENT_ISROUTINE_BIT);

	lua_settable(State, -3);
	
	//OS path separator
	lua_pushstring(State, "ospathdiv");
	lua_pushstring(State, PATH_DIVIDER_STRING);
	
	lua_settable(State, -3);
	
	InitConationStreamBindings(State);
	return true;
}

static int LuaConationStreamGCFunc(lua_State *State)
{
	Conation::ConationStream *Ptr = static_cast<Conation::ConationStream*>(lua_touserdata(State, 1));
	
	//Explicitly call the destructor.
	Ptr->~ConationStream();
	
	return 0;
}

static int NewLuaConationStream(lua_State *State)
{ //Create a new instance of the class.
	std::vector<decltype(LUA_TNONE)> BasicConstructorParams { LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER };
	std::vector<decltype(LUA_TNONE)> CloneStreamConstructorParams { LUA_TTABLE };
	std::vector<decltype(LUA_TNONE)> CBasedCloneConstructorParams { LUA_TLIGHTUSERDATA };
	
	struct
	{
		CommandCode CmdCode;
		uint8_t Flags;
		uint64_t Ident;
		Conation::ConationStream *ToClone;
	} Args{};
	
	if (VerifyLuaFuncArgs(State, BasicConstructorParams))
	{ //Standard constructor.
		const lua_Integer Code = lua_tointeger(State, 1);
		
		if (Code <= 0 || Code > CMDCODE_MAX)
		{ //Invalid value.
			lua_settop(State, 0);
			lua_pushnil(State);
			return 1;
		}
		
		Args.CmdCode = static_cast<CommandCode>(Code);
		Args.Flags = lua_tointeger(State, 2);
		Args.Ident = lua_tointeger(State, 3);
	}
	else if (VerifyLuaFuncArgs(State, CloneStreamConstructorParams))
	{ //Cloning one.
		
		//Get the userdata pointer we've got here.
		lua_pushstring(State, "VL_INTRNL");
		lua_gettable(State, 1);
		
		if (lua_type(State, -1) != LUA_TUSERDATA)
		{ //Wh, why?
			lua_settop(State, 0);
			lua_pushnil(State);
			return 1;
		}
		Args.ToClone = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	}
	else if (VerifyLuaFuncArgs(State, CBasedCloneConstructorParams))
	{
		Args.ToClone = static_cast<Conation::ConationStream*>(lua_touserdata(State, 1));
	}
	else
	{ //The user is retarded.
		lua_settop(State, 0);
		lua_pushnil(State);
		return 1;
	}
	
	//We now have all arguments, wipe the stack.
	lua_settop(State, 0);
	
	//Find the class template table cuz we'll nee d it later and it's actually better to put it here.
	lua_getglobal(State, "VL");
	
	//Create the new instance table.
	lua_newtable(State);
	
	//Create its metatable.
	lua_newtable(State);
	
	///Populate the metatable.

	//Index metamethod.
	lua_pushstring(State, "__index");
	//Puts the class template table on the stack.
	lua_pushstring(State, "ConationStream");
	lua_gettable(State, -5); ///Goes all the way up to that getglobal wayyy up there.
	//Set the __index metamethod.
	lua_settable(State, -3);
	
	///Now set the metatable. This pops the metatable afterwards.
	lua_setmetatable(State, -2);
	
	//Finally. This metatable code hurt to write.
	/**Now clean up the stack for everything but our table instance.
	 * !!! We're back at index 1! !!!**/
	lua_replace(State, 1);
	lua_settop(State, 1);
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_newuserdata(State, sizeof(Conation::ConationStream)));
	
	//We check which are defined in the Args struct to see what we need to do.
	if (Args.CmdCode != CMDCODE_INVALID)
	{
		new (Stream) Conation::ConationStream(Args.CmdCode, Args.Flags, Args.Ident);
	}
	else if (Args.ToClone)
	{
		new (Stream) Conation::ConationStream(Args.ToClone->GetHeader(), Args.ToClone->GetArgData());
	}

	//Setting the internal userdata now.	
	lua_pushstring(State, "VL_INTRNL");
	lua_pushvalue(State, -2);
	lua_settable(State, -4);

	//Userdata's metatable.	
	lua_newtable(State);
	
	//Garbage collection function for metatable.
	lua_pushstring(State, "__gc");
	lua_pushcfunction(State, LuaConationStreamGCFunc);
	lua_settable(State, -3);
	
	///Setting the userdata's metatable finally.
	lua_setmetatable(State, -2);
	
	//Finally we're done. God this function sucked.
	lua_settop(State, 1);

	return 1;
}

static int BuildCSResponse_Lua(lua_State *State)
{
	if (!VerifyLuaFuncArgs(State, { LUA_TTABLE }))
	{
		return 0;
	}
	
	lua_getfield(State, -1, "VL_INTRNL");
	
	if (lua_type(State, -1) != LUA_TUSERDATA) return 0;
	
	const Conation::ConationStream *const OriginalPtr = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));

	if (!OriginalPtr) return 0;
	
	Conation::ConationStream::StreamHeader Hdr { OriginalPtr->GetHeader() };

	Conation::ConationStream Original { Hdr, OriginalPtr->GetArgData() };

	//Make it into a report
	Hdr.CmdIdentFlags |= Conation::IDENT_ISREPORT_BIT;
	Hdr.StreamArgsSize = 0;
	
	//New stream we're gonna use.
	Conation::ConationStream New { Hdr };
	
	lua_settop(State, 0);
	
	VLScopedPtr<Conation::ConationStream::BaseArg*> Arg { Original.PopArgument(true) }; //Peek only

	if (Arg && Arg->Type == Conation::ARGTYPE_ODHEADER)
	{
		Conation::ConationStream::ODHeaderArg *ODArg = static_cast<Conation::ConationStream::ODHeaderArg*>(+Arg);
		//Invert origin/destination
		New.Push_ODHeader(ODArg->Hdr.Destination, ODArg->Hdr.Origin);
		VLDEBUG("Using OD Destination " +ODArg->Hdr.Destination + " and origin " + +ODArg->Hdr.Origin);
	}
	
	CloneConationStreamToLua(State, &New);
	
	return 1;
}
	
static int GetCSHeader_Lua(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TTABLE)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushnil(State);
		return 1;
	}
	
	lua_pushstring(State, "VL_INTRNL");
	lua_gettable(State, 1);
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	if (!Stream) goto EasyFail;
	
	lua_settop(State, 0);
	
	const Conation::ConationStream::StreamHeader Hdr = Stream->GetHeader();
	
	//We return this in a table.
	lua_newtable(State);
	
	lua_pushstring(State, "argssize");
	lua_pushinteger(State, Hdr.StreamArgsSize);
	lua_settable(State, -3);
	
	lua_pushstring(State, "flags");
	lua_pushinteger(State, Hdr.CmdIdentFlags);
	lua_settable(State, -3);
	
	lua_pushstring(State, "ident");
	lua_pushinteger(State, Hdr.CmdIdent);
	lua_settable(State, -3);
	
	lua_pushstring(State, "cmdcode");
	lua_pushinteger(State, Hdr.CmdCode);
	lua_settable(State, -3);
	
	return 1;
}

static int GetCSArgsList_Lua(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TTABLE)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushnil(State);
		return 1;
	}
	
	lua_pushstring(State, "VL_INTRNL");
	lua_gettable(State, 1);
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	if (!Stream) goto EasyFail;
	
	lua_settop(State, 0); //We don't need anything else on the stack now.
	
	lua_newtable(State);
	
	std::vector<Conation::ArgType> *Args = Stream->GetArgTypes();
	
	for (size_t Inc = 0; Inc < Args->size(); ++Inc)
	{
		lua_pushinteger(State, Inc + 1);
		lua_pushinteger(State, Args->at(Inc));
		lua_settable(State, 1);
	}
	
	delete Args;
	
	return 1;
}

static int PushArg_LuaConationStream(lua_State *State)
{
	const size_t ArgCount = lua_gettop(State);
	
	/* First argument is our implicit self parameter that should be a table,
	 * second argument is an integer mapping to an ArgType specifying the argument type,
	 * third argument is the value. We can have more than 3 for some argument types.
	 */
	if (ArgCount < 3 || lua_type(State, 1) != LUA_TTABLE ||
		lua_type(State, 2) != LUA_TNUMBER)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushboolean(State, false);
		return 1;
	}
	
	lua_pushstring(State, "VL_INTRNL");
	lua_gettable(State, 1); //Get the userdata from our implicit self argument.
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	lua_pop(State, 1); //We got what we need, pop the userdata off.
	
	if (lua_tointeger(State, 2) < 0 || lua_tointeger(State, 2) > Conation::ARGTYPE_MAXPOPULATED) goto EasyFail; //Wrong kind of integer.
	
	Conation::ArgType Type = static_cast<Conation::ArgType>(lua_tointeger(State, 2));
	
	switch (Type)
	{
		case Conation::ARGTYPE_NETCMDSTATUS:
		{
			
			if (lua_tointeger(State, 3) < 0 || lua_tointeger(State, 3) > STATUS_MAXPOPULATED) goto EasyFail;
			
			const StatusCode Code = static_cast<StatusCode>(lua_tointeger(State, 3));
			
			VLString Msg;
			if (ArgCount == 4) //We have a message string specified.
			{
				if (lua_type(State, 4) != LUA_TSTRING) goto EasyFail; //The user is 'special'.
				Msg = lua_tostring(State, 4);
			}
			else if (ArgCount > 4) goto EasyFail;
			
			NetCmdStatus Status(Code == STATUS_OK || Code == STATUS_WARN, Code, Msg ? +Msg : nullptr);
			
			Stream->Push_NetCmdStatus(Status);
			break;
		}
		case Conation::ARGTYPE_BOOL:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TBOOLEAN) goto EasyFail;
			
			Stream->Push_Bool(lua_toboolean(State, 3));
			break;
		}
		case Conation::ARGTYPE_STRING:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TSTRING) goto EasyFail;
			
			Stream->Push_String(lua_tostring(State, 3));
			break;
		}
		case Conation::ARGTYPE_SCRIPT:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TSTRING) goto EasyFail;
			
			Stream->Push_Script(lua_tostring(State, 3));
			break;
		}
		case Conation::ARGTYPE_FILEPATH:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TSTRING) goto EasyFail;
			
			Stream->Push_FilePath(lua_tostring(State, 3));
			break;
		}
		case Conation::ARGTYPE_INT32:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TNUMBER) goto EasyFail;
			
			Stream->Push_Int32(lua_tointeger(State, 3));
			break;
		}
		case Conation::ARGTYPE_UINT32:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TNUMBER) goto EasyFail;
			
			Stream->Push_Uint32(lua_tointeger(State, 3));
			break;
		}
		case Conation::ARGTYPE_INT64:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TNUMBER) goto EasyFail;
			
			Stream->Push_Int64(lua_tointeger(State, 3));
			break;
		}
		case Conation::ARGTYPE_UINT64:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TNUMBER) goto EasyFail;
			
			Stream->Push_Uint64(lua_tointeger(State, 3));
			break;
		}
		case Conation::ARGTYPE_ODHEADER:
		{
			if (ArgCount != 4 || lua_type(State, 3) != LUA_TSTRING || lua_type(State, 4) != LUA_TSTRING) goto EasyFail;
			
			Stream->Push_ODHeader(lua_tostring(State, 3), lua_tostring(State, 4));
			break;
		}
		case Conation::ARGTYPE_FILE:
		{
			if (ArgCount == 3)
			{ //It's a file path.
				if (lua_type(State, 3) != LUA_TSTRING) goto EasyFail;
				
				Stream->Push_File(lua_tostring(State, 3));
			}
			else if (ArgCount == 4)
			{
				if (lua_type(State, 3) != LUA_TSTRING || lua_type(State, 4)) goto EasyFail;
				
				VLString Filename = lua_tostring(State, 3);
				
				size_t BytestreamSize = 0;
				const uint8_t *Bytestream = (const uint8_t*)lua_tolstring(State, 4, &BytestreamSize);
				
				Stream->Push_File(Filename, Bytestream, BytestreamSize);
			}
			else goto EasyFail;
			break;
		}
		case Conation::ARGTYPE_BINSTREAM:
		{
			if (ArgCount > 3 || lua_type(State, 3) != LUA_TSTRING) goto EasyFail;
			
			size_t BinStreamLength = 0;
			
			const void *BinStream = lua_tolstring(State, 3, &BinStreamLength);
			
			Stream->Push_BinStream(BinStream, BinStreamLength);
			break;
		}
		default:
			goto EasyFail; //Invalid value.
	}
			
	lua_settop(State, 0);
	lua_pushboolean(State, true);
#ifdef DEBUG
	puts(VLString("PushArg_LuaConationStream(): Push successful for value of type ") + ArgTypeToString(Type));
#endif
	return 1;
}

static int PopArg_LuaConationStream(lua_State *State)
{
	const size_t OldStackTop = lua_gettop(State);

	if (lua_type(State, -1) != LUA_TTABLE)
	{
		VLDEBUG("Invalid first argument is not a table");
	EasyFail:
		lua_settop(State, OldStackTop);
		return 0;
	}
	
	//Get the userdata.
	lua_getfield(State, -1, "VL_INTRNL");
	
	if (lua_type(State, -1) != LUA_TUSERDATA)
	{
		VLDEBUG("No userdata found");
		goto EasyFail;
	}
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));

	lua_settop(State, OldStackTop);
	
	if (!Stream)
	{
		VLDEBUG("Failed to decode userdata");
		goto EasyFail;
	}

	//No arguments.
	if (!Stream->CountArguments())
	{
		VLDEBUG("No arguments in stream at all, rewound or otherwise");
		goto EasyFail;
	}
	
	VLScopedPtr<Conation::ConationStream::BaseArg*> Arg { Stream->PopArgument() };
	
	if (!Arg)
	{
		VLDEBUG("No arguments left");
		goto EasyFail;
	}
	
	//We also need to return the string name of the argument type for our first return value.
	lua_pushinteger(State, Arg->Type);
	
	switch (Arg->Type)
	{
		case Conation::ARGTYPE_BOOL:
		{
			lua_pushboolean(State, Arg->ReadAs<Conation::ConationStream::BoolArg>().Value);
			break;
		}
		case Conation::ARGTYPE_FILE:
		{
			const Conation::ConationStream::FileArg *File = &Arg->ReadAs<Conation::ConationStream::FileArg>();
			
			lua_pushstring(State, File->Filename);
			lua_pushlstring(State, (const char*)File->Data, File->DataSize);
			
			break;
		}
		case Conation::ARGTYPE_BINSTREAM:
		{
			const Conation::ConationStream::BinStreamArg *BinStream = &Arg->ReadAs<Conation::ConationStream::BinStreamArg>();
			
			lua_pushlstring(State, (const char*)BinStream->Data, BinStream->DataSize);
			break;
		}
		case Conation::ARGTYPE_FILEPATH:
		{
			lua_pushstring(State, Arg->ReadAs<Conation::ConationStream::FilePathArg>().Path);
			break;
		}
		case Conation::ARGTYPE_STRING:
		{
			lua_pushstring(State, Arg->ReadAs<Conation::ConationStream::StringArg>().String);
			break;
		}
		case Conation::ARGTYPE_SCRIPT:
		{
			lua_pushstring(State, Arg->ReadAs<Conation::ConationStream::ScriptArg>().String);
			break;
		}
		case Conation::ARGTYPE_INT32:
		{
			lua_pushinteger(State, Arg->ReadAs<Conation::ConationStream::Int32Arg>().Value);
			break;
		}
		case Conation::ARGTYPE_UINT32:
		{
			lua_pushinteger(State, Arg->ReadAs<Conation::ConationStream::Uint32Arg>().Value);
			break;
		}
		case Conation::ARGTYPE_INT64:
		case Conation::ARGTYPE_UINT64: //Because Lua's only integer type is 64-bit signed, we just have to hope it fits.
		{
			lua_pushinteger(State, Arg->ReadAs<Conation::ConationStream::Int64Arg>().Value);
			break;
		}
		case Conation::ARGTYPE_NETCMDSTATUS:
		{
			const Conation::ConationStream::NetCmdStatusArg *Status = &Arg->ReadAs<Conation::ConationStream::NetCmdStatusArg>();
			
			lua_pushboolean(State, Status->Code == STATUS_OK || Status->Code == STATUS_WARN); //Cuz sometimes we just wanna know if it worked.
			lua_pushinteger(State, Status->Code);
			lua_pushstring(State, Status->Msg);
			break;
		}
		case Conation::ARGTYPE_ODHEADER:
		{
			const Conation::ConationStream::ODHeader &Hdr = Arg->ReadAs<Conation::ConationStream::ODHeaderArg>().Hdr;
			
			lua_pushstring(State, Hdr.Origin);
			lua_pushstring(State, Hdr.Destination);
			break;
		}
		default:
			//Unknown value type.
			VLDEBUG("Unknown value type " + VLString::IntToString(Arg->Type));
			goto EasyFail;
	}
	
	VLDEBUG("Success, returning");
	//Aaaand this is why the stack needs to be empty before we start returning arguments.
	return lua_gettop(State) - OldStackTop;
	
}

static int CountCSArgsLua(lua_State *State)
{
	if (lua_gettop(State) != 1 || lua_type(State, 1) != LUA_TTABLE)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushnil(State);
		return 1;
	}
	
	lua_pushstring(State, "VL_INTRNL");
	lua_gettable(State, 1);
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	lua_settop(State, 0);
	
	lua_pushinteger(State, Stream->CountArguments());
	return 1;
}

static int VerifyCSArgsLua(lua_State *State)
{
	size_t ArgCount = lua_gettop(State);
	
	if (!ArgCount || lua_type(State, 1) != LUA_TTABLE)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushboolean(State, false);
		return 1;
	}
	
	lua_pushstring(State, "VL_INTRNL");
	lua_gettable(State, 1);
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	size_t Inc = 2; //Cuz 1 is the 'self' argument.
	
	std::vector<Conation::ArgType> ToCheck;
	ToCheck.reserve(ArgCount - 1);
	
	for (; Inc <= ArgCount; ++Inc)
	{
		if (lua_type(State, Inc) != LUA_TNUMBER) goto EasyFail;
		
		const lua_Integer Num = lua_tointeger(State, Inc);
		
		if (Num < 0 || Num > Conation::ARGTYPE_MAXPOPULATED) goto EasyFail;
		
		ToCheck.push_back(static_cast<Conation::ArgType>(Inc));
	}
	
	
	const bool Result = Stream->VerifyArgTypes(ToCheck);
	
	lua_settop(State, 0);
	
	lua_pushboolean(State, Result);
	
	return 1;
}

static void InitConationStreamBindings(lua_State *State)
{
	///Create the class template table.
	lua_pushstring(State, "ConationStream");
	lua_newtable(State);
	
	lua_settable(State, -3);
	
	///Get the class template table back onto the stack since lua_settable() popped it.
	lua_pushstring(State, "ConationStream");
	lua_gettable(State, -2);

	///Populate the class template table.
	
	//The new instance function.
	lua_pushstring(State, "New");
	lua_pushcfunction(State, NewLuaConationStream);

	lua_settable(State, -3);
	
	//Argument pushing
	lua_pushstring(State, "Push");
	lua_pushcfunction(State, PushArg_LuaConationStream);
	
	lua_settable(State, -3);
	
	//Argument popping
	lua_pushstring(State, "Pop");
	lua_pushcfunction(State, PopArg_LuaConationStream);
	
	lua_settable(State, -3);
	
	//Verification of argument types.
	lua_pushstring(State, "VerifyArgTypes");
	lua_pushcfunction(State, VerifyCSArgsLua);
	
	lua_settable(State, -3);
	
	//Argument counting.
	lua_pushstring(State, "CountArguments");
	lua_pushcfunction(State, CountCSArgsLua);
	
	lua_settable(State, -3);
	
	//Returns a table of argument types.
	lua_pushstring(State, "GetArgTypes");
	lua_pushcfunction(State, GetCSArgsList_Lua);
	
	lua_settable(State, -3);
	
	//When we want a string containing the full raw data.
	lua_pushstring(State, "GetData");
	lua_pushcfunction(State, GetCSData_Lua);
	
	lua_settable(State, -3);
	
	//When we want a string just containing the argument data, minus the header.
	lua_pushstring(State, "GetArgData");
	lua_pushcfunction(State, GetCSArgData_Lua);
	
	lua_settable(State, -3);

	//Get the header info for the stream.
	lua_pushstring(State, "GetHeader");
	lua_pushcfunction(State, GetCSHeader_Lua);
	
	lua_settable(State, -3);
	
	//Build an otherwise empty, ODHeader-inverted, report-flagged response message from the original's stream headers.
	lua_pushstring(State, "BuildResponse");
	lua_pushcfunction(State, BuildCSResponse_Lua);

	lua_settable(State, -3);

	//Iterate over stream arguments
	lua_pushstring(State, "Iter");
	lua_pushcfunction(State, GetCSIter);
	
	lua_settable(State, -3);
	
	//Rewind stream
	lua_pushstring(State, "Rewind");
	lua_pushcfunction(State, RewindCS_Lua);
	
	lua_settable(State, -3);
	
	//Pop ConationStream from the stack before we return, for the next binding functions.
	///This is important!
	lua_pop(State, 1);
}

static int GetCSData_Lua(lua_State *State)
{
	size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TTABLE)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushboolean(State, false);
		return 1;
	}
	
	lua_pushstring(State, "VL_INTRNL");
	lua_gettable(State, 1);
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	if (!Stream) goto EasyFail;
	
	lua_pushlstring(State, (const char*)Stream->GetData().data(), Stream->GetData().size());
	
	return 1;
}
static int GetCSArgData_Lua(lua_State *State)
{
	size_t ArgCount = lua_gettop(State);
	
	if (ArgCount != 1 || lua_type(State, 1) != LUA_TTABLE)
	{
	EasyFail:
		lua_settop(State, 0);
		lua_pushboolean(State, false);
		return 1;
	}
	
	lua_pushstring(State, "VL_INTRNL");
	lua_gettable(State, 1);
	
	if (lua_type(State, -1) != LUA_TUSERDATA) goto EasyFail;
	
	Conation::ConationStream *Stream = static_cast<Conation::ConationStream*>(lua_touserdata(State, -1));
	
	if (!Stream) goto EasyFail;
	
	lua_pushlstring(State, (const char*)Stream->GetArgData(), Stream->GetHeader().StreamArgsSize);
	
	return 1;
}

bool Script::LoadScript(const char *ScriptBuffer, const char *ScriptName, const bool OverwritePermissible)
{
	if (!OverwritePermissible && LoadedScripts.count(ScriptName) != 0) return false;
	
	LoadedScripts[ScriptName] = ScriptBuffer;
	return true;
}

bool Script::UnloadScript(const char *ScriptName)
{
	if (!LoadedScripts.count(ScriptName)) return false;
	
	LoadedScripts.erase(ScriptName);
	return true;
}

bool Script::ScriptIsLoaded(const char *ScriptName)
{
	return LoadedScripts.count(ScriptName);
}

static lua_State *InitScript(const char *ScriptName)
{
	if (ScriptName && LoadedScripts.count(ScriptName) == 0) return nullptr;
	
#ifdef DEBUG
	puts("Entered InitScript()");
#endif
	lua_State *State = luaL_newstate();

#ifdef DEBUG
	puts("luaL_newstate() succeeded");
#endif

	//Load standard Lua libraries.
	luaL_openlibs(State);

#ifdef DEBUG
	puts("luaL_openlibs() succeeded");
#endif

	//Load VLAPI functions
	LoadVLAPI(State);

#ifdef DEBUG
	puts("LoadVLAPI() succeeded");
#endif

	bool Success = false;
	
	if (!ScriptName) //Initialization script glued to the node
	{
		const VLString &StartupScript { IdentityModule::GetInitScript() };
		
		if (!StartupScript)
		{
#ifdef DEBUG
			fputs("InitScript() executed with a null pointer but no startup script present in identity module!\n", stderr);
#endif
			lua_close(State);
			return nullptr;
		}
		
		Success = luaL_loadbuffer(State, +StartupScript, StartupScript.Length(), "VLISCRIPT") == LUA_OK;
	}
	else
	{
		//Load the script
		Success = luaL_loadbuffer(State, LoadedScripts[ScriptName], LoadedScripts[ScriptName].Length(), ScriptName) == LUA_OK;
	}
	
	if (!Success)
	{
		lua_close(State);
		
		return nullptr;
	}
	

#ifdef DEBUG
	puts("luaL_loadbuffer() succeeded");
#endif

	//Set this BEFORE we call the initial script startup.
	lua_pushboolean(State, ScriptName != nullptr);
	lua_setglobal(State, "VL_ISJOB");

	if (lua_pcall(State, 0, 0, 0) != LUA_OK)
	{
		lua_close(State);

		return nullptr;
	}

#ifdef DEBUG
	puts("lua_pcall() succeeded");
#endif
	
	lua_settop(State, 0); //Make sure the stack is clean afterwards.

	return State;
	
}

bool Script::ExecuteScriptFunction(const char *ScriptName, const char *FunctionName, Conation::ConationStream *Stream, Jobs::Job *OurJob)
{
#ifdef DEBUG
	puts("Script::ExecuteScriptFunction(): Attempting to InitScript()");
#endif

	lua_State *State = InitScript(ScriptName);
	
	if (!State) throw ScriptError { ScriptName, FunctionName, "Failed to call InitScript() for script" };

#ifdef DEBUG
	puts("Script::ExecuteScriptFunction(): InitScript() succeeded");
#endif

	if (OurJob != nullptr)
	{ //Store our job pointer for C/C++ functions that want it.
		lua_pushlightuserdata(State, OurJob);
		lua_setglobal(State, "VL_OurJob_LUSRDTA");
	}
	
	///All this is to create a duplicated Lua-ified ConationStream for the argument to FunctionName.
	
	if (!CloneConationStreamToLua(State, Stream))
	{ //Call the Lua function VL.ConationStream:New() to get a new copy for arguments. Pushes the Lua ConationStream onto the stack.
		lua_close(State);
		throw ScriptError { ScriptName, FunctionName
#ifdef DEBUG
						, "Failed to lua_pcall() required C prerequesite function NewLuaConationStream()."
#endif
						};
	}
	
	if (lua_type(State, -1) != LUA_TTABLE)
	{ //NewLuaConationStream() should return a table (a lua ConationStream)
		lua_close(State);
		throw ScriptError { ScriptName, FunctionName

#ifdef DEBUG
						, "C prerequesite function NewLuaConationStream() did not return a Lua ConationStream."
#endif
						};
	}
	
	///Now we have the Lua-ified ConationStream on the stack, and we're gonna use it as the only argument to our script function.
	
	//Find our function.
	lua_getglobal(State, FunctionName);
	
	if (lua_type(State, -1) != LUA_TFUNCTION)
	{
		lua_close(State);
		throw ScriptError { ScriptName, FunctionName
#ifdef DEBUG
						, "Requested Lua script function is not actually a function or does not exist."
#endif
						};
	}
	
	lua_pushvalue(State, -2);
	
	//Call the script function.
	if (lua_pcall(State, 1, 1, 0) != LUA_OK)
	{
		luaL_traceback(State, State, nullptr, 1);
		
		const VLString Traceback { lua_tostring(State, -1) };
		
		VLDEBUG("FAILURE IN SCRIPTING CORE, got traceback: " + Traceback);

		lua_close(State);
		
		throw ScriptError { ScriptName, FunctionName
#ifdef DEBUG
						, VLString("Lua error while calling lua_pcall() for function: ") + Traceback
#endif
						};
	}
	
	//If it returns some weird value, I guess just assume it worked.
	const bool FinalResult = lua_type(State, -1) == LUA_TBOOLEAN ? lua_toboolean(State, -1) : true;
	
	lua_close(State);
	
	return FinalResult;
}

VLThreads::Thread *Script::SpawnInitScript(void)
{
	if (!IdentityModule::GetInitScript()) return nullptr;
	
	VLThreads::Thread *Thd = new VLThreads::Thread(
	[] (void *) -> void*
	{	
		lua_State *State = InitScript(nullptr);
		
		if (State) lua_close(State);
		
		return (void*)(State != nullptr);
	}, nullptr);
	
	Thd->Start();
	
	return Thd;
}

