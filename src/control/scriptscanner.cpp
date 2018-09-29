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
#include "../libvolition/include/conation.h"

#include "config.h"
#include "scriptscanner.h"

#include <map>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

#define SCRIPTHEADER_DELIM_START "VLSI_BEGIN_SPEC"
#define SCRIPTHEADER_DELIM_END "VLSI_END_SPEC"

#define FUNC_SPECIFIER "function::"
#define SCRIPTNAME_SPECIFIER "scriptname::"
#define SCRIPTVER_SPECIFIER "scriptversion::"
#define SCRIPTDESC_SPECIFIER "scriptdescription::"

static std::map<VLString, ScriptScanner::ScriptInfo*> KnownScripts;

static ScriptScanner::ScriptInfo::ScriptFunctionInfo *ScanFunction(const char *const Line_)
{
	VLString Line = Line_;
	
	if (!Line.StartsWith(FUNC_SPECIFIER)) return nullptr;
	
	Line += sizeof(FUNC_SPECIFIER) - 1;
	
	VLString Name(Line.GetCapacity());
	//Get function name
	for (; *Line && *Line != '|'; ++Line, ++Name)
	{
		*Name = *Line;
	}
	*Name = '\0';
	
	Name.Rewind();
	Name.ShrinkToFit();
	
	auto FuncInfo = new ScriptScanner::ScriptInfo::ScriptFunctionInfo{};
	
	FuncInfo->Name = Name;
#ifdef DEBUG
	puts(VLString("ScanFunction(): Name detected as ") + FuncInfo->Name);
#endif
	if (!Line) //No description or parameters specified, and that's ok.
	{
		return FuncInfo;
	}
	
	++Line; //Get past the '|'
	
	VLString Description(Line.GetCapacity());
	//Get function description
	for (; *Line && *Line != '|'; ++Line, ++Description)
	{
		*Description = *Line;
	}
	*Description = '\0';
	
	Description.Rewind();
	Description.ShrinkToFit();
	
	FuncInfo->Description = Description;

#ifdef DEBUG
	puts(VLString("ScanFunction(): Description detected as ") + FuncInfo->Description);
#endif
	if (!Line)
	{
		return FuncInfo;
	}
	
	++Line; //Get past the '|'

	//Get parameters
	while (Line)
	{
		const bool Optional = *Line == '~';
		
		if (*Line == '~') ++Line;
		
		VLString Param(Line.GetCapacity());
		
		for (; *Line && *Line != ','; ++Line, ++Param)
		{
			*Param = *Line;
		}
		*Param = '\0';
		
		Param.Rewind();
		Param.ShrinkToFit();
		
		if (!Param) continue; //Empty parameter. Whatever.
		
#ifdef DEBUG
		puts(VLString("ScanFunction(): Parameter:") + Param);
#endif
		const Conation::ArgType Converted = StringToArgType(Param);
		
		if (Converted == Conation::ARGTYPE_NONE)
		{
			delete FuncInfo; //Fucked.
			return nullptr;
		}
	
		if (Optional)
		{
			//Oddly there's literally no way to make the bitwise assignment operators work on an enum.
			*(Conation::ArgTypeUint*)&FuncInfo->AllowedExtraParameters |= Converted;
		}
		else
		{
			FuncInfo->RequiredParameters.push_back(Converted);
		}
		
		if (*Line == ',') ++Line;
	}
	
	return FuncInfo;
	
}

static ScriptScanner::ScriptInfo *ScanFile(const char *Path)
{
	if (!Path || !*Path) return nullptr;
	
	uint64_t FileSize = 0ul;
	
	if (!Utils::GetFileSize(Path, &FileSize) || !FileSize) return nullptr;
	
	VLString Buffer(FileSize + 1);
	
	if (!Utils::Slurp(Path, Buffer.GetBuffer(), Buffer.GetCapacity()) || !Buffer) return nullptr;
	
	//Isn't it nice that VLString constructors can take a null pointer and interpret it as empty?
	VLString Text = Buffer.Find(SCRIPTHEADER_DELIM_START);
	
	if (!Text) return nullptr;
	
	//Skip past the header starting delimiter.
	Text += sizeof SCRIPTHEADER_DELIM_START - 1;
	
	Buffer.Wipe(true); //Might as well release this memory since we don't need dupes.
	
	char *End = const_cast<char*>(Text.Find(SCRIPTHEADER_DELIM_END));
	
	if (!End) return nullptr;
	
	*End = '\0';
	
	ScriptScanner::ScriptInfo *RetVal = new ScriptScanner::ScriptInfo;
	
	RetVal->ScriptPath = Path;
	
	const char *Worker = Text;
	do
	{
		while (*Worker == '\r' || *Worker == '\n' || isblank(*Worker)) ++Worker;

		//End of the header it seems.
		if (!*Worker) break;
		
		//If it's a comment, ignore it. We can have our own comments inside Lua's comments.
		if (*Worker == '#') continue;
		
		//Compute length of this line.
		const char *NextNewline = strpbrk(Worker, "\r\n");
		
		const size_t LineLength = NextNewline ? (NextNewline - Worker) : strlen(Worker);
		
		VLString Line(LineLength + 1);
		
		memcpy(Line.GetBuffer(), Worker, LineLength);
		Line[LineLength] = '\0'; //Not actually necessary since VLStrings zero out their own buffers first.
		
#ifdef DEBUG
		puts(VLString("ScriptScanner::ScanFile(): File \"") + Path + "\" line data: \"" + Line + '"');
#endif
		if (Line.StartsWith(FUNC_SPECIFIER))
		{
			ScriptScanner::ScriptInfo::ScriptFunctionInfo *Func = ScanFunction(Line);
			
			if (!Func) continue;
			Func->Parent = RetVal;
			
			RetVal->Functions[Func->Name] = Func;
		}
		else if (Line.StartsWith(SCRIPTNAME_SPECIFIER))
		{
			Line += sizeof SCRIPTNAME_SPECIFIER - 1;
			
			RetVal->ScriptName = +Line; //If we don't do the plus, then the whole VLString and its useless preceding data
										//gets copied into memory as the key, which sucks.
#ifdef DEBUG
			puts(VLString("ScriptScanner::ScanFile(): Loaded script name as \"") + RetVal->ScriptName + '"');
#endif
		}
		else if (Line.StartsWith(SCRIPTDESC_SPECIFIER))
		{
			Line += sizeof SCRIPTDESC_SPECIFIER - 1;
			
			RetVal->ScriptDescription = +Line;
#ifdef DEBUG
			puts(VLString("ScriptScanner::ScanFile(): Loaded script description as ") + RetVal->ScriptDescription);
#endif
		}
		else if (Line.StartsWith(SCRIPTVER_SPECIFIER))
		{
			Line += sizeof SCRIPTVER_SPECIFIER - 1;
			
			RetVal->ScriptVersion = +Line;
#ifdef DEBUG
			puts(VLString("ScriptScanner::ScanFile(): Loaded script version as ") + RetVal->ScriptVersion);
#endif
		}
		else
		{
			fprintf(stderr, "WARNING: Unrecognized syntax: \"%s\"\n", +Line);
			continue;
		}
		
	} while ((Worker = strpbrk(Worker, "\r\n")));
		
	return RetVal;
}

bool ScriptScanner::ScanScriptsDirectory(void)
{
	const VLString Directory = Config::GetKey("ScriptsDirectory");
	
	if (!Directory) return false;
	
	DIR *DirHandle = opendir(Directory);
	
	if (!DirHandle) return false;
	
	struct dirent *DirWorker = nullptr;
	struct stat FileStat{};
	
	while ((DirWorker = readdir(DirHandle)))
	{
		const VLString Path = Directory + PATH_DIVIDER + (const char*)DirWorker->d_name;

		if (stat(Path, &FileStat) != 0 ||
			S_ISDIR(FileStat.st_mode))
		{
			continue;
		}

		if (!VLString(DirWorker->d_name).EndsWith(".lua"))
		{
			continue;
		}
		
		
		ScriptInfo *Info = ScanFile(Path);
		
		if (!Info)
		{
			fprintf(stderr, "WARNING: Unable to scan script file \"%s\"!\n", DirWorker->d_name);
			continue;
		}
		
		KnownScripts[Info->ScriptName] = Info;
	}
	
	return true;
}
	
const std::map<VLString, ScriptScanner::ScriptInfo*> &ScriptScanner::GetKnownScripts(void)
{
	return KnownScripts;
}
