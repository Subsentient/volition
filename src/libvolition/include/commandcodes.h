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


#ifndef _VL_COMMANDCODES_H_
#define _VL_COMMANDCODES_H_

#include <stdint.h>
#include "common.h"

typedef uint8_t CommandCodeUint;

enum CommandCode : CommandCodeUint //Keep this 8-bit unsigned, or the gerbocalypse will begin!
{ ///Not a bit-shifting thing, we want 255 possible values.

	/**Legend:
	 * _ANY_ : any direction
	 * _C2S_ : Node to server
	 * _S2C_ : Server to node
	 * _A2S_ : Admin to server
	 * _A2C_ : Admin to node, communicating through the server.
	 * _S2A_ : Server to admin.
	 * _C2A_ : Node to admin, shouldn't happen often.
	 * _C2C_ : Two or more nodes communicating with each other through the server.
	 * _B2S_ : Either client or admin to server.
	 * _B2C_ : Either admin or server to client.
	 **/

	/*IMPORTANT!!!! Do not add commands in the middle of this list! Only at the end!
	* It breaks ABI compatibility with existing nodes and admins if you do. **/
	
	CMDCODE_INVALID				= 0, ///Fucky, not usable.
	CMDCODE_ANY_NOOP			= 1, //No operation. Instantly discarded.
	CMDCODE_ANY_ECHO			= 2, //Just send everything we received back where it came from, switched to a report. Used for diagnostics.
	CMDCODE_ANY_PING			= 3, //Does basically the same thing as echo, except usually isn't sent any arguments.
	CMDCODE_B2S_AUTH			= 4, //Client logging in to server.
	CMDCODE_ANY_DISCONNECT		= 5, //Client disconnecting from server.
	CMDCODE_A2C_SLEEP			= 6, //Orders client to terminate but still be in the boot sequence later.
	CMDCODE_S2C_KILL			= 7, //Orders client to slit its wrists and delete itself.
	CMDCODE_A2S_INFO			= 8, //Asks the server for some information about a specific client.
	CMDCODE_A2S_INDEXDL			= 9, //Admin requesting to download list of information on all clients.
	CMDCODE_S2A_ADMINDEAUTH		= 10, //Admin must disconnect, probably because another logged on.
	CMDCODE_A2C_CHDIR			= 11, //Change working directory
	CMDCODE_A2C_GETCWD			= 12, //Get current working directory
	CMDCODE_A2C_GETPROCESSES	= 13, //Get list of all system processes
	CMDCODE_A2C_KILLPROCESS		= 14, //Kill one or more processes.
	CMDCODE_A2C_EXECCMD			= 15, //Execute a command on the node's host's shell.
	CMDCODE_A2C_FILES_DEL		= 16, //Delete file(s)
	CMDCODE_A2C_FILES_PLACE		= 17, //Upload file(s) to node's host.
	CMDCODE_A2C_FILES_FETCH		= 18, //Download file(s) from host's drive(s).
	CMDCODE_A2C_FILES_COPY		= 19, //Local copy operation, like 'cp -rf'.
	CMDCODE_A2C_FILES_MOVE		= 20, //Move files from one location to another.
	CMDCODE_S2A_NOTIFY_NODECHG	= 21, //Server telling the admin of a joining or quitting node or updating other info.
	CMDCODE_B2C_USEUPDATE		= 22, //The command the server sends nodes that contains their updated binary to re-execute.
	CMDCODE_A2S_PROVIDEUPDATE	= 23, //The admin sending a new update to the server to disseminate to nodees of a certain platform.
	CMDCODE_A2S_CHGNODEGROUP	= 24, //The admin wants to change the associated group for a node.
	CMDCODE_A2C_WEB_FETCH		= 25, //We need to download a file/resource.
	CMDCODE_A2C_MOD_LOADSCRIPT	= 26, //Tells the client to load a script we send them and execute the main body to define functions and variables.
	CMDCODE_A2C_MOD_UNLOADSCRIPT= 27, //Tells the client to unload a script they have in memory.
	CMDCODE_A2C_MOD_EXECFUNC	= 28, //Tells the client to execute a function in a module/script.
	CMDCODE_A2C_MOD_GENRESP		= 29, //Generic response from module/script running on a node.
	CMDCODE_B2C_GETJOBSLIST		= 30, //Reports the jobs the node has running and their command codes and idents.
	CMDCODE_B2C_KILLJOBID		= 31, //Kill a running job on the node by its ID.
	CMDCODE_B2C_KILLJOBBYCMDCODE= 32, //Kill a running job on the node by the command code of the job.
	CMDCODE_A2S_SETSCONFIG		= 33, //Set a configuration variable on the server.
	CMDCODE_A2S_GETSCONFIG		= 34, //Sends the value of a specified server config variable to the admin.
	CMDCODE_A2S_DROPCONFIG		= 35, //Delete key from configuration database
	CMDCODE_B2S_VAULT_ADD		= 36, //Add a new item to the server's vault
	CMDCODE_B2S_VAULT_FETCH		= 37, //Get an item from the server's file vault.
	CMDCODE_B2S_VAULT_UPDATE	= 38, //Upload a new item to the vault or update an existing item.
	CMDCODE_B2S_VAULT_DROP		= 39, //Delete item from vault
	CMDCODE_A2S_VAULT_LIST		= 40, //Admins can ask to see what's in the vault
	CMDCODE_A2S_FORGETNODE		= 41, //Delete a node from the database.
	CMDCODE_A2S_AUTHTOKEN_ADD	= 42, //Add node authorization token
	CMDCODE_A2S_AUTHTOKEN_CHG	= 43, //Change the permissions granted by an authorization token.
	CMDCODE_A2S_AUTHTOKEN_REVOKE= 44, //Delete node authorization token
	CMDCODE_A2S_AUTHTOKEN_LIST	= 45, //List installed authorization tokens
	CMDCODE_A2S_AUTHTOKEN_USEDBY= 46, //Get information on what nodes are using an authorization token
	CMDCODE_A2S_REMOVEUPDATE	= 47, //Tell the server to forget about an update for a certain platform string.
	CMDCODE_A2S_SRVLOG_TAIL		= 48, //Get n number of lines of the server log
	CMDCODE_A2S_SRVLOG_WIPE		= 49, //Clear server log
	CMDCODE_A2S_SRVLOG_SIZE		= 50, //Get server log size in bytes
	CMDCODE_A2S_ROUTINE_ADD		= 51, //Add a new routine for one or more nodes.
	CMDCODE_A2S_ROUTINE_DEL		= 52, //Delete a routine by its name.
	CMDCODE_A2S_ROUTINE_LIST	= 53, //List routines known to the server.
	CMDCODE_A2S_ROUTINE_CHG_TGT	= 54, //Change target nodes for routine.
	CMDCODE_A2S_ROUTINE_CHG_SCHD= 55, //Change schedule for routine
	CMDCODE_A2S_ROUTINE_CHG_FLAG= 56, //Change routine flags
	CMDCODE_A2C_HOSTREPORT		= 57, //Get the node to gather information on its host and provide it to the admin.
	CMDCODE_A2C_LISTDIRECTORY	= 58, //List the contents of a directory on the node's host system
	CMDCODE_N2N_GENERIC			= 59, //Filler command code for node to node communications, provided the right authentication tokens are in use.
	CMDCODE_A2C_EXECSNIPPET		= 60, //Execute a typed in or copy pasted chunk of Lua code
	CMDCODE_MAX
};

static_assert(sizeof(CommandCode) == sizeof(CommandCodeUint), "CommandCode is not the same size as its designated integer type!");
//Get ASCII version of it.
VLString CommandCodeToString(CommandCode CmdCode);
CommandCode StringToCommandCode(const VLString &String);

#endif //_VL_COMMANDCODES_H_
