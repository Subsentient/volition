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


#ifndef VL_CTL_TICKER_H
#define VL_CTL_TICKER_H
#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"
#include "../libvolition/include/conation.h"

#include <list>
#include <time.h>

namespace Ticker
{
	enum MsgOriginType { ORIGIN_INVALID, ORIGIN_NODE, ORIGIN_SERVER, ORIGIN_SYSTEM };
	struct TickerMessage
	{
		uint64_t MsgNumber; //Unique identifier for this ticker message.
		VLString Origin; //Empty if server/system message.
		Ticker::MsgOriginType OriginType;
		uint64_t CmdIdent; //The ident associated with this message, probably zero for server/system messages.
		CommandCode CmdCode; //Empty if server/system message.
		//Message data
		time_t Time;
		VLString Summary;
		VLString Expanded;
	};

	class Iterator
	{
	private:
		std::list<TickerMessage>::iterator Internal;
	public:
		Iterator(decltype(Internal) Param) : Internal(Param) {}
		operator bool(void) const;
		Iterator &operator++(void) { ++Internal; return *this; }
		Iterator operator++(int) { auto Backup = Internal; ++Internal; return Backup; }
		Iterator &operator--(void) { --Internal; return *this; }
		Iterator operator--(int) { auto Backup = Internal; --Internal; return Backup; }
		
		TickerMessage &operator*(void) { return *Internal; }
		const TickerMessage &operator*(void) const { return *Internal; }
		TickerMessage *operator->(void) { return &*Internal; }
		const TickerMessage *operator->(void) const { return &*Internal; }
	};

	Iterator GetIterator(void);
	uint64_t NewNodeMsgIdent(void);

	void AddNodeMessage(const char *NodeID, const CommandCode CmdCode, const uint64_t Ident, const char *Summary, const char *Expanded = nullptr);
	bool UpdateNodeMessage(const char *NodeID, const uint64_t Ident, const char *Summary, const char *Expanded = nullptr);
	void AddServerMessage(const char *Summary, const char *Expanded = nullptr);
	void AddSystemMessage(const char *Summary, const char *Expanded = nullptr);

	void BuildServerMessageSummary(const Iterator &Msg, VLString &TimeFormatOut, VLString &SummaryOut);
	void BuildSystemMessageSummary(const Iterator &Msg, VLString &TimeFormatOut, VLString &SummaryOut);
	
	void BuildNodeRootMessageSummary(const Iterator &Msg, VLString &SummaryOut);
	
	void BuildNodeMessageSummary(const Iterator &Msg, VLString &TimeFormatOut, VLString &SummaryOut);

	const TickerMessage *LookupMsg(const uint64_t MsgNumber);
}

#endif //VL_CTL_TICKER_H
