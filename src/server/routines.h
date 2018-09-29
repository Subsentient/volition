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


#ifndef _VL_SERVER_ROUTINES_H_
#define _VL_SERVER_ROUTINES_H_

/*
After one but before the next,
In a tree where new life writhed and flexed
Two birds emerged to hold the world
And grow from feeble talons curled

But fate see them part that day
As lightning cut their branch away
By winds to distant places sent
Almost as if it all had meant

That though their bloodied wounds would fade,
They'd wonder where their brother lay
For every day from that day on
They'd wait to hear a certain song

In vain, for years, thought one was strong
And one was not, for far too long
Until his bones and thoughts were old,
And feathers burnt and lost and cold

The stronger of the two could see
A distant bird, how weak was he
In drawing near but knowing not,
Just who he was, or why, or what

The stronger talons tore at flesh
And stripped away that feathered mess
And all without a sound or cry,
Or even ever knowing why

Yet as the sun began to sink
He seemed to sense, he seemed to think
That soon his brother might appear
From somewhere close, from somewhere near
Convinced this was his brother's fate,
Above his corpse, he sat... to wait
*/


#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "clients.h"

namespace Routines
{
	enum RoutineFlags : uint8_t
	{
		SCHEDFLAG_INVALID = 0,
		SCHEDFLAG_RUNONCE = 1 << 0,
		SCHEDFLAG_ONCONNECT = 1 << 1,
		SCHEDFLAG_DISABLED = 1 << 2,
	};
	
	struct RoutineInfo
	{
		VLString Name;
		std::vector<VLString> Targets;
		VLString Schedule;
		uint8_t Flags;
	};
	
	void ProcessScheduledRoutines(const time_t CurrentTime);
	bool ScanRoutineDB(void);
	uint64_t AllocateRoutineID(void);
	void ProcessOnConnectRoutines(Clients::ClientObj *Node);
	void AddRoutine(const RoutineInfo *Routine);
	bool DeleteRoutine(const VLString &RoutineName);
	RoutineInfo *LookupRoutine(const VLString &RoutineName);
	VLString CollateRoutineList(uint32_t *const NumRoutinesOut = nullptr);
}

#endif //_VL_SERVER_ROUTINES_H_
