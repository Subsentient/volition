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


#ifndef VL_CTL_GUIMENUS_H
#define VL_CTL_GUIMENUS_H

#include <stddef.h>
#include <gtk/gtk.h>
#include "../libvolition/include/common.h"
#include "../libvolition/include/commandcodes.h"

namespace GuiMenus
{
	bool PopulateMWNodeTreeMenus(GtkWidget *Menu);
	bool PopulateMWServerMenus(GtkWidget *Menu);
	bool IsNodeOrder(const CommandCode CmdCode);
}

#endif //VL_CTL_GUIMENUS_H
