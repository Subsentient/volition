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


#ifndef _VL_SERVER_DB_H_
#define _VL_SERVER_DB_H_

/* "Neither the sun nor death can be looked at steadily."
 * -François de La Rochefoucauld
 */
 
#define SERVER_DBFILE "./server.db"
#include <time.h>
#include "../libvolition/include/common.h"
#include <sqlite3.h>
#include <list>
#include <vector>

#include "clients.h"
#include "routines.h"

namespace DB
{

	enum AuthTokenPermissions : uint32_t
	{
		ATP_INVALID					= 0, //Invalid value
		ATP_N2NCOMM_ANY				= 1 << 1, //Node to node communications to ANY node
		ATP_N2NCOMM_GROUP			= 1 << 2, //Node to node communications to nodes in their group only
		ATP_VAULT_READ_OUR			= 1 << 3, //Read our own items from the vault
		ATP_VAULT_READ_ANY			= 1 << 4, //Read ANY items from the vault
		ATP_VAULT_CHG_OUR			= 1 << 5, //Add, update or delete items in the vault that belong to this node.
		ATP_VAULT_CHG_ANY			= 1 << 6, //Update or delete ANY items in the vault.
		ATP_VAULT_ADD				= 1 << 7, //Add a new item to the vault
	};
	
	struct NodeDBEntry
	{
		VLString ID;
		VLString PlatformString;
		VLString NodeRevision;
		time_t LastConnectedTime;
		VLString Group;
	};
	
	struct PlatformBinaryEntry
	{
		VLString PlatformString;
		VLString Revision;
		std::vector<uint8_t> Binary;
	};

	struct VaultDBEntry
	{
		VLString Key;
		std::vector<uint8_t> Binary;
		VLString OriginNode;
		time_t StoredTime;
	};
	
	struct GlobalConfigDBEntry
	{
		VLString Key;
		VLString Value;
	};
	
	struct AuthTokensDBEntry
	{
		VLString Token;
		AuthTokenPermissions Permissions;
	};  
	
	
	struct RoutineDBEntry : public Routines::RoutineInfo
	{
		Conation::ConationStream Stream;
	};
	
	//Stuff for all DBs
	bool InitializeEmptyDB(void);
	
	//Node DB
	bool UpdateNodeDB(const char *ID, const char *PlatformString, const char *NodeRevision, const char *Group, const time_t ConnectedTime = 0);
	NodeDBEntry *LookupNodeInfo(const char *ID);
	bool DeleteNode(const char *ID);
	bool GetOfflineNodes(std::list<DB::NodeDBEntry> &Ref);
	
	//Platform binary DB
	PlatformBinaryEntry *LookupPlatformBinaryEntry(const char *PlatformString, const char *SQLFields = "*");
	bool UpdatePlatformBinaryDB(const char *PlatformString, const char *Revision, const void *BinaryData, const size_t BinarySize);
	bool DeletePlatformBinaryEntry(const char *PlatformString);
	bool NodeBinaryNeedsUpdate(Clients::ClientObj *Client, std::vector<uint8_t> *NewBinaryOut = nullptr, VLString *NewRevisionOut = nullptr);
	
	//Vault database
	VaultDBEntry *LookupVaultDBEntry(const char *Key, const char *SQLFields = "*");
	bool UpdateVaultDB(const VaultDBEntry *Entry);
	bool DeleteVaultDBEntry(const char *Key);
	std::vector<VaultDBEntry> *GetVaultItemsInfo(void);

	//Global config table
	GlobalConfigDBEntry *LookupGlobalConfigDBEntry(const char *Key);
	bool UpdateGlobalConfigDB(const GlobalConfigDBEntry *Entry);
	bool DeleteGlobalConfigEntry(const char *Key);
	
	//Authorization tokens table
	AuthTokensDBEntry *LookupAuthToken(const char *Token);
	bool UpdateAuthTokensDB(const AuthTokensDBEntry *Entry);
	bool DeleteAuthToken(const char *Token);
	std::vector<AuthTokensDBEntry> *GetAllAuthTokens(void);
	
	//Routine database
	bool DeleteRoutineDBEntry(const char *Name);
	RoutineDBEntry *LookupRoutineDBEntry(const char *Name, const char *SQLFields = "*");
	bool UpdateRoutineDB(const RoutineDBEntry *Entry);
	std::vector<RoutineDBEntry> *GetRoutineDBInfo(void);
}
#endif //_VL_SERVER_DB_H_
