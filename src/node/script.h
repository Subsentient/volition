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
#include "jobs.h" //Because we need JobReadQueue class.

namespace Script
{
	struct ScriptError
	{ //For exceptions.
		VLString ScriptName;
		VLString Function;
		VLString Msg;
	};
	
	bool LoadScript(const char *ScriptBuffer, const char *ScriptName, const bool OverwritePermissible = false);
	bool UnloadScript(const char *ScriptName);
	bool ScriptIsLoaded(const char *ScriptName);
	VLThreads::Thread *SpawnInitScript(void);
	bool ExecuteScriptFunction(const char *ScriptName, const char *FunctionName, Conation::ConationStream *Stream, Jobs::Job *OurJob);
}

#endif //_VL_NODE_SCRIPT_H_
