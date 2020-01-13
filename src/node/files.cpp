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
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "files.h"

NetCmdStatus Files::Delete(const char *Path)
{ //Try to do as much of our needed allocation on the heap as possible to allow lots of recursion.
	VLScopedPtr<struct stat*> FileStat = new struct stat();
	
	if (stat(Path, FileStat) != 0)
	{
		return NetCmdStatus(false, STATUS_MISSING);
	}
	

	//Just a file.
	if (!S_ISDIR(FileStat->st_mode))
	{
		return !unlink(Path);
	}
	
	
	//Directory.
	DIR *CurDir = opendir(Path);
	
	if (!CurDir)
	{
		return false;
	}
	struct dirent *DirPtr = nullptr;
	bool AllSucceeded = true;
	
	while ((DirPtr = readdir(CurDir)))
	{
		const VLString &NewPath = VLString(Path) + PATHSEP + (const char*)DirPtr->d_name;
		
		if (!Files::Delete(NewPath)) AllSucceeded = false;
		
	}
	
	closedir(CurDir);
	
	return AllSucceeded && !rmdir(Path);
}

#define CHUNK_SIZE (1024*1024*4) //4MB

static bool CopyFileSub(const char *Source, const char *Destination)
{
	VLScopedPtr<struct stat*> FileStat = new struct stat();
	
	if (stat(Source, FileStat) != 0 ||
		S_ISDIR(FileStat->st_mode))
	{
		VLDEBUG("stat() of source failed.");
		return false;
	}
	
	const size_t Size = FileStat->st_size;
	
	
	VLScopedPtr<FILE*, int(*)(FILE*)> InDesc { fopen(Source, "rb"), fclose };
	VLScopedPtr<FILE*, int(*)(FILE*)> OutDesc { fopen(Destination, "wb"), fclose };
	
	if (!InDesc || !OutDesc)
	{
		VLDEBUG("Failed to open a descriptor. InDesc open: " + VLString::IntToString((bool)InDesc) + " OutDesc open: " + VLString::IntToString((bool)InDesc));
		return false;
	}
	
	size_t TotalRead = 0;
	size_t Read = 0;
	
	VLScopedPtr<uint8_t*> Buffer { new uint8_t[CHUNK_SIZE], VL_ALLOCTYPE_ARRAYNEW };
	
	for (; TotalRead < Size; TotalRead += Read)
	{
		Read = fread(Buffer, 1, CHUNK_SIZE, InDesc);
		
		if (ferror(InDesc))
		{
	LoopFailed:

			VLDEBUG("InDesc tripped ferror()");
			return false;
		}
		
		if (!Read) goto LoopFailed; //We're using the loop condition to test this, we shouldn't get here!
		
		if (fwrite(Buffer, 1, Read, OutDesc) < 0) goto LoopFailed;
		
		if (ferror(OutDesc))
		{
			VLDEBUG("OutDesc tripped ferror()");
			goto LoopFailed;
		}
		
	}
		
	return true;
	
}


bool Files::Copy(const char *Source, const char *Destination)
{
	
	VLDEBUG("Source: \"" + Source + "\", Destination: \"" + Destination + "\"");
	
	VLScopedPtr<struct stat *> FileStat { new struct stat() };
	
	if (stat(Source, FileStat) != 0)
	{
		return false;
	}
	
	if (!S_ISDIR(FileStat->st_mode))
	{ //Regular file.
		VLDEBUG("Copying regular file \"" + Source + "\" to \"" + Destination + "\".");
		return CopyFileSub(Source, Destination);
	}
	
	//Directory
	
	//Create destination directory.
#ifdef WIN32
	if (mkdir(Destination) != 0)
#else
	if (mkdir(Destination, 0755) != 0)
#endif //WIN32
	{
		return false;
	}
	
	VLScopedPtr<DIR*, decltype(&closedir)> CurDir { opendir(Source), closedir };
	
	if (!CurDir) return false;
	
	struct dirent *DirPtr = nullptr;
	
	while ((DirPtr = readdir(CurDir)))
	{
		const VLString &NewOldPath { VLString(Source) + PATHSEP + (const char*)DirPtr->d_name };
		const VLString &NewNewPath { VLString(Destination) + PATHSEP + (const char*)DirPtr->d_name };
		
		//Reuse the pointer declared in the outer scope.
		VLScopedPtr<struct stat*> FileStat { new struct stat() };
		
		//Get file type.
		if (stat(NewOldPath, FileStat) != 0)
		{
			return false;
		}
		
		if (S_ISDIR(FileStat->st_mode))
		{ //Directory
			if (!Files::Copy(NewOldPath, NewNewPath))
			{
				return false;
			}
		}
		else
		{
			if (!CopyFileSub(NewOldPath, NewNewPath))
			{
				return false;
			}
		}
	}

	return true;	
}

bool Files::Move(const char *Source, const char *Destination)
{
	int Status = rename(Source, Destination); //Try an easy rename.
	
	if (Status == 0) return true; //Worked.
	
	//Something went wrong.
	
	if (errno == EXDEV) //Target isn't on the same filesystem.
	{
		if (!Files::Copy(Source, Destination)) return false; //Can't do it.
		
		if (!Files::Delete(Source)) return false;
		
		return true;
	}
	
	//Another error occurred.
	return false;

}

bool Files::Chdir(const char *NewWD)
{
	return !chdir(NewWD);
}

VLString Files::GetWorkingDirectory(void)
{
	char CWD[2048] {};
	getcwd(CWD, sizeof CWD);

	return CWD;
}


std::list<Files::DirectoryEntry> *Files::ListDirectory(const char *Path)
{
	if (!Utils::IsDirectory(Path)) return nullptr; //Not a directory, so wtf are you doing
	
	VLScopedPtr<DIR*, decltype(&closedir)> Dir { opendir(Path), closedir };

	if (!Dir) return nullptr;
	
	struct dirent *Ptr = nullptr;

	std::list<Files::DirectoryEntry> *RetVal = new std::list<Files::DirectoryEntry>;
	
	while ((Ptr = readdir(Dir)))
	{
		const VLString &FullPath = VLString(Path) + PATHSEP + (const char*)Ptr->d_name;
		
		RetVal->push_back({FullPath, Utils::IsDirectory(FullPath)});
	}

	return RetVal;
}
