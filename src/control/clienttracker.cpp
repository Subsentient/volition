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

#include "clienttracker.h"

static std::map<VLString, ClientTracker::ClientRecord> Records;

void ClientTracker::Add(const ClientRecord &In)
{ //Allows updating existing records.
	Records[In.ID] = In;
}

bool ClientTracker::Delete(const VLString &ID)
{
	if (Records.count(ID) == 0) return false;

	Records.erase(ID);

	return true;
}


void ClientTracker::Wipe(void)
{
	Records.clear();
}

ClientTracker::ClientRecord *ClientTracker::Lookup(const VLString &ID)
{
	if (Records.count(ID) == 0) return nullptr;

	return &Records[ID];
}



ClientTracker::RecordIter ClientTracker::GetIterator(void)
{
	return RecordIter(Records.begin());
}

ClientTracker::RecordIter::operator bool(void) const
{
	return this->Internal != Records.end();
}

bool ClientTracker::GroupHasMember(const char *Group)
{
	RecordIter Iter = GetIterator();
	
	for (; Iter; ++Iter)
	{
		if (Iter->Group == Group)
		{
			return true;
		}
	}
	
	return false;
}
