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

#ifndef VL_CTL_CLIENTTRACKER_H
#define VL_CTL_CLIENTTRACKER_H

#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"

#include <map>

#include <time.h>
namespace ClientTracker
{
	struct ClientRecord
	{
		VLString ID;
		VLString PlatformString;
		VLString NodeRevision;
		VLString Group;
		VLString IPAddr;
		int64_t LastPingLatency;
		time_t ConnectedTime;
		bool Online;
	};

	class RecordIter
	{
	private:
		std::map<VLString, ClientRecord>::iterator Internal;
	public:
		RecordIter(decltype(Internal) Param) : Internal(Param) {}
		ClientRecord &operator*(void) { return Internal->second; }
		const ClientRecord &operator*(void) const { return Internal->second; }
		ClientRecord *operator->(void) { return &Internal->second; }
		const ClientRecord *operator->(void) const { return &Internal->second; }
		RecordIter operator++(int) { auto Backup = this->Internal; ++Internal; return Backup; }
		RecordIter &operator++(void) { ++Internal; return *this; }
		RecordIter operator--(int) { auto Backup = this->Internal; --Internal; return Backup; }
		RecordIter &operator--(void) { --Internal; return *this; }
		operator bool(void) const;
	};

	void Add(const ClientRecord &In);
	bool Delete(const VLString &ID);
	void Wipe(void);
	ClientRecord *Lookup(const VLString &ID);
	bool GroupHasMember(const char *Group);
	RecordIter GetIterator(void);


}
#endif //VL_CTL_CLIENTTRACKER_H
