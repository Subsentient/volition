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
#include "../libvolition/include/conation.h"
#include "../libvolition/include/utils.h"
#include "ticker.h"

#include <list>
#include <map>
#include <time.h>

#define MAX_TICKER_IDENT_INTEGER 72057594037927936ull
static uint64_t MsgIdentCounter = 1;

static std::list<Ticker::TickerMessage> TickerMessages;
static std::map<uint64_t, std::vector<Ticker::TickerMessage*> > NodeTickerMessages; //Points to elements in TickerMessages.

static uint64_t MsgCounter = 1;

void Ticker::AddNodeMessage(const char *NodeID, const CommandCode CmdCode, const uint64_t Ident, const char *Summary, const char *Expanded)
{
	assert((Conation::GetIdentFlags(Ident) & Conation::IDENT_ISREPORT_BIT) == 0); //Ident shouldn't include the "is report" bit.
	
	assert(NodeID != nullptr);
	assert(Summary != nullptr);

	TickerMessage Msg = { MsgCounter++, NodeID, ORIGIN_NODE, Ident, CmdCode, time(nullptr), Summary, Expanded };

	TickerMessages.push_back(Msg);

	NodeTickerMessages[Ident].push_back(&TickerMessages.back());
	
#ifdef DEBUG
	puts(VLString() + "Ticker::AddNodeMessage(): Got Ident of " + Utils::ToBinaryString(Ident));
#endif
}

void Ticker::AddServerMessage(const char *Summary, const char *Expanded)
{
	assert(Summary != nullptr);

	TickerMessage Msg = { MsgCounter++, "", ORIGIN_SERVER, 0u, CMDCODE_INVALID, time(nullptr), Summary, Expanded };

	TickerMessages.push_back(Msg);
}

void Ticker::AddSystemMessage(const char *Summary, const char *Expanded)
{
	assert(Summary != nullptr);

	TickerMessage Msg = { MsgCounter++, "", ORIGIN_SYSTEM, 0u, CMDCODE_INVALID, time(nullptr), Summary, Expanded };

	TickerMessages.push_back(Msg);
}

bool Ticker::UpdateNodeMessage(const char *NodeID, const uint64_t Ident, const char *Summary, const char *Expanded)
{
	assert(NodeID != nullptr);
	assert((Conation::GetIdentFlags(Ident) & Conation::IDENT_ISREPORT_BIT) == 0); //Ident shouldn't include the "is report" bit.
	assert(Summary != nullptr);

	std::vector<TickerMessage*> &Vector = NodeTickerMessages[Ident];

	for (size_t Inc = 0u; Inc < Vector.size(); ++Inc)
	{
		assert(Vector[Inc]->CmdIdent == Ident);
		
		//Not a match.
		if (Vector[Inc]->OriginType != ORIGIN_NODE || Vector[Inc]->Origin != NodeID) continue;

		//Found it.
		Vector[Inc]->Summary = Summary;
		Vector[Inc]->Expanded = Expanded;
		Vector[Inc]->Time = time(nullptr);

		return true;
	}

	return false;
}

uint64_t Ticker::NewNodeMsgIdent(void)
{
	assert(MsgIdentCounter <= Conation::IDENT_MAX_IDENT_VALUE);

	const uint64_t RetVal = MsgIdentCounter;

	if (MsgIdentCounter == Conation::IDENT_MAX_IDENT_VALUE) MsgIdentCounter = 1;
	else ++MsgIdentCounter;

	return RetVal;
}

Ticker::Iterator Ticker::GetIterator(void)
{
	return TickerMessages.begin();
}

Ticker::Iterator::operator bool(void) const
{
	return this->Internal != TickerMessages.end();
}


const Ticker::TickerMessage *Ticker::LookupMsg(const uint64_t MsgNumber)
{
	Iterator Iter = GetIterator();

	for (; Iter; ++Iter)
	{
		if (Iter->MsgNumber == MsgNumber)
		{
			return &*Iter;
		}
	}

	return nullptr;
}

static void BuildMessageSummarySub(const char *InSummary, const char *Header, VLString &TimeFormatOut, VLString &SummaryOut)

{
	time_t TimeCore = time(nullptr);
	struct tm TimeStruct = *localtime(&TimeCore);

	char Buf[256]{};
	
	strftime(Buf, sizeof Buf, "%Y-%m-%d %I:%M:%S %p", &TimeStruct);

	TimeFormatOut = Buf;

	SummaryOut = VLString() + "<b>" + Header + "</b> " + InSummary;
}

void Ticker::BuildServerMessageSummary(const Iterator &Iter, VLString &TimeFormatOut, VLString &SummaryOut)
{
	BuildMessageSummarySub(Iter->Summary, "<span foreground=\"#CC00CC\">[SERVER]:</span>", TimeFormatOut, SummaryOut);
}

void Ticker::BuildSystemMessageSummary(const Iterator &Iter, VLString &TimeFormatOut, VLString &SummaryOut)
{
	BuildMessageSummarySub(Iter->Summary, "<span foreground=\"#FFAA00\">[SYSTEM]:</span>", TimeFormatOut, SummaryOut);
}

void Ticker::BuildNodeRootMessageSummary(const Iterator &Iter, VLString &SummaryOut)
{
	char Buf[512]{};
	
	snprintf(Buf, sizeof Buf, "<b><span foreground=\"#00aa00\">[ORDER]:</span> %s IDENT %u</b>", +CommandCodeToString(Iter->CmdCode), (unsigned)Iter->CmdIdent);

	SummaryOut = Buf;
}

void Ticker::BuildNodeMessageSummary(const Iterator &Iter, VLString &TimeFormatOut, VLString &SummaryOut)
{
	BuildMessageSummarySub(Iter->Summary,
							VLString("<span foreground=\"#00AAEE\">[NODE ")
							+ Iter->Origin + "]:</span>", TimeFormatOut, SummaryOut);
}


