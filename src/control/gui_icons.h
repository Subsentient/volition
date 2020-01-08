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


#ifndef VL_CTL_GUIICONS_H
#define VL_CTL_GUIICONS_H
#include "../libvolition/include/common.h"
#include <vector>

namespace GuiIcons
{
	bool LoadAllIcons(const VLString &ConfigDir);
	const std::vector<uint8_t> &LookupIcon(const char *IconID);
	bool IconsLoaded(void);
}
	
#endif //VL_CTL_GUIICONS_H
