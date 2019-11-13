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
#include "../libvolition/include/utils.h"

#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <map>

static const std::map<Logger::ItemType, VLString> ItemTypeNames
{
	{ Logger::LOGITEM_INVALID, "(undefined event type)" },
	{ Logger::LOGITEM_CONN, "CONNECTIONS" },
	{ Logger::LOGITEM_INFO, "INFO" },
	{ Logger::LOGITEM_PERMS, "PERMS" },
	{ Logger::LOGITEM_SYSERROR, "ERROR" },
	{ Logger::LOGITEM_SYSWARN, "WARNING" },
	{ Logger::LOGITEM_SECUREERROR, "SECURITY ERROR" },
	{ Logger::LOGITEM_SECUREWARN, "SECURITY WARNING" },
	{ Logger::LOGITEM_ROUTINEERROR, "ROUTINE ERROR" },
	{ Logger::LOGITEM_ROUTINEWARNING, "ROUTINE WARNING" },
};

const char LOG_FILENAME[] = "server.log";

bool Logger::WriteLogLine(const Logger::ItemType Type, const char *Text)
{
	FILE *Descriptor = fopen(LOG_FILENAME, "ab");
	
	if (!Descriptor) return false;
	
	VLString String(128);
	
	const time_t CurrentTime = time(nullptr);
	
	struct tm TimeStruct = *localtime(&CurrentTime);
	
	strftime(String.GetBuffer(), String.GetCapacity(), "[%a %Y-%m-%d | %I:%M:%S %p] ", &TimeStruct);

	String += ItemTypeNames.at(ItemTypeNames.count(Type) ? Type : LOGITEM_INVALID) + ": ";
	String += Text;
	String += '\n';

#ifdef DEBUG //Debug mode, we want all log entries printed to the screen.
	fputs(String, stdout);
#endif

	fwrite(String, 1, String.Length(), Descriptor);
	fclose(Descriptor);
	
	return true;
}

VLString Logger::TailLog(const size_t NumLinesFromLast)
{
	uint64_t FileSize = 0;

	if (!Utils::GetFileSize(LOG_FILENAME, &FileSize)) return VLString();

	VLString RetVal(FileSize);
	
	if (!Utils::Slurp(LOG_FILENAME, RetVal.GetBuffer(), RetVal.GetCapacity())) return VLString();
	
	VLScopedPtr<std::vector<VLString>*> Lines { Utils::SplitTextByCharacter(RetVal, '\n') };
	
	RetVal.Wipe(false);

	const size_t NeededLines = Lines->size() < NumLinesFromLast ? Lines->size() - 1 : NumLinesFromLast - 1;

	for (size_t Inc = (Lines->size() - 1) - NeededLines; Inc < Lines->size(); ++Inc)
	{
		RetVal += Lines->at(Inc) + '\n';
	}
	
	return RetVal;
}

bool Logger::WipeLog(void)
{
	return !remove(LOG_FILENAME);
}

size_t Logger::GetLogSize(void)
{
	uint64_t RetVal = 0;

	Utils::GetFileSize(LOG_FILENAME, &RetVal);

	return RetVal;
}

VLString Logger::ReportArgsToText(Conation::ConationStream *Stream)
{
	VLString RetVal(8192);
	
	VLScopedPtr<std::vector<Conation::ArgType>*> ArgTypes = Stream->GetArgTypes();
	
	for (size_t Inc = 0; Inc < ArgTypes->size(); ++Inc)
	{
		RetVal += ArgTypeToString(ArgTypes->at(Inc)) + ":";
		
		switch (ArgTypes->at(Inc))
		{
			case Conation::ARGTYPE_BOOL:
			{
				RetVal += Stream->Pop_Bool() ? "true" : "false";
				break;
			}
			case Conation::ARGTYPE_STRING:
			{
				RetVal += VLString("\"") + Stream->Pop_String() + "\"";
				break;
			}
			case Conation::ARGTYPE_FILEPATH:
			{
				RetVal += VLString("\"") + Stream->Pop_FilePath() + "\"";
				break;
			}
			case Conation::ARGTYPE_NETCMDSTATUS:
			{
				const NetCmdStatus &Ref = Stream->Pop_NetCmdStatus();
				
				RetVal += StatusCodeToString(Ref.Status) + ":\"" + Ref.Msg + "\"";
				break;
			}
			case Conation::ARGTYPE_ODHEADER:
			{
				const Conation::ConationStream::ODHeader &Hdr = Stream->Pop_ODHeader();
				
				RetVal += Hdr.Origin + ":" + Hdr.Destination;
				break;
			}
			case Conation::ARGTYPE_INT64:
			{
				RetVal += VLString::IntToString(Stream->Pop_Int64());
				break;
			}
			case Conation::ARGTYPE_UINT64:
			{
				RetVal += VLString::UintToString(Stream->Pop_Uint64());
				break;
			}
			case Conation::ARGTYPE_INT32:
			{
				RetVal += VLString::IntToString(Stream->Pop_Int32());
				break;
			}
			case Conation::ARGTYPE_UINT32:
			{
				RetVal += VLString::UintToString(Stream->Pop_Uint32());
				break;
			}
			default:
			{
				RetVal += "<unconvertable>";
				break;
			}
		}
		
		//Add argument separator
		if (Inc + 1 < ArgTypes->size()) RetVal += ", ";
	}
	
	RetVal.ShrinkToFit();
	
	return RetVal;
}
