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


#ifndef _VL_SERVER_CMDHANDLING_H_
#define _VL_SERVER_CMDHANDLING_H_

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "clients.h"

#include <vector>
namespace CmdHandling
{
	void HandleReport(Clients::ClientObj *Client, Conation::ConationStream *Stream);
	void HandleRequest(Clients::ClientObj *Client, Conation::ConationStream *Stream);
	void HandleN2N(Clients::ClientObj *Client, Conation::ConationStream *Stream);

	Conation::ConationStream *HandleIndexRequest(Conation::ConationStream *Stream);
	bool NotifyAdmin_NodeChange(const char *NodeID, const bool Online);

}

#endif //_VL_SERVER_CMDHANDLING_H_
