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


#ifndef __CORE_H__
#define __CORE_H__

//Server core timer frequency in milliseconds, that is, how often it checks for changes.
#ifndef SERVER_CORE_FREQUENCY_MS
#define SERVER_CORE_FREQUENCY_MS 5
#endif //SERVER_TIMER_FREQUENCY_MS

#include "../libvolition/include/common.h"
#include <vector>

namespace Core
{
	//Types

	//Functions
	bool ValidServerAdminLogin(const char *const Username, const char *const Password);

	void SignalSkipHeadRelease(void);
	bool CheckInsideHandleInterface(void);
	bool CheckIsActiveClient(void *Client);
	void SignalResetLoopAfter(void);
	//Globals
}

#endif //__CORE_H__
