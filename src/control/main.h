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


#ifndef VL_CTL_MAIN_H
#define VL_CTL_MAIN_H

#include "gui_base.h"
#include "../libvolition/include/netscheduler.h"

#define PROGRESS_WINDOW_DATA_THRESHOLD (1024*512)
namespace Main
{
	void InitiateLogin(GuiBase::LoginScreen *Instance);
	void TerminateLink(void);
	
	int GetSocketDescriptor(void);
	
	NetScheduler::ReadQueue &GetReadQueue(void);
	NetScheduler::WriteQueue &GetWriteQueue(void);
}
#endif //VL_CTL_MAIN_H
