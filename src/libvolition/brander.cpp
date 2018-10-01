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


//I wrote this code when I was tired and didn't really give a fuck, so I did a lot of things
//in hideous poor practice ways. I'm not sorry.

#include "include/common.h"
#include "include/utils.h"
#include "include/conation.h" //For split peas encoding

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include <map>

#include "include/brander.h"

static std::map<Brander::AttributeTypes, VLString> DelimMap =	{
																	{ Brander::AttributeTypes::IDENTITY, NODE_IDENTITY_DELIMSTART },
																	{ Brander::AttributeTypes::SERVERADDR, NODE_SERVERADDR_DELIMSTART },
																	{ Brander::AttributeTypes::REVISION, NODE_REVISION_DELIMSTART },
																	{ Brander::AttributeTypes::PLATFORMSTRING, NODE_PLATFORM_STRING_DELIMSTART },
																	{ Brander::AttributeTypes::AUTHTOKEN, NODE_AUTHTOKEN_DELIMSTART },
																	{ Brander::AttributeTypes::CERT, NODE_CERT_DELIMSTART },
																	{ Brander::AttributeTypes::COMPILETIME, NODE_COMPILETIME_DELIMSTART },
																};
																
static std::map<Brander::AttributeTypes, size_t> CapacityMap =	{
																	{ Brander::AttributeTypes::IDENTITY, NODE_IDENTITY_CAPACITY },
																	{ Brander::AttributeTypes::SERVERADDR, NODE_SERVERADDR_CAPACITY },
																	{ Brander::AttributeTypes::REVISION, NODE_REVISION_CAPACITY },
																	{ Brander::AttributeTypes::PLATFORMSTRING, NODE_PLATFORM_STRING_CAPACITY },
																	{ Brander::AttributeTypes::AUTHTOKEN, NODE_AUTHTOKEN_CAPACITY },
																	{ Brander::AttributeTypes::CERT, NODE_CERT_CAPACITY },
																	{ Brander::AttributeTypes::COMPILETIME, NODE_COMPILETIME_CAPACITY },
																};


static char *ScanForDelim(const char *Buf, const size_t FileSize, const char *DelimStart, const size_t DelimStartLength)
{
	const char *const Stopper = Buf + FileSize;
	
	const char *Worker = Buf;

	for (; Worker != Stopper; ++Worker)
	{
		if (!memcmp(Worker, DelimStart, DelimStartLength + 1))
		{ //Let us re-qualify it ourselves if we must.
			return const_cast<char*>(Worker + DelimStartLength + 1); //We never purge the null terminator
		}
	}

	return nullptr;
}

bool Brander::BrandBinaryViaBuffer(void *Buf, const size_t BufSize, const std::map<Brander::AttributeTypes, Brander::AttrValue> &Values)
{
	for (auto Iter = Values.begin(); Iter != Values.end(); ++Iter)
	{
		char *Search = ScanForDelim(static_cast<char*>(Buf), BufSize, DelimMap[Iter->first], DelimMap[Iter->first].length());
		
		if (!Search)
		{
			return false;
		}
		
		switch (Iter->first)
		{
			case Brander::AttributeTypes::COMPILETIME:
			{ //We store time for this, not strings
				if (Iter->second.Get_ValueType() != Brander::AttrValue::TYPE_INT64)
				{
					return false;
				}
				
				int64_t Time = Iter->second.Get_Int64();
				*(uint64_t*)&Time = Utils::vl_htonll(*(uint64_t*)&Time);
				memcpy(Search, &Time, sizeof Time);
				break;
			}
			default:
				if (Iter->second.Get_ValueType() != Brander::AttrValue::TYPE_STRING)
				{
					return false;
				}
				
				strncpy(Search, Iter->second.Get_String(), CapacityMap[Iter->first] - DelimMap[Iter->first].length() - 3);
				Search[CapacityMap[Iter->first] - DelimMap[Iter->first].length() - 3] = '\0';
				break;
		}
		
		
	}
	
	return true;
}

bool Brander::BrandBinaryViaFile(const char *Path, std::map<Brander::AttributeTypes, Brander::AttrValue> &Values)
{
	if (!Utils::FileExists(Path)) return false;
	
	uint64_t FileSize;
	if (!Utils::GetFileSize(Path, &FileSize)) return false;
	
	std::vector<char> Buf;
	Buf.resize(FileSize);
	
	if (!Utils::Slurp(Path, Buf.data(), FileSize)) return false;
	

	if (!BrandBinaryViaBuffer(Buf.data(), FileSize, Values)) return false;
	
	if (!Utils::WriteFile(Path, Buf.data(), FileSize))
	{
		return false;
	}
	
	return true;
}

Brander::AttrValue *Brander::ReadBrandedBinaryViaFile(const char *Path, const AttributeTypes Attribute)
{
	if (!Utils::FileExists(Path)) return nullptr;
	
	uint64_t FileSize;
	if (!Utils::GetFileSize(Path, &FileSize)) return nullptr;
	
	std::vector<char> Buf;
	Buf.resize(FileSize);
	
	if (!Utils::Slurp(Path, Buf.data(), FileSize)) return nullptr;
	
	return ReadBrandedBinaryViaBuffer(Buf.data(), Buf.size(), Attribute);
}

Brander::AttrValue *Brander::ReadBrandedBinaryViaBuffer(const void *Buf, const size_t BufSize, const AttributeTypes Attribute)
{
	const char *Search = ScanForDelim(static_cast<const char*>(Buf), BufSize, DelimMap[Attribute], DelimMap[Attribute].length());
	
	if (!Search) return nullptr; //Not found;
	
	AttrValue *RetVal = nullptr;
	
	switch (Attribute)
	{
		case AttributeTypes::COMPILETIME:
		{
			int64_t Time = 0;
			
			memcpy(&Time, Search, sizeof Time);
			
			Time = Utils::vl_ntohll(Time);
			
			RetVal = new AttrValue(Time);
			break;
		}
		default:
			VLString Value(strlen(Search) + 1);
			Value = Search;
			
			RetVal = new AttrValue(Value);
			break;
	}
	
	return RetVal;
}
