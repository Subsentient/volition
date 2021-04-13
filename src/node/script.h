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


#ifndef _VL_NODE_SCRIPT_H_
#define _VL_NODE_SCRIPT_H_

#include "../libvolition/include/common.h"
#include "../libvolition/include/vlthreads.h"
#include "jobs.h" //Because we need Jobs::Job

namespace Script
{
	enum LuaJobType : uint8_t
	{
		LUAJOB_INVALID = 0,
		LUAJOB_JOB		= 1,
		LUAJOB_STARTUP	= 1 << 1,
		LUAJOB_SNIPPET	= 1 << 2,
		LUAJOB_WORKER	= 1 << 3,
		LUAJOB_MAXVALUE = LUAJOB_WORKER
	};
	
	struct ScriptError
	{ //For exceptions.
		VLString Function;
		VLString Msg;
	};
	
	bool LoadScript(const char *ScriptBuffer, const char *ScriptName, const bool OverwritePermissible = false);
	bool UnloadScript(const char *ScriptName);
	bool ScriptIsLoaded(const char *ScriptName);
	void ExecuteStartupScript(Jobs::Job *const OurJob);
	bool ExecuteScriptFunction(const LuaJobType Type, const char *ScriptData, const char *FunctionName, Conation::ConationStream *Stream, Jobs::Job *OurJob);
	
	extern std::map<VLString, VLString> LoadedScripts;
}

#endif //_VL_NODE_SCRIPT_H_
