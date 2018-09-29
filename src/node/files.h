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


#ifndef __VLNODE_FILES_H__
#define __VLNODE_FILES_H__


#include "../libvolition/include/common.h"
#include <list>

#ifndef WIN32
#define PATHSEP "/"
#else
#define PATHSEP "\\"
#endif //WIN32

namespace Files
{
	NetCmdStatus Delete(const char *Path);
	bool Copy(const char *Source, const char *Destination);
	bool Move(const char *Source, const char *Destination);
	bool Chdir(const char *NewWD);
	VLString GetWorkingDirectory(void);
	
	struct DirectoryEntry
	{
		VLString Path;
		bool IsDirectory;
	};

	std::list<DirectoryEntry> *ListDirectory(const char *Path);
}

#endif //__VLNODE_FILES_H__
