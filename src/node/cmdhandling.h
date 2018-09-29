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

#ifndef _VL_NODE_CMDHANDLING_H_
#define _VL_NODE_CMDHANDLING_H_

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include <stddef.h>

namespace CmdHandling
{
	void HandleRequest(Conation::ConationStream *Stream);
	void HandleReport(Conation::ConationStream *Stream);
}

#endif //_VL_NODE_CMDHANDLING_H_
