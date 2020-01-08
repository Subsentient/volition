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

#include <map>
#include <vector>

#include "gui_icons.h"
static std::map<VLString, VLString> FilenameToIDMap =
										{
											{ "ctlnodeicon_android.png", "android" },
											{ "ctlnodeicon_freebsd.png", "freebsd" },
											{ "ctlnodeicon_haikuos.png", "haikuos" },
											{ "ctlnodeicon_linux.png", "linux" },
											{ "ctlnodeicon_openbsd.png", "openbsd" },
											{ "ctlnodeicon_osx.png", "macosx" },
											{ "ctlnodeicon_unknown.png", "unknown" },
											{ "ctlnodeicon_win.png", "windows" },
											{ "ctlsplash.png", "ctlsplash" },
											{ "ctlabout.png", "ctlabout" },
											{ "ctlwmicon.png", "ctlwmicon" },
										};

static std::map<VLString, const std::vector<uint8_t> *> IconMap;

bool GuiIcons::LoadAllIcons(const VLString &ConfigDir)
{
#ifdef DEBUG
	puts("GuiIcons::LoadAllIcons(): Will now load icons.");
#endif
	for (auto Iter = FilenameToIDMap.begin(); Iter != FilenameToIDMap.end(); ++Iter)
	{
		std::vector<uint8_t> *FileBuffer = Utils::Slurp(ConfigDir + PATH_DIVIDER_STRING "icons" PATH_DIVIDER_STRING + Iter->first);
		
		if (!FileBuffer)
		{
#ifdef DEBUG
			puts(VLString("FAILED to load icon file \"") + Iter->first + "\"!");
#endif
			goto Failure;
		}
		
		FileBuffer->pop_back(); //Remove null terminator that Slurp leaves in case it's text.
		IconMap[Iter->second] = FileBuffer;
		
#ifdef DEBUG
		puts(VLString("Loaded ") + "icons" PATH_DIVIDER_STRING + Iter->first + " as \"" + Iter->second + "\".");
#endif
	}
	
#ifdef DEBUG
	puts("GuiIcons::LoadAllIcons(): All icons loaded successfully.");
#endif
	return true;

Failure:
	for (auto Iter = IconMap.begin(); Iter != IconMap.end(); ++Iter) delete Iter->second;
	IconMap.clear();
	return false;
}

const std::vector<uint8_t> &GuiIcons::LookupIcon(const char *IconID)
{
	if (!IconMap.count(IconID)) return *IconMap["unknown"];
	
	return *IconMap[IconID];
}

bool GuiIcons::IconsLoaded(void)
{
	return IconMap.size() == FilenameToIDMap.size();
}
