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

#include "../../libvolition/include/common.h"
#include "../../libvolition/include/utils.h"
#include "../../libvolition/include/brander.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include <map>


static std::map<VLString, Brander::AttributeTypes> ArgMap =	{
														{ "--id", Brander::AttributeTypes::IDENTITY },
														{ "--server", Brander::AttributeTypes::SERVERADDR },
														{ "--revision", Brander::AttributeTypes::REVISION },
														{ "--platformstring", Brander::AttributeTypes::PLATFORMSTRING },
														{ "--authtoken", Brander::AttributeTypes::AUTHTOKEN },
														{ "--cert", Brander::AttributeTypes::CERT },
														{ "--compiletime", Brander::AttributeTypes::COMPILETIME },
													};



static bool ReadBinary(const char *Binary)
{
	
	VLScopedPtr<std::vector<uint8_t>*> FileBuffer { Utils::Slurp(Binary) };
	
	if (!FileBuffer) return false;
	
	//Get identity
	VLScopedPtr<Brander::AttrValue*> Value { Brander::ReadBrandedBinaryViaBuffer((const char*)FileBuffer->data(), FileBuffer->size(), Brander::AttributeTypes::IDENTITY) };
	
	if (!Value)
	{
		return false;
	}
	printf("Identity: \"%s\"\n", +Value->Get_String());
	
	//Get server address
	Value = Brander::ReadBrandedBinaryViaBuffer((const char*)FileBuffer->data(), FileBuffer->size(), Brander::AttributeTypes::SERVERADDR);
	
	if (!Value)
	{
		return false;
	}

	printf("Server address: \"%s\"\n", +Value->Get_String());
	
	//Get revision
	Value = Brander::ReadBrandedBinaryViaBuffer((const char*)FileBuffer->data(), FileBuffer->size(), Brander::AttributeTypes::REVISION);

	if (!Value)
	{
		return false;
	}
	
	printf("Revision: \"%s\"\n", +Value->Get_String());

	//Get authentication token
	Value = Brander::ReadBrandedBinaryViaBuffer((const char*)FileBuffer->data(), FileBuffer->size(), Brander::AttributeTypes::AUTHTOKEN);
	
	if (!Value)
	{
		return false;
	}
		
	printf("Authentication token:  \"%s\"\n", +Value->Get_String());
	
	
	//Get platform string
	Value = Brander::ReadBrandedBinaryViaBuffer((const char*)FileBuffer->data(), FileBuffer->size(), Brander::AttributeTypes::PLATFORMSTRING);

	if (!Value)
	{
		return false;
	}

	printf("Platform string: \"%s\"\n", +Value->Get_String());
	
	//Get compile time;
	Value = Brander::ReadBrandedBinaryViaBuffer((const char*)FileBuffer->data(), FileBuffer->size(), Brander::AttributeTypes::COMPILETIME);

	if (!Value)
	{
		return false;
	}
	
	printf("Compile time: \"%llu\"\n", (unsigned long long)Value->Get_Int64());
	
	//Done
	return true;
}

int main(const int argc, const char **argv)
{
	VLString Binary;
	std::map<Brander::AttributeTypes, Brander::AttrValue> Values{};

	if (argc < 3)
	{
		fprintf(stderr, "Incorrect number of arguments %d\n", argc);
		exit(1);
	}
	
	Binary = argv[1];

	//We want the contents.
	if (VLString(argv[2]) == "--read")
	{
		return ReadBinary(Binary);
	}
			
	int Inc = 2;

	for (; Inc < argc; ++Inc)
	{
		const char *String = argv[Inc];

		if (!ArgMap.count(String))
		{
			fprintf(stderr, "Invalid argument \"%s\"\n", String);
			exit(1);
		}
		
		const char *Argument = argv[++Inc];

		switch (ArgMap[String])
		{
			case Brander::AttributeTypes::COMPILETIME:				
				Values.emplace(ArgMap[String], atoll(Argument));
				break;
			case Brander::AttributeTypes::CERT:
				Values.emplace(ArgMap[String], (const char*)(VLScopedPtr<std::vector<uint8_t>*>(Utils::Slurp(Argument))->data()));
				break;
			default:
				Values.emplace(ArgMap[String], Argument);
				break;
		}
	}
	
	const bool Result = Brander::BrandBinaryViaFile(Binary, Values);
	
	puts(VLString("Branding of binary ") + Binary + " " + (Result ? "succeeded." : "failed."));
}
	
