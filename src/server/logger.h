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


#ifndef _VL_SERVER_LOGGER_H_
#define _VL_SERVER_LOGGER_H_

/* A noble sacrifice is foolish when a noble deed would have sufficed. */

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"

namespace Logger
{
	enum ItemType
	{
		LOGITEM_INVALID,
		LOGITEM_CONN,
		LOGITEM_INFO,
		LOGITEM_PERMS,
		LOGITEM_SYSERROR,
		LOGITEM_SYSWARN,
		LOGITEM_SECUREERROR,
		LOGITEM_SECUREWARN,
		LOGITEM_ROUTINEERROR,
		LOGITEM_ROUTINEWARNING,
		LOGITEM_MAX,
	};
	
	bool WriteLogLine(const ItemType Type, const char *Text);
	VLString TailLog(const size_t NumLinesFromLast);
	bool WipeLog(void);
	size_t GetLogSize(void);
	VLString ReportArgsToText(Conation::ConationStream *Stream);
}

#endif //_VL_SERVER_LOGGER_H_
