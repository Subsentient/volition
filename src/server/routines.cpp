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
Silent but distant,
Real but not seen
Here but not happening,
Forever has been

Into this absence,
Two had but strayed
Retracing in circles,
They surrendered, dismayed

Behind them, darkness
Ahead, the unknown
So they stood, uninvited
But the roots had grown

An error, a tangent
A curious mind
An instant, a lifetime
A secret to find

The dream and the waking
Occurring together
Realising, then
That it mattered not whether

Imagined movements
Near shimmering webs
The nightfall descended
To rest by their heads

Their feet planted firmly
Neath four darting eyes
With pupils dilated,
They counted the sighs

Their legs fading into
A curdling moss
Their lungs slowly filling
With decades of loss

That scent from the forest,
Not taking, not giving
They knew only this -
Not buried, not living

Their stagnating veins
Now hidden from view
In skin for the willow
And bone for the dew

The dream and the waking
Ending together
Understanding then
It had not mattered, ever
*/

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/utils.h"

#include "routines.h"
#include "db.h"
#include "logger.h"
#include "clients.h"

#include <list>

//Types
enum TimeField : uint8_t
{
	TIMEFIELD_INVALID = 0,
	TIMEFIELD_WEEKDAY,
	TIMEFIELD_YEAR,
	TIMEFIELD_MONTH,
	TIMEFIELD_DAY,
	TIMEFIELD_HOUR,
	TIMEFIELD_MINUTE,
	TIMEFIELD_SECOND,
	TIMEFIELD_MAX
};


//Globals
std::list<Routines::RoutineInfo> KnownRoutines;
static uint64_t RoutineIDCounter = 0;

//Prototypes
static void ExecuteOnConnectRoutine(Routines::RoutineInfo *Routine, Clients::ClientObj *Node);
static void ExecuteScheduledRoutine(Routines::RoutineInfo *Routine);
static inline bool TimeFieldMatches(const time_t Time, const char *Text, const TimeField Field);

//Function definitions
static inline bool TimeFieldMatches(const time_t Time, const char *Text, const TimeField Field)
{
	static struct tm TimeStruct;
	static time_t PreviousTime;

	if (PreviousTime != Time)
	{
		PreviousTime = Time;
		TimeStruct = *localtime(&Time);
	}

	
	if (*Text == '*') return true;

	VLScopedPtr<std::vector<VLString>*> Subfields = Utils::SplitTextByCharacter(Text, ',');
	
	for (size_t Inc = 0u; Inc < Subfields->size(); ++Inc)
	{
		switch (Field)
		{
			case TIMEFIELD_WEEKDAY:
				if (TimeStruct.tm_wday + 1 == atoi(Subfields->at(Inc)))
				{
					return true;
				}
				break;
			case TIMEFIELD_YEAR:
				if (TimeStruct.tm_year + 1900 == atoi(Subfields->at(Inc)))
				{
					return true;
				}
				break;
			case TIMEFIELD_MONTH:
				if (TimeStruct.tm_mon + 1 == atoi(Subfields->at(Inc)))
				{
					return true;
				}
				break;
			case TIMEFIELD_DAY:
				if (TimeStruct.tm_mday == atoi(Subfields->at(Inc)))
				{
					return true;
				}
				break;
			case TIMEFIELD_HOUR:
				if (TimeStruct.tm_hour == atoi(Subfields->at(Inc)))
				{
					return true;
				}
				break;
			case TIMEFIELD_MINUTE:
				if (TimeStruct.tm_min == atoi(Subfields->at(Inc)))
				{
					return true;
				}
				break;
			case TIMEFIELD_SECOND:
				if (TimeStruct.tm_sec == atoi(Subfields->at(Inc)))
				{
					return true;
				}
				break;
			default: //Invalid field, wtf
				return false;
		}
	}
	
	return false;
}

static bool CheckRoutineTimer(Routines::RoutineInfo *Routine, const time_t CurrentTime)
{
	VLScopedPtr<std::vector<VLString>*> Fields = Utils::SplitTextByCharacter(Routine->Schedule, ' ');

	for (uint8_t Inc = 1u; Inc < TIMEFIELD_MAX; ++Inc)
	{
		if (!TimeFieldMatches(CurrentTime, Fields->at(Inc - 1), static_cast<TimeField>(Inc)))
		{
			return false;
		}
	}

	return true;
}

bool Routines::ScanRoutineDB(void)
{
	VLScopedPtr<std::vector<DB::RoutineDBEntry>*> RoutineInfoList = DB::GetRoutineDBInfo();

	if (!RoutineInfoList) return false;

	KnownRoutines.clear(); //If you're calling ScanRoutineDB() more than once, making clearing necessary, you're probably a 'tard.
	
	for (size_t Inc = 0u; Inc < RoutineInfoList->size(); ++Inc)
	{
		AddRoutine(&RoutineInfoList->at(Inc));
	}

	return true;
}

void Routines::AddRoutine(const Routines::RoutineInfo *Routine)
{
	Routines::RoutineInfo *Target = nullptr;
	
	for (auto Iter = KnownRoutines.begin(); Iter != KnownRoutines.end(); ++Iter)
	{ //Check if we're updating.
		if (Iter->Name == Routine->Name)
		{
			Target = &*Iter;
			break;
		}
	}

	if (!Target)
	{
		KnownRoutines.push_back({});
		Target = &KnownRoutines.back();
	}
	
	*Target = *Routine;
}

bool Routines::DeleteRoutine(const VLString &RoutineName)
{
	for (auto Iter = KnownRoutines.begin(); Iter != KnownRoutines.end(); ++Iter)
	{
		if (Iter->Name == RoutineName)
		{
			KnownRoutines.erase(Iter);
			return true;
		}
	}
	
	return false;
}

uint64_t Routines::AllocateRoutineID(void)
{
	return ++RoutineIDCounter;
}


void Routines::ProcessOnConnectRoutines(Clients::ClientObj *Node)
{
	if (KnownRoutines.empty()) return;
	
	auto Iter = KnownRoutines.begin();

	for (; Iter != KnownRoutines.end(); ++Iter)
	{
		if (!(Iter->Flags & Routines::SCHEDFLAG_ONCONNECT) || (Iter->Flags & Routines::SCHEDFLAG_DISABLED)) continue;

		bool IsTarget = false;

		for (size_t Inc = 0u; Inc < Iter->Targets.size(); ++Inc)
		{
			if (Iter->Targets[Inc] == Node->GetID())
			{
				IsTarget = true;
				break;
			}
		}

		if (!IsTarget) continue;
		
		Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("On-connect activation triggered for routine ") + Iter->Name + " targeting node \"" + Node->GetID() + "\".");
		ExecuteOnConnectRoutine(&*Iter, Node);
	}
}

static void ExecuteScheduledRoutine(Routines::RoutineInfo *Routine)
{
	VLScopedPtr<DB::RoutineDBEntry*> OnDiskRoutine = DB::LookupRoutineDBEntry(Routine->Name);

	VLScopedPtr<std::vector<Conation::ArgType> *> Args = OnDiskRoutine->Stream.GetArgTypes();

	const bool HasODHeader = Args->at(0) == Conation::ARGTYPE_ODHEADER;
	
	Args.Release();

	//Build the header for a new stream.
	Conation::ConationStream::StreamHeader Hdr = OnDiskRoutine->Stream.GetHeader();

	//Set up the routine stream's header
	Hdr.CmdIdent = Routines::AllocateRoutineID();
	Hdr.CmdIdentFlags |= Conation::IDENT_ISROUTINE_BIT; //Flag this as a routine.
	Hdr.StreamArgsSize = 0; ///WE MUST SET THIS TO ZERO for the new stream constructor!


	if (HasODHeader)
	{ //Because we need to seek past it to make our own
		delete OnDiskRoutine->Stream.PopArgument();
	}
	
	for (size_t Inc = 0u; Inc < Routine->Targets.size(); ++Inc)
	{ ///Send to all targets.
		Clients::ClientObj *Node = Clients::LookupClient(Routine->Targets[Inc]);
		
		if (!Node)
		{
			Logger::WriteLogLine(Logger::LOGITEM_ROUTINEWARNING, VLString("No node with ID \"") + Routine->Targets[Inc] + "\" was found for routine \"" + Routine->Name + "\".");
			continue;
		}
		
		Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("Node \"") + Node->GetID() + "\" is a target for routine " + Routine->Name);

		Conation::ConationStream *NewStream = new Conation::ConationStream(Hdr, nullptr);

		if (HasODHeader)
		{ //Build a new ODHeader for the target node
			NewStream->Push_ODHeader("SERVER", Routine->Targets.at(Inc));
		}

		//Copy in the routine's argument data.
		NewStream->AppendArgData(OnDiskRoutine->Stream);

		Node->SendStream(NewStream);

	}

	Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("Processed time activated routine ") + Routine->Name + " and assigned routine ident #" + VLString::UintToString(Hdr.CmdIdent));

	if (Routine->Flags & Routines::SCHEDFLAG_RUNONCE)
	{ //After running once, we turn it off.
		OnDiskRoutine->Flags |= Routines::SCHEDFLAG_DISABLED;
		Routine->Flags |= Routines::SCHEDFLAG_DISABLED;
		OnDiskRoutine->Stream.Rewind(); //Necessary if we're going to write it back to disk.

		//Update on disk.
		DB::UpdateRoutineDB(OnDiskRoutine);
	}
}

static void ExecuteOnConnectRoutine(Routines::RoutineInfo *Routine, Clients::ClientObj *Node)
{
	VLScopedPtr<DB::RoutineDBEntry*> OnDiskRoutine = DB::LookupRoutineDBEntry(Routine->Name);

	VLScopedPtr<std::vector<Conation::ArgType> *> Args = OnDiskRoutine->Stream.GetArgTypes();

	const bool HasODHeader = Args ? Args->at(0) == Conation::ARGTYPE_ODHEADER : false;

	Args.Release();
	
	//Build the header for a new stream.
	Conation::ConationStream::StreamHeader Hdr = OnDiskRoutine->Stream.GetHeader();

	//Set up the routine stream's header
	Hdr.CmdIdent = Routines::AllocateRoutineID();
	Hdr.CmdIdentFlags |= Conation::IDENT_ISROUTINE_BIT; //Flag this as a routine.
	Hdr.StreamArgsSize = 0; ///WE MUST SET THIS TO ZERO for the new stream constructor!

	Conation::ConationStream *NewStream = new Conation::ConationStream(Hdr, nullptr);

	if (HasODHeader)
	{ //Get a new ODHeader
		delete OnDiskRoutine->Stream.PopArgument();
		NewStream->Push_ODHeader("SERVER", Node->GetID());
	}
		
	//Copy in the routine's argument data.
	NewStream->AppendArgData(OnDiskRoutine->Stream);

	VLASSERT(NewStream->GetCmdIdentFlags() & Conation::IDENT_ISROUTINE_BIT);
	
	Node->SendStream(NewStream);

	Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("Processed on-connect routine ") + Routine->Name + " and assigned routine ident #" + VLString::UintToString(Hdr.CmdIdent));

	if (Routine->Flags & Routines::SCHEDFLAG_RUNONCE)
	{ //After running once, we turn it off.
		OnDiskRoutine->Flags |= Routines::SCHEDFLAG_DISABLED;
		Routine->Flags |= Routines::SCHEDFLAG_DISABLED;
		OnDiskRoutine->Stream.Rewind(); //Necessary if we're going to write it back to disk.

		//Update on disk.
		DB::UpdateRoutineDB(OnDiskRoutine);
	}
}

void Routines::ProcessScheduledRoutines(const time_t CurrentTime)
{
	if (KnownRoutines.empty()) return;
	
	auto Iter = KnownRoutines.begin();

	for (; Iter != KnownRoutines.end(); ++Iter)
	{
		if (Iter->Flags & Routines::SCHEDFLAG_DISABLED || !Iter->Schedule) continue;

		if (CheckRoutineTimer(&*Iter, CurrentTime))
		{
			Logger::WriteLogLine(Logger::LOGITEM_INFO, VLString("Time activation triggered for routine ") + Iter->Name);
			ExecuteScheduledRoutine(&*Iter);
		}
	}	
}

VLString Routines::CollateRoutineList(uint32_t *const NumRoutinesOut)
{
	VLString RetVal(16384);
	
	for (auto Iter = KnownRoutines.begin(); Iter != KnownRoutines.end(); ++Iter)
	{
		VLString Item(2048);
		
		snprintf(Item.GetBuffer(),
				 Item.GetCapacity(),
				"Routine name: %s\n"
				"Target nodes: %s\n"
				"Schedule string: %s\n"
				"Flags integer: %s\n",
				+Iter->Name,
				+Utils::JoinTextByCharacter(&Iter->Targets, ','),
				+Iter->Schedule,
				+Utils::ToBinaryString(Iter->Flags));

		RetVal += Item;
		RetVal += "----------------------\n";
	}
	
	RetVal.ShrinkToFit();

	if (NumRoutinesOut != nullptr)
	{
		*NumRoutinesOut = KnownRoutines.size();
	}
	
	return RetVal;
}

Routines::RoutineInfo *Routines::LookupRoutine(const VLString &RoutineName)
{
	for (auto Iter = KnownRoutines.begin(); Iter != KnownRoutines.end(); ++Iter)
	{
		if (Iter->Name == RoutineName)
		{
			return &*Iter;
		}
	}

	return nullptr;
}
