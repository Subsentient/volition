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


/*
                                                                                                    
                                                                                                    
                                                                                                    
                                               `.`                                                  
                                    --.        ./`.       - .                                       
                                   `::/        -/`.      :`.``                                      
                           ``       .::.       -/.     .:..`.-                                      
                          `-:.       .::.`.-:/+oo/+:.` o:-`-`    `.-.-                              
                           .:/:.    `:sy+mNNNmNNmhsNNmddo:- `/` :/--``.                             
                            :.:+/./ymNNNyhmNNNNNNmyNNNNNddmso-.//-`..`                              
                            `..:yydmNNNNNsNNNNNNNydmNNNmdmydmmds--. `.:                             
                    ..`      .shhdmNNNNddhNNNNNNNdhdNNNmmhmmmmdyh+..`                               
                    `:/:---`-ymmmhyNNNNNymmNNNNNNmsNNNNNNdNNhyhhyys.`-/:                            
                     `.----/dmhhhmmNNNNNNshmmmmmmdymmmmmmhdhmmmddddh+/-` ...-                       
                       `..:hymmmdysmmmNNNNmNNmmNNNmNNNNNNdmmmmmdhhhhh-...`                          
                  ...-....yyddhhdmyNNNNNmNNNNNNNNmdmNNNNNdNNNNNNhddddo      .                       
                  `.-----:dhyshdmmdNNNNNdhmNmmmmmhsmNNNNNddddmNNdmmdys-/:::-.`                      
                   ....``:ydhhdhhssddmmmmdymNNNNNmmNNmNmmymmmmmmhdhhhs:...````                      
                    `..`-/hdhhhydmdhNNNNNNmNNmNNNmNNNNNNNdNNNNNNhhhddh:`````..                      
                    ....-:ssoodddmmhNNNNNNdNNNNNNdmNNNNNNdNNNNNmhddhss-`....                        
                      `-.-hyhhhyhyyyhddddyohhhhdhyohdhhdyyyyydddhdddhh:````.                        
                     .oosssyhshysyhsdmddhdyyysydmdNmhsssshddddddooyhho-:-                           
                     :-:s:/+oysshyss/o////-...:hmdNd+-...:///osssysyyosy++.`                        
                     //ssss+hdyhho//-`.-.---`.odmdmdoo/-``..-..:+shhdhsso:`                         
                     -ohy:-:shhhddhso+/:::+yydmmdhohyys+//::///+syhhy/yss`                          
                      +y+--:oyhsydmmdmhysydNddddNNdmhdsyyyyoyyhhdddho.+y+                           
                      .yhyo+yysoyyyyshhhhyysohdmmmdmdd/yhhhhhhoyysso+-+o--.                         
                       :dyhhyyyyddmmsmmNNNNmyydmmmdmdhommmmmmmhshdhyysh/                            
                        -yhdhhhyddmmhmNNNNmdyyhddhsdhhhmmmmmmmdhhdhyyhs                             
                       .-./+/osohhhdydhshhy+yo:/oo+o/+soyhhhhhdmddhyhs-```                          
                        ..`` :s+hhhshmdmmmosydoo+/yhhdhydmmmddshys//-...:.                          
                             `hsdddyymmNNhyNmdmNNdNNmmmdhmmmmmyddh.                                 
                        `````.ssdddysyhsysmmddNNNdhmNNmdhyyhhhshho`                                 
                         .....-+syyoysymmysdhs+/+ssoooyhysddhhyys-.-....                            
                            `--/oyysmmdh+-/++shhhhhhysoo::shddyy....-``.`                           
                              `.+syodhhyydhhsso+/::/oyhhhhhdddy/.                                   
                              `..:o+sshdmdyhhyyhhsyyyyhdhmdhhs:`                                    
                          ```````.-:oshdmymNNNNNNdNNNNNmhddhs/.```                                  
                            `.--....:::/+ohdddddhshhhddhyo+/-````.``                                
                              ``````...`.:+ooo+//-:ooyyy/-`` `..` ``                                
                             ````````   `    ```` ```...``  `   `                                   
                             .````` ``..```         ````    `````.`                                 
                           `-.``````..```          ``-:-`   `````...`                               
                        `.:/..`````````````````````.-.``   ``````..---.                             
                  ````.::/-:-.````````````...`.```.`-.`````````...--.--.....`                       
     ```.`` ``..-----:-:::--.-:`...`...``.......`....``` ``.......--...--.`.:--.....``````          
..-:://::::/+++/::::/::::::::.-::..-...............-....----..---..---------..-.-----....-.....```` 
::::::::/++/////::/++////:::++:.-/+////-::-...........------//-::---.-----:``----------------::-----

No tears please. It's a waste of good suffering.
*/

#include <sqlite3.h>
#include <string.h>
#include <time.h>

#include <list>
#include "db.h"
#include "clients.h"

#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"

//Globals
static const VLString DBSchema[] = 	{
										//Node information
										"create table nodeinfo (\n"
										"ID text not null unique,\n"
										"PlatformString text not null,\n"
										"NodeRevision text not null,\n"
										"LastConnectedTime int not null,\n"
										"NodeGroup text);\n",
										
										//Current versions of platform binaries
										"create table platformbinaries (\n"
										"PlatformString text not null unique,\n"
										"Revision text not null,\n"
										"Binary blob not null);\n",
										
										//Global server configuration
										"create table globalconfig (\n"
										"Key text not null unique,\n"
										"Value text not null unique);\n",
										
										//Vault for node item storage/retrieval
										"create table vaultdb (\n"
										"Key text not null unique,\n"
										"Binary blob not null,\n"
										"StoredTime int not null,\n"
										"OriginNode text not null);\n",

										//Known node authorization tokens and their permissions
										"create table authtokens (\n"
										"Token text not null unique,\n"
										"Permissions int not null);\n",
										
										//Routine system table
										"create table routines (\n"
										"Name text not null unique,\n"
										"Stream blob not null,\n"
										"Schedule text not null,\n"
										"Flags int not null,\n"
										"Targets text not null);\n",
									};


//Prototypes
static bool SaveNewNode(const DB::NodeDBEntry *Entry);
static bool UpdateExistingNode(const DB::NodeDBEntry *Entry);
static void LoadNodeDBColumn(sqlite3_stmt *const Statement, DB::NodeDBEntry *const Out, const int Index);

static bool SaveNewVaultDBEntry(const DB::VaultDBEntry *Entry);
static bool UpdateExistingVaultDBEntry(const DB::VaultDBEntry *Entry);
static void LoadVaultDBColumn(sqlite3_stmt *const Statement, DB::VaultDBEntry *const Out, const int Index);

static bool SaveNewGlobalConfigDBEntry(const DB::GlobalConfigDBEntry *Entry);
static bool UpdateExistingGlobalConfigDBEntry(const DB::GlobalConfigDBEntry *Entry);
static void LoadGlobalConfigDBColumn(sqlite3_stmt *const Statement, DB::GlobalConfigDBEntry *const Out, const int Index);

static bool SaveNewPlatformBinaryEntry(const DB::PlatformBinaryEntry *Entry);
static bool UpdateExistingPlatformBinaryEntry(const DB::PlatformBinaryEntry *Entry);
static void LoadPlatformBinaryColumn(sqlite3_stmt *const Statement, DB::PlatformBinaryEntry *const Out, const int Index);

static bool SaveNewAuthTokensDBEntry(const DB::AuthTokensDBEntry *Entry);
static bool UpdateExistingAuthTokensDBEntry(const DB::AuthTokensDBEntry *Entry);
static void LoadAuthTokensColumn(sqlite3_stmt *const Statement, DB::AuthTokensDBEntry *const Out, const int Index);

static bool UpdateExistingRoutineDBEntry(const DB::RoutineDBEntry *Entry);
static bool SaveNewRoutineDBEntry(const DB::RoutineDBEntry *Entry);
static void LoadRoutineColumn(sqlite3_stmt *const Statement, DB::RoutineDBEntry *const Out, const int Index);

static bool InitDBSub(sqlite3 *Handle, const char *TableSchema);

//Function definitions
bool DB::InitializeEmptyDB(void)
{
	//Create empty file or erase the old.
	Utils::WriteFile(SERVER_DBFILE, nullptr, 0);

	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
#ifdef DEBUG
		puts("DB::InitializeEmptyDB(): Failed to sqlite3_open()");
#endif
		return false;
	}
	
	bool RetVal = true;
	
	for (size_t Inc = 0u; Inc < sizeof DBSchema / sizeof *DBSchema; ++Inc)
	{
		if (!InitDBSub(Handle, DBSchema[Inc]))
		{
			RetVal = false;
			break;
		}
	}
	
	
	sqlite3_close(Handle);
	
	return RetVal;
}

static bool InitDBSub(sqlite3 *Handle, const char *TableSchema)
{
	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	if (sqlite3_prepare(Handle, TableSchema, strlen(TableSchema), &Statement, &Tail) != SQLITE_OK)
	{
#ifdef DEBUG
		puts("InitDBSub(): Failed to sqlite3_prepare()");
#endif
		return false;
	}

	if (sqlite3_step(Statement) != SQLITE_DONE)
	{
#ifdef DEBUG
		puts("InitDBSub(): Failed to sqlite3_step()");
#endif
		sqlite3_finalize(Statement);
		return false;
	}

	sqlite3_finalize(Statement);


	return true;
}

static bool SaveNewNode(const DB::NodeDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "insert into nodeinfo (ID, PlatformString, NodeRevision, LastConnectedTime, NodeGroup) values (?, ?, ?, ?, ?);";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
#ifdef DEBUG
		puts("DB::SaveNewNode(): Failed to sqlite3_prepare()");
#endif
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, Entry->ID, Entry->ID.size(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 2, Entry->PlatformString, Entry->PlatformString.size(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 3, Entry->NodeRevision, Entry->NodeRevision.size(), SQLITE_STATIC);
	sqlite3_bind_int64(Statement, 4, Entry->LastConnectedTime);
	Entry->Group ? (void)sqlite3_bind_text(Statement, 5, Entry->Group, Entry->Group.size(), SQLITE_STATIC) : (void)sqlite3_bind_null(Statement, 5);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}

static bool SaveNewVaultDBEntry(const DB::VaultDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "insert into vaultdb (Key, Binary, StoredTime, OriginNode) values (?, ?, ?, ?);";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_text(Statement, 1, Entry->Key, Entry->Key.Length(), SQLITE_STATIC);
	sqlite3_bind_blob(Statement, 2, Entry->Binary.data(), Entry->Binary.size(), SQLITE_STATIC);
	sqlite3_bind_int64(Statement, 3, Entry->StoredTime);
	sqlite3_bind_text(Statement, 4, Entry->OriginNode, Entry->OriginNode.Length(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

static bool SaveNewRoutineDBEntry(const DB::RoutineDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "insert into routines (Name, Stream, Schedule, Flags, Targets) values (?, ?, ?, ?, ?);";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_text(Statement, 1, Entry->Name, Entry->Name.Length(), SQLITE_STATIC);
	sqlite3_bind_blob(Statement, 2, Entry->Stream.GetData().data(), Entry->Stream.GetData().size(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 3, Entry->Schedule, Entry->Schedule.Length(), SQLITE_STATIC);
	sqlite3_bind_int(Statement, 4, Entry->Flags);
	
	VLString TargetsString = Utils::JoinTextByCharacter(&Entry->Targets, ',');
	
	sqlite3_bind_text(Statement, 5, TargetsString, TargetsString.Length(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
#ifdef DEBUG
		puts("Failed to execute sqlite3_step in function DB::SaveNewRoutineDBEntry()");
#endif
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

bool DB::DeleteVaultDBEntry(const char *Key)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "delete from vaultdb where Key=?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_text(Statement, 1, Key, strlen(Key), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

bool DB::DeleteRoutineDBEntry(const char *Name)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "delete from routines where Name = ?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_text(Statement, 1, Name, strlen(Name), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

static bool SaveNewAuthTokensDBEntry(const DB::AuthTokensDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "insert into authtokens (Token, Permissions) values (?, ?);";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_text(Statement, 1, Entry->Token, Entry->Token.Length(), SQLITE_STATIC);
	sqlite3_bind_int(Statement, 2, Entry->Permissions);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

static bool SaveNewGlobalConfigDBEntry(const DB::GlobalConfigDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "insert into globalconfig (Key, Value) values (?, ?);";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_text(Statement, 1, Entry->Key, Entry->Key.Length(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 2, Entry->Value, Entry->Value.Length(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

static bool UpdateExistingRoutineDBEntry(const DB::RoutineDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "update routines set Stream = ?, Schedule = ?, Flags = ?, Targets = ? where Name = ?;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_blob(Statement, 1, Entry->Stream.GetData().data(), Entry->Stream.GetData().size(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 2, Entry->Schedule, Entry->Schedule.Length(), SQLITE_STATIC);
	sqlite3_bind_int(Statement,  3, Entry->Flags);

	const VLString TargetsString = Utils::JoinTextByCharacter(&Entry->Targets, ',');
	
	sqlite3_bind_text(Statement, 4, TargetsString, TargetsString.Length(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 5, Entry->Name, Entry->Name.Length(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}
static bool UpdateExistingVaultDBEntry(const DB::VaultDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "update vaultdb set Binary = ?, StoredTime = ?, OriginNode = ? where Key = ?;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_bind_blob(Statement, 1, Entry->Binary.data(), Entry->Binary.size(), SQLITE_STATIC);
	sqlite3_bind_int64(Statement, 2, Entry->StoredTime);
	sqlite3_bind_text(Statement, 3, Entry->OriginNode, Entry->OriginNode.Length(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 4, Entry->Key, Entry->Key.Length(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}
static bool UpdateExistingAuthTokensDBEntry(const DB::AuthTokensDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "update authtokens set Permissions = ? where Token = ?;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_int(Statement, 1, Entry->Permissions);
	sqlite3_bind_text(Statement, 2, Entry->Token, Entry->Token.Length(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

static bool UpdateExistingGlobalConfigDBEntry(const DB::GlobalConfigDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "update globalconfig set Value = ? where Key = ?;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, Entry->Value, Entry->Value.Length(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 2, Entry->Key, Entry->Key.Length(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);
	
	return true;
}

static bool SaveNewPlatformBinaryEntry(const DB::PlatformBinaryEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "insert into platformbinaries (PlatformString, Revision, Binary) values (?, ?, ?);";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
#ifdef DEBUG
		puts("DB::SaveNewPlatformBinaryEntry(): Failed to sqlite3_prepare()");
#endif
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, Entry->PlatformString, Entry->PlatformString.size(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 2, Entry->Revision, Entry->Revision.size(), SQLITE_STATIC);
	sqlite3_bind_blob(Statement, 3, Entry->Binary.data(), Entry->Binary.size(), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}

bool DB::DeletePlatformBinaryEntry(const char *PlatformString)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "delete from platformbinaries where PlatformString = ?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, PlatformString, strlen(PlatformString), SQLITE_STATIC);

	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}

static bool UpdateExistingPlatformBinaryEntry(const DB::PlatformBinaryEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "update platformbinaries set Revision = ?, Binary = ? where PlatformString = ?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, Entry->Revision, Entry->Revision.size(), SQLITE_STATIC);
	sqlite3_bind_blob(Statement, 2, Entry->Binary.data(), Entry->Binary.size(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 3, Entry->PlatformString, Entry->PlatformString.size(), SQLITE_STATIC);


	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}
static bool UpdateExistingNode(const DB::NodeDBEntry *Entry)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "update nodeinfo set PlatformString = ?, NodeRevision = ?, LastConnectedTime = ?, NodeGroup = ? where ID = ?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, Entry->PlatformString, Entry->PlatformString.size(), SQLITE_STATIC);
	sqlite3_bind_text(Statement, 2, Entry->NodeRevision, Entry->NodeRevision.size(), SQLITE_STATIC);
	sqlite3_bind_int64(Statement, 3, Entry->LastConnectedTime);
	Entry->Group ? sqlite3_bind_text(Statement, 4, Entry->Group, Entry->Group.size(), SQLITE_STATIC) : sqlite3_bind_null(Statement, 4);
	sqlite3_bind_text(Statement, 5, Entry->ID, Entry->ID.size(), SQLITE_STATIC);


	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}

bool DB::DeleteAuthToken(const char *Token)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "delete from authtokens where Token=?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, Token, strlen(Token), SQLITE_STATIC);

	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}
bool DB::DeleteGlobalConfigEntry(const char *Key)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "delete from globalconfig where Key=?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, Key, strlen(Key), SQLITE_STATIC);

	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}

bool DB::DeleteNode(const char *ID)
{
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "delete from nodeinfo where ID = ?;";

	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}

	sqlite3_bind_text(Statement, 1, ID, strlen(ID), SQLITE_STATIC);

	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_ERROR || Code == SQLITE_MISUSE)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return true;
}

static void LoadGlobalConfigDBColumn(sqlite3_stmt *const Statement, DB::GlobalConfigDBEntry *const Out, const int Index)
{
	const VLString &Name = sqlite3_column_name(Statement, Index);
	
	if 		(Name == "Key")
	{
		Out->Key = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "Value")
	{
		Out->Value = (const char*)sqlite3_column_text(Statement, Index);
	}
}
static void LoadVaultDBColumn(sqlite3_stmt *const Statement, DB::VaultDBEntry *const Out, const int Index)
{
	const VLString &Name = sqlite3_column_name(Statement, Index);
	
	if 		(Name == "Key")
	{
		Out->Key = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "Binary")
	{
		const size_t BinarySize = sqlite3_column_bytes(Statement, Index);
		const void *Data = sqlite3_column_blob(Statement, Index);
		
		Out->Binary.clear();
		Out->Binary.resize(BinarySize);
		
		memcpy(Out->Binary.data(), Data, BinarySize);
	}
	else if (Name == "StoredTime")
	{
		Out->StoredTime = sqlite3_column_int64(Statement, Index);
	}
	else if (Name == "OriginNode")
	{
		Out->OriginNode = (const char*)sqlite3_column_text(Statement, Index);
	}
}

static void LoadAuthTokensColumn(sqlite3_stmt *const Statement, DB::AuthTokensDBEntry *const Out, const int Index)
{
	const VLString &Name = sqlite3_column_name(Statement, Index);
	
	if 		(Name == "Token")
	{
		Out->Token = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "Permissions")
	{
		Out->Permissions = static_cast<DB::AuthTokenPermissions>(sqlite3_column_int(Statement, Index));
	}
}

static void LoadNodeDBColumn(sqlite3_stmt *const Statement, DB::NodeDBEntry *const Out, const int Index)
{
	const VLString &Name = sqlite3_column_name(Statement, Index);
	
	if 		(Name == "ID")
	{
		Out->ID = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "PlatformString")
	{
		Out->PlatformString = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "NodeRevision")
	{
		Out->NodeRevision = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "NodeGroup")
	{
		Out->Group = sqlite3_column_type(Statement, Index) == SQLITE_NULL ? "" : (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "LastConnectedTime")
	{
		Out->LastConnectedTime = sqlite3_column_int64(Statement, Index);
	}

}


static void LoadPlatformBinaryColumn(sqlite3_stmt *const Statement, DB::PlatformBinaryEntry *const Out, const int Index)
{
	const VLString &Name = sqlite3_column_name(Statement, Index);
	
	if 		(Name == "PlatformString")
	{
		Out->PlatformString = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "Revision")
	{
		Out->Revision = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "Binary")
	{
		const size_t BinarySize = sqlite3_column_bytes(Statement, Index);
		const void *Data = sqlite3_column_blob(Statement, Index);
		
		Out->Binary.clear();
		Out->Binary.resize(BinarySize);
		
		memcpy(Out->Binary.data(), Data, BinarySize);
	}
}


static void LoadRoutineColumn(sqlite3_stmt *const Statement, DB::RoutineDBEntry *const Out, const int Index)
{
	const VLString &Name = sqlite3_column_name(Statement, Index);
	
	if 		(Name == "Name")
	{
		Out->Name = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "Schedule")
	{
		Out->Schedule = (const char*)sqlite3_column_text(Statement, Index);
	}
	else if (Name == "Targets")
	{
		std::vector<VLString> *Breakdown = Utils::SplitTextByCharacter((const char*)sqlite3_column_text(Statement, Index), ',');
		
		if (!Breakdown || Breakdown->empty()) return;
		
		Out->Targets = *Breakdown;
		delete Breakdown;
	}
	else if (Name == "Flags")
	{
		Out->Flags = sqlite3_column_int(Statement, Index);
	}
	else if (Name == "Stream")
	{
		const void *Data = sqlite3_column_blob(Statement, Index);

		Out->Stream = Conation::ConationStream { static_cast<const uint8_t*>(Data) };
	}
}

DB::NodeDBEntry *DB::LookupNodeInfo(const char *ID)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "select * from nodeinfo where ID = ? limit 1;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	sqlite3_bind_text(Statement, 1, ID, strlen(ID), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_DONE)
	{ //Not found.
		sqlite3_finalize(Statement);
		sqlite3_close(Handle);
		return nullptr;
	}
	
	if (Code != SQLITE_ROW)
	{ //Possible other error.
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);

	DB::NodeDBEntry *RetVal = new DB::NodeDBEntry;

	for (int Inc = 0; Inc < Columns; ++Inc)
	{
		LoadNodeDBColumn(Statement, RetVal, Inc); //We can use this to check for existence this way.
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return RetVal;
}

DB::PlatformBinaryEntry *DB::LookupPlatformBinaryEntry(const char *PlatformString, const char *SQLFields)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const VLString SQL = VLString("select ") + SQLFields + " from platformbinaries where PlatformString = ? limit 1;";

	
	if (sqlite3_prepare(Handle, SQL, SQL.Length(), &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	sqlite3_bind_text(Statement, 1, PlatformString, strlen(PlatformString), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_DONE)
	{ //Not found.
		sqlite3_finalize(Statement);
		sqlite3_close(Handle);
		return nullptr;
	}
	
	if (Code != SQLITE_ROW)
	{ //Possible other error.
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);

	DB::PlatformBinaryEntry *RetVal = new DB::PlatformBinaryEntry;

	for (int Inc = 0; Inc < Columns; ++Inc)
	{
		LoadPlatformBinaryColumn(Statement, RetVal, Inc); //We can use this to check for existence this way.
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return RetVal;
}

bool DB::GetOfflineNodes(std::list<NodeDBEntry> &Ref)
{ //Gives us a list of all known nodes that are NOT currently online.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return false;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "select * from nodeinfo;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return false;
	}
	
	const int Columns = sqlite3_column_count(Statement);
	
	int Code{};
	
	while ((Code = sqlite3_step(Statement)) == SQLITE_ROW)
	{ //Get all of them.
		NodeDBEntry Values{};
		
		for (int Inc = 0; Inc < Columns; ++Inc)
		{
			LoadNodeDBColumn(Statement, &Values, Inc); //We can use this to check for existence this way.
		}
		
		if (Clients::LookupClient(Values.ID) != nullptr) continue;
		
		Ref.push_back(Values);
	}
	
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return Code == SQLITE_DONE;
}

	

bool DB::UpdateNodeDB(const char *ID, const char *PlatformString, const char *NodeRevision, const char *Group, const time_t ConnectedTime)
{ //Group is always optional, the platform string is required for new nodes.
	NodeDBEntry *Lookup = LookupNodeInfo(ID);

	if ((!PlatformString || !NodeRevision) && !Lookup)
	{
		return false;
	}
	
	NodeDBEntry Node;
	Node.ID = ID;
	Node.PlatformString = PlatformString ? PlatformString : +Lookup->PlatformString;
	Node.NodeRevision = NodeRevision ? NodeRevision : +Lookup->NodeRevision;
	Node.LastConnectedTime = ConnectedTime ? ConnectedTime : Lookup->LastConnectedTime;
	Node.Group = Group ? Group : "";
	
	if (Lookup)
	{
		if (!Group) Node.Group = Lookup->Group;
		delete Lookup;
		const bool RetVal = UpdateExistingNode(&Node);
#ifdef DEBUG
		printf("DB::UpdateNodeDB(): Existing node update %s\n", RetVal ? "succeeded" : "failed");
#endif
		return RetVal;
	}
	else
	{
		const bool RetVal = SaveNewNode(&Node);
#ifdef DEBUG
		printf("DB::UpdateNodeDB(): New node storage %s\n", RetVal ? "succeeded" : "failed");
#endif
		return RetVal;
	}
}

bool DB::UpdatePlatformBinaryDB(const char *PlatformString, const char *Revision, const void *BinaryData, const size_t BinarySize)
{
	assert(PlatformString && Revision && BinaryData && BinarySize); //If this is wrong, something's wrong with code higher up.
	
	PlatformBinaryEntry *Lookup = LookupPlatformBinaryEntry(PlatformString, "PlatformString, Revision");
	
	PlatformBinaryEntry Entry;
	
	Entry.PlatformString = PlatformString;
	Entry.Revision = Revision;
	Entry.Binary.resize(BinarySize);
	
	memcpy(Entry.Binary.data(), BinaryData, BinarySize);
	
	const bool RetVal = Lookup ? UpdateExistingPlatformBinaryEntry(&Entry) : SaveNewPlatformBinaryEntry(&Entry);
	
#ifdef DEBUG
	puts(VLString("DB::UpdatePlatformBinaryDB(): ") + (Lookup ? "Update of binary for platform " : "Saving of new binary for platform ") + PlatformString + (RetVal ? " succeeded." : " failed!"));
#endif
	delete Lookup; //Might be null but irrelevant
	
	return RetVal;
}

bool DB::NodeBinaryNeedsUpdate(Clients::ClientObj *Client, std::vector<uint8_t> *NewBinaryOut, VLString *NewRevisionOut)
{
	assert(Client != nullptr);
	
	PlatformBinaryEntry *Lookup = LookupPlatformBinaryEntry(Client->GetPlatformString());
	
	if (!Lookup) return false; //No record of an update so we must not need updating.
	
	//If we do have a record, does it match what the node's running?
	const bool RetVal = Lookup->Revision != Client->GetNodeRevision();
	
	//If it's a different revision than the node's running, make them update.
	if (RetVal)
	{
		if (NewBinaryOut) *NewBinaryOut = Lookup->Binary;
		if (NewRevisionOut) *NewRevisionOut = Lookup->Revision;
	}
	
	delete Lookup;
	
	return RetVal;
}

bool DB::UpdateGlobalConfigDB(const GlobalConfigDBEntry *Entry)
{
	DB::GlobalConfigDBEntry *Lookup = LookupGlobalConfigDBEntry(Entry->Key);
	
	bool (*StorageFunc)(const GlobalConfigDBEntry*) = Lookup ? UpdateExistingGlobalConfigDBEntry : SaveNewGlobalConfigDBEntry;
	
	delete Lookup;
	
	return StorageFunc(Entry);	
}

bool DB::UpdateAuthTokensDB(const AuthTokensDBEntry *Entry)
{
	DB::AuthTokensDBEntry *Lookup = LookupAuthToken(Entry->Token);
	
	bool (*StorageFunc)(const AuthTokensDBEntry*) = Lookup ? UpdateExistingAuthTokensDBEntry : SaveNewAuthTokensDBEntry;
	
	delete Lookup;
	
	return StorageFunc(Entry);	
}

bool DB::UpdateVaultDB(const VaultDBEntry *Entry)
{
	DB::VaultDBEntry *Lookup = LookupVaultDBEntry(Entry->Key, "Key");
	
	bool (*StorageFunc)(const VaultDBEntry*) = Lookup ? UpdateExistingVaultDBEntry : SaveNewVaultDBEntry;
	
	delete Lookup;
	
	return StorageFunc(Entry);	
}

bool DB::UpdateRoutineDB(const RoutineDBEntry *Entry)
{
	VLScopedPtr<RoutineDBEntry*> Lookup = LookupRoutineDBEntry(Entry->Name, "Name");
	
	bool (*StorageFunc)(const RoutineDBEntry*) = Lookup ? UpdateExistingRoutineDBEntry : SaveNewRoutineDBEntry;
	
	return StorageFunc(Entry);	
}

DB::RoutineDBEntry *DB::LookupRoutineDBEntry(const char *Name, const char *SQLFields)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	VLString SQL = VLString("select ") + SQLFields + " from routines where Name = ? limit 1;";

	
	if (sqlite3_prepare(Handle, SQL, SQL.Length(), &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	sqlite3_bind_text(Statement, 1, Name, strlen(Name), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_DONE)
	{ //Not found.
		sqlite3_finalize(Statement);
		sqlite3_close(Handle);
		return nullptr;
	}
	
	if (Code != SQLITE_ROW)
	{ //Possible other error.
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);

	DB::RoutineDBEntry *RetVal = new DB::RoutineDBEntry;

	for (int Inc = 0; Inc < Columns; ++Inc)
	{
		LoadRoutineColumn(Statement, RetVal, Inc); //We can use this to check for existence this way.
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return RetVal;
}

DB::VaultDBEntry *DB::LookupVaultDBEntry(const char *Key, const char *SQLFields)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	VLString SQL = VLString("select ") + SQLFields + " from vaultdb where Key = ? limit 1;";

	
	if (sqlite3_prepare(Handle, SQL, SQL.Length(), &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	sqlite3_bind_text(Statement, 1, Key, strlen(Key), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_DONE)
	{ //Not found.
		sqlite3_finalize(Statement);
		sqlite3_close(Handle);
		return nullptr;
	}
	
	if (Code != SQLITE_ROW)
	{ //Possible other error.
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);

	DB::VaultDBEntry *RetVal = new DB::VaultDBEntry;

	for (int Inc = 0; Inc < Columns; ++Inc)
	{
		LoadVaultDBColumn(Statement, RetVal, Inc); //We can use this to check for existence this way.
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return RetVal;
}


DB::GlobalConfigDBEntry *DB::LookupGlobalConfigDBEntry(const char *Key)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "select * from globalconfig where Key = ? limit 1;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	sqlite3_bind_text(Statement, 1, Key, strlen(Key), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_DONE)
	{ //Not found.
		sqlite3_finalize(Statement);
		sqlite3_close(Handle);
		return nullptr;
	}
	
	if (Code != SQLITE_ROW)
	{ //Possible other error.
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);

	DB::GlobalConfigDBEntry *RetVal = new DB::GlobalConfigDBEntry;

	for (int Inc = 0; Inc < Columns; ++Inc)
	{
		LoadGlobalConfigDBColumn(Statement, RetVal, Inc); //We can use this to check for existence this way.
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return RetVal;
}

DB::AuthTokensDBEntry *DB::LookupAuthToken(const char *Token)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "select * from authtokens where Token = ? limit 1;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	sqlite3_bind_text(Statement, 1, Token, strlen(Token), SQLITE_STATIC);
	
	int Code = sqlite3_step(Statement);
	
	if (Code == SQLITE_DONE)
	{ //Not found.
		sqlite3_finalize(Statement);
		sqlite3_close(Handle);
		return nullptr;
	}
	
	if (Code != SQLITE_ROW)
	{ //Possible other error.
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);

	DB::AuthTokensDBEntry *RetVal = new DB::AuthTokensDBEntry();

	for (int Inc = 0; Inc < Columns; ++Inc)
	{
		LoadAuthTokensColumn(Statement, RetVal, Inc); //We can use this to check for existence this way.
	}
	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	return RetVal;
}

std::vector<DB::AuthTokensDBEntry> *DB::GetAllAuthTokens(void)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "select * from authtokens;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	int Code = 0;	
	const int Columns = sqlite3_column_count(Statement);
	
	std::vector<AuthTokensDBEntry> *RetVal = new std::vector<AuthTokensDBEntry>;

	while ((Code = sqlite3_step(Statement)) == SQLITE_ROW)
	{
		RetVal->push_back({});
		
		for (int Inc = 0; Inc < Columns; ++Inc)
		{
			LoadAuthTokensColumn(Statement, &RetVal->back(), Inc); //We can use this to check for existence this way.
		}

	}

	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	if (Code != SQLITE_DONE)
	{
		delete RetVal;
		return nullptr;
	}
	
	return RetVal;
}

std::vector<DB::RoutineDBEntry> *DB::GetRoutineDBInfo(void)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "select Name, Schedule, Flags, Targets from routines;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);
	
	std::vector<RoutineDBEntry> *RetVal = new std::vector<RoutineDBEntry>;
	RetVal->reserve(8);
	
	int Code = 0;
	while ((Code = sqlite3_step(Statement)) == SQLITE_ROW)
	{
		if (RetVal->size() >= RetVal->capacity())
		{
			RetVal->reserve(RetVal->capacity() * 2);
		}
		
		RetVal->push_back({});
		
		for (int Inc = 0; Inc < Columns; ++Inc)
		{
			LoadRoutineColumn(Statement, &RetVal->back(), Inc); //We can use this to check for existence this way.
		}

	}

	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	if (Code != SQLITE_DONE)
	{
		delete RetVal;
		return nullptr;
	}
	
	RetVal->shrink_to_fit();
	return RetVal;
}

std::vector<DB::VaultDBEntry> *DB::GetVaultItemsInfo(void)
{ //Returns a pointer allocated on the heap.
	sqlite3 *Handle = nullptr;

	if (sqlite3_open(SERVER_DBFILE, &Handle) != 0)
	{
		return nullptr;
	}

	sqlite3_stmt *Statement = nullptr;
	const char *Tail = nullptr;

	const char SQL[] = "select Key, StoredTime, OriginNode from vaultdb;";

	
	if (sqlite3_prepare(Handle, SQL, sizeof SQL - 1, &Statement, &Tail) != SQLITE_OK)
	{
		sqlite3_close(Handle);
		return nullptr;
	}

	const int Columns = sqlite3_column_count(Statement);
	
	std::vector<VaultDBEntry> *RetVal = new std::vector<VaultDBEntry>;
	RetVal->reserve(128);
	
	int Code = 0;
	while ((Code = sqlite3_step(Statement)) == SQLITE_ROW)
	{
		RetVal->push_back({});
		
		for (int Inc = 0; Inc < Columns; ++Inc)
		{
			LoadVaultDBColumn(Statement, &RetVal->back(), Inc); //We can use this to check for existence this way.
		}

	}

	
	sqlite3_finalize(Statement);
	sqlite3_close(Handle);

	if (Code != SQLITE_DONE)
	{
		delete RetVal;
		return nullptr;
	}
	
	RetVal->shrink_to_fit();
	return RetVal;
}

