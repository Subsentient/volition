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
#include "../libvolition/include/brander.h"
#include "../libvolition/include/conation.h"
#include "clients.h"
#include "nodeupdates.h"
#include "db.h"

#include <vector>
#include <map>

void NodeUpdates::HandleUpdatesForNode(Clients::ClientObj *Client)
{
	assert(Client != Clients::LookupCurAdmin());
	
	//Check if they need updating.
	std::vector<uint8_t> NewBinary;
	VLString NewRevision;
	
	const bool NeedUpdate = DB::NodeBinaryNeedsUpdate(Client, &NewBinary, &NewRevision);
	
	assert((!NeedUpdate && !NewBinary.size()) || (NeedUpdate && NewBinary.size()));
	
	if (!NeedUpdate)
	{
		return; //Nothing to do.
	}
	
	//Guess we do need an update.
	
	//Check to see if this binary is validly branded for the platform.
	Brander::AttrValue *BinPlatformString = Brander::ReadBrandedBinaryViaBuffer((const char*)NewBinary.data(), NewBinary.size(), Brander::AttributeTypes::PLATFORMSTRING);
	Brander::AttrValue *BinRevision = Brander::ReadBrandedBinaryViaBuffer((const char*)NewBinary.data(), NewBinary.size(), Brander::AttributeTypes::REVISION);
	Brander::AttrValue *BinServerAddr = Brander::ReadBrandedBinaryViaBuffer((const char*)NewBinary.data(), NewBinary.size(), Brander::AttributeTypes::SERVERADDR);
	Brander::AttrValue *BinAuthToken = Brander::ReadBrandedBinaryViaBuffer((const char*)NewBinary.data(), NewBinary.size(), Brander::AttributeTypes::AUTHTOKEN);

	
	//These checks are all mostly unnecessary
	if (!BinPlatformString ||
		!BinRevision ||
		!BinServerAddr || //We just wanna make sure the server is set.
		!BinAuthToken ||
		BinRevision->Get_String() != NewRevision ||
		BinPlatformString->Get_String() != Client->GetPlatformString())
	{
		throw Errors::CorruptedBinary(BinPlatformString ? BinPlatformString->Get_String() : "",
									  BinRevision ? BinRevision->Get_String() : "",
									  BinServerAddr ? BinServerAddr->Get_String() : "");
	}
	
	//Brand the binary.
	std::map<Brander::AttributeTypes, Brander::AttrValue> Values;
	Values.emplace(Brander::AttributeTypes::IDENTITY, Client->GetID());
	Values.emplace(Brander::AttributeTypes::AUTHTOKEN, Client->GetAuthToken());

	if (!Brander::BrandBinaryViaBuffer((char*)NewBinary.data(), NewBinary.size(), Values))
	{
		VLDEBUG("Failed to brand new binary in RAM for node " + Client->GetID());
		return;
	}
	
	//alright, now it's time to send the order.
	Conation::ConationStream Order(CMDCODE_B2C_USEUPDATE, false, 0u);
	
	Order.Push_File(VLString(), NewBinary.data(), NewBinary.size());
	
	//So we don't have potentially 3 copies in memory at once.
	NewBinary.clear();
	
	VLDEBUG("Sending new binary to node " + Client->GetID());
	
	Client->SendStream(Order);
	
	VLDEBUG("Transmission succeeded");
	
}

bool NodeUpdates::GetNodeBinaryBrandInfo(const void *Buf, const size_t BufSize, VLString *PlatformStringOut, VLString *RevisionOut, VLString *ServerOut)
{
	if (!Buf || !BufSize) return false;
	
	std::map<Brander::AttributeTypes, VLString *> OutputMap =
															{
																{ Brander::AttributeTypes::PLATFORMSTRING, PlatformStringOut },
																{ Brander::AttributeTypes::REVISION, RevisionOut },
																{ Brander::AttributeTypes::SERVERADDR, ServerOut },
															};
															
	for (auto Iter = OutputMap.begin(); Iter != OutputMap.end(); ++Iter)
	{
		if (!Iter->second) continue; //We don't gotta do anything.
		
		Brander::AttrValue *Ptr = nullptr;
		if (!(Ptr = Brander::ReadBrandedBinaryViaBuffer(Buf, BufSize, Iter->first)) )
		{
			return false;
		}
		
		*Iter->second = Ptr->Get_String();
	}
	
	return true;		
}

