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
#include "../libvolition/include/netcore.h"
#include "../libvolition/include/conation.h"
#include "jobs.h"
#include "files.h"
#include "main.h"
#include "interface.h"
#include "identity_module.h"

#include <time.h>
#include <unistd.h> //For execlp()
#ifdef WIN32
#include <windows.h> //For GetModuleFileName()
#else
#include <sys/stat.h> //For chmod()
#endif //WIN32
#include "updates.h"

#include <vector>

//Types
enum RecoveryMode : int { UPDATE_X_STAGE1, UPDATE_X_STAGE2 };

//Globals
static const char *const UpdateParam = strdup(UPDATE_PARAM);

//prototypes
static NetCmdStatus InitStage1(const Conation::ConationStream::FileArg &File);
static void InitStage2(const VLString &OriginalBinaryPath, const VLString &TempBinaryPath);
static void CompleteUpdate(const VLString &TempBinaryPath);
static VLString GetNewTempBinaryPath(void);

static void ExecuteBinary(const VLString &Path, const std::vector<VLString> &ExtraArguments);

//Function definitions


static VLString GetNewTempBinaryPath(void)
{
	VLString NewPath(2048);

	NewPath = Utils::GetTempDirectory() + "vl_" + VLString::IntToString(~time(nullptr) << 1);
	
#ifdef WIN32
	NewPath += ".exe";
#endif //WIN32

	NewPath.ShrinkToFit();
	
	return NewPath;
}

static void ExecuteBinary(const VLString &Path, const std::vector<VLString> &ExtraArguments)
{
#ifdef WIN32
	//Farts
	VLString SuperArgument(8192);
	
	SuperArgument += '"';
	SuperArgument += Path;
	SuperArgument += "\" ";
	
	for (size_t Inc = 0; Inc < ExtraArguments.size(); ++Inc)
	{
		SuperArgument += '"';
		SuperArgument += ExtraArguments[Inc];
		SuperArgument += '"';
		
		if (Inc + 1 != ExtraArguments.size())
		{
			SuperArgument += ' ';
		}
	}
	
	SuperArgument.ShrinkToFit();

#ifdef DEBUG
	puts(VLString("Final value of SuperArgument: ") + SuperArgument);
#endif //DEBUG

	STARTUPINFO StartupInfo{};
	StartupInfo.cb = sizeof StartupInfo;
	StartupInfo.hStdInput = nullptr;
	StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
	
	PROCESS_INFORMATION ProcessInfo{};
	
	const bool Succeeded = CreateProcess(nullptr,
										const_cast<char*>(+SuperArgument),
										nullptr,
										nullptr,
										true,
										0,
										nullptr,
										nullptr,
										&StartupInfo,
										&ProcessInfo);
#ifdef DEBUG
	puts(VLString("Call to CreateProcess() ") + (Succeeded ? "succeeded" : "failed"));
#endif //DEBUG

	exit(!Succeeded);

#ifdef DEBUG
	puts("Landed after exit()! This is impossible!");
#endif //DEBUG

#else //Not win32
	std::vector<char*> Params;
	Params.reserve(ExtraArguments.size() + 2);
	
	Params.push_back(const_cast<char*>(+Path));
	
	for (size_t Inc = 0; Inc < ExtraArguments.size(); ++Inc)
	{
		Params.push_back(const_cast<char*>(+ExtraArguments[Inc]));
	}
	
	Params.push_back(nullptr);
	
	execvp(+Path, Params.data());
#endif //WIN32	
}
void Updates::HandleUpdateRecovery(void)
{
	int *argc;
	char **argv = Main::GetArgvData(&argc);
	
	if (*argc <= 1 || VLString(argv[1]) != UpdateParam)
	{
		return;
	}
	
#ifdef WIN32
	//Windows needs a little for the old process to die. Fuck race conditions.
	Utils::vl_sleep(1000);
#endif

#ifdef DEBUG
	puts("Updates::HandleUpdateRecovery(): Entered function body after recovery mode detected.");
#endif
	const RecoveryMode Mode = static_cast<RecoveryMode>(VLString::StringToInt(argv[2]));
	
	switch (Mode)
	{
		case UPDATE_X_STAGE1:
		{ //We're running as the temp binary.
#ifdef DEBUG
			puts("Updates::HandleUpdateRecovery(): Detected Stage 1 update action");
#endif

			if (*argc != 5)
			{
#ifdef DEBUG
				puts(VLString("Updates::HandleUpdateRecovery(): Argument count mismatch for stage 1. Got ") + VLString::IntToString(*argc));
				
				for (int Inc = 0; Inc < *argc; ++Inc)
				{
					puts(VLString("Argument ") + VLString::IntToString(Inc) + ": \"" + (const char*)argv[Inc] + "\".");
				}
#endif
				return;
			}

			InitStage2(argv[3], argv[4]);
			break;
		}
		case UPDATE_X_STAGE2:
		{ //We're running as the original, updated binary.
			
			if (*argc != 4)
			{
#ifdef DEBUG
				puts(VLString("Updates::HandleUpdateRecovery(): Argument count mismatch for stage 2. Got ") + VLString::IntToString(*argc));
				
				for (int Inc = 0; Inc < *argc; ++Inc)
				{
					puts(VLString("Argument ") + VLString::IntToString(Inc) + ": \"" + (const char*)argv[Inc] + "\".");
				}
#endif
				return;
			}
			
			CompleteUpdate(argv[3]);
			break; //Shouldn't really reach here.
		}
		default:
#ifdef DEBUG
			//Cuz we probably wanna see what happened.
			puts("Updates::HandleUpdateRecovery(): Recovery mode unknown, aborting.");
			Utils::vl_sleep(10000);
			exit(1);
#endif
			break;
	}
}


static void CompleteUpdate(const VLString &TempBinaryPath)
{
#ifdef DEBUG
	puts("Entered CompleteUpdate()");
#endif //DEBUG
	Files::Delete(TempBinaryPath);
	Main::Begin(true);
}

static NetCmdStatus InitStage1(const Conation::ConationStream::FileArg &File)
{
	vlassert(File.DataSize > 0 && File.Data);
	
	NetCmdStatus RetVal(false);
	
	const VLString &SelfBinaryPath = Main::GetCurrentBinaryPath();
	
	if (!SelfBinaryPath)
	{
		RetVal = NetCmdStatus(false, STATUS_ACCESSDENIED
#ifdef DEBUG
								, "Unable to find location of current binary!"
#endif
							);
#ifdef DEBUG
		puts("InitStage1(): Unable to find location of current binary!");
#endif //DEBUG
		return RetVal;
	}

	if (!Utils::FileExists(SelfBinaryPath))
	{
#ifdef DEBUG
		puts(VLString("InitStage1(): Failed to stat original binary at path \"") + SelfBinaryPath + "\"!");
#endif //DEBUG
		RetVal = NetCmdStatus(false, STATUS_MISSING
#ifdef DEBUG
							, VLString("Our reported binary \"") + SelfBinaryPath + "\" was not found at its reported location!"
#endif
							);
		return RetVal;
	}
	
	const VLString &TempBinaryPath = GetNewTempBinaryPath();
	
	if (!Utils::WriteFile(TempBinaryPath, File.Data, File.DataSize))
	{
		RetVal = NetCmdStatus(false, STATUS_IERR
#ifdef DEBUG
							, VLString("Unable to write temporary binary to \"") + TempBinaryPath + "\"!"
#endif
							);
#ifdef DEBUG
		puts(VLString("InitStage1(): ") + RetVal.Msg);
#endif
		return RetVal;
	}
	
#ifndef WIN32
	//We need to set the permissions to executable.
	chmod(TempBinaryPath, 0700);
#endif

	std::vector<VLString> Arguments = 	{
											UpdateParam,
											VLString::IntToString(UPDATE_X_STAGE1),
											SelfBinaryPath,
											TempBinaryPath
										};
	ExecuteBinary(TempBinaryPath, Arguments);
	
	RetVal = NetCmdStatus(false, STATUS_IERR
#ifdef DEBUG
	, VLString("Failed to ExecuteBinary(), \"") + TempBinaryPath + "\", we landed after the call!"
#endif
						);

#ifdef DEBUG
	puts(VLString("InitStage1(): ") + RetVal.Msg);
#endif //DEBUG

	return RetVal;
}


static void InitStage2(const VLString &OriginalBinaryPath, const VLString &TempBinaryPath)
{
	if (!Files::Copy(TempBinaryPath, OriginalBinaryPath))
	{
		exit(1); //What can we really do?
	}
	
#ifndef WIN32
	//We need to set the permissions to executable.
	chmod(OriginalBinaryPath, 0700);
#endif
	
	std::vector<VLString> Arguments = 	{
											UpdateParam,
											VLString::IntToString(UPDATE_X_STAGE2),
											TempBinaryPath
										};
	ExecuteBinary(OriginalBinaryPath, Arguments);

	
#ifdef DEBUG
	puts("InitStage2(): Landed after execlp()!");
#endif //DEBUG
	exit(1); //Again, nothing we can do about it.
	
}

NetCmdStatus Updates::AttemptUpdate(const Conation::ConationStream::FileArg &File)
{ //Regardless of what we return, it's gonna be failure, cuz this shouldn't return at all.
#ifdef DEBUG
	puts("Entering Updates::AttemptUpdate()");
#endif

	vlassert(File.DataSize > 0 && File.Data);
	
	//Release the mutex by force, otherwise we can't gracefully kill the read queue threads.
	Main::GetReadQueue().Head_Release(false);
	
	Main::MurderAllThreads();

	Conation::ConationStream DisconnectRequest(CMDCODE_ANY_DISCONNECT, 0, 0);
	DisconnectRequest.Transmit(*Main::GetSocketDescriptor());

	Net::Close(*Main::GetSocketDescriptor());
	
	const NetCmdStatus WhatFailed = InitStage1(File); //If this returns, we failed.


	while (!((*(int*)Main::GetSocketDescriptor()) = Interface::Establish(IdentityModule::GetServerAddr() ) ) ) Utils::vl_sleep(1000);

	//Now get the old update command off the top of the queue and discard it.
	Main::GetReadQueue().Head_Acquire();
	Main::GetReadQueue().Head_Release(true);
	
	return WhatFailed;
}
