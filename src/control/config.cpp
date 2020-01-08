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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include "config.h"

//Globals
static std::map<VLString, VLString> ConfigKeys;

//Prototypes
static bool VerifyConfig(void);

//Function definitions
VLString Config::GetKey(const char *Keyname)
{
	if (!ConfigKeys.count(Keyname)) return VLString(); //Empty VLStrings can be boolean tested.
	return ConfigKeys[Keyname];
}

#if defined(WIN32) && !defined(FORCE_POSIX_NEWLINES)
#define NEWLINE_SEQUENCE "\r\n"
#define NEWLINE_SEQUENCE_LEN 2
#else
#define NEWLINE_SEQUENCE "\n"
#define NEWLINE_SEQUENCE_LEN 1
#endif
 
bool Config::ReadConfig(const VLString &ConfigDir)
{
	std::vector<uint8_t> *FileData = Utils::Slurp(ConfigDir + PATH_DIVIDER_STRING + CONFIGFILE);

	if (!FileData)
	{
		VLERROR("Failed to slurp config file \"" + (ConfigDir + PATH_DIVIDER_STRING CONFIGFILE "\"."));
		return false;
	}

	const char *Worker = reinterpret_cast<const char*>(&FileData->at(0));

	VLString Line(4096);
	
	char Key[256];
	char Value[4096];
	
	do
	{ ///God I hate working with strings in C/C++....
		Line.Wipe(false);
		
		while (strpbrk(Worker, NEWLINE_SEQUENCE) == Worker) ++Worker;
		
		if (!*Worker) continue;
		
		//Get line.
		size_t Inc = 0;
		
		for (; Inc < FileData->size() - 1 && Worker[Inc] && !isspace(Worker[Inc]); ++Inc)
		{
			Line += Worker[Inc];
		}
		
		char *Search = (char*)strchr(Line, '=');

		if (!Search) goto Failure;

		*Search = '\0';

		strncpy(Key, Line, sizeof Key - 1); Key[sizeof Key - 1] = '\0';
		strncpy(Value, Search + 1, sizeof Value - 1); Value[sizeof Value - 1] = '\0';

		//Set the value.
		ConfigKeys[Key] = Value;

		
	} while ((Worker = strpbrk(Worker, NEWLINE_SEQUENCE)));
		

	delete FileData;

	//Make sure we have everything we need.
	if (!VerifyConfig())
	{
		ConfigKeys.clear();
		return false;
	}
	
	return true;

Failure:
	delete FileData;
	return false;
}

static bool VerifyConfig(void)
{
	if (!ConfigKeys.count("ServerName") ||
		!ConfigKeys.count("DownloadDirectory") ||
		!ConfigKeys.count("ScriptsDirectory") ||
		!ConfigKeys.count("RootCertificate") )
	{
		return false;
	}

	return true;
}

