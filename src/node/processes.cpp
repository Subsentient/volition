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


#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#include <tlhelp32.h>
#include <winbase.h>
#include <aclapi.h>

#elif defined(FREEBSD)

#else

#include <signal.h>
#include <dirent.h> //We scan /proc
#include <pwd.h>
#include <unistd.h>

#endif //WIN32



#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"

#include "processes.h"

#include <vector> //Already included from processes.h, but for documentation. You'll see that a lot.

NetCmdStatus Processes::ExecuteCmd(const char *Command, VLString &CmdOutput_Out)
{

#ifdef WIN32
#define POPEN_FUNC _popen
#define PCLOSE_FUNC _pclose
#else
#define POPEN_FUNC popen
#define PCLOSE_FUNC pclose
#endif //WIN32

	FILE *CmdStdout = POPEN_FUNC(Command, "r");

	if (!CmdStdout) return { false, STATUS_IERR };

	int Char = 0;

	VLString Output(128);

	size_t Capacity = Output.GetCapacity();
	size_t Length = 0;
	
	while ((Char = fgetc(CmdStdout)) != EOF)
	{
		if (Length >= Capacity)
		{
			Output.ChangeCapacity(Capacity * 2);
			Capacity = Output.GetCapacity();
		}
		
		Output[Length++] = Char;
	}

	if (!Output)
	{
		Output = "Command returned no output.";
	}

	Output += "\n\n";
	
	const int ExitCode = PCLOSE_FUNC(CmdStdout);

	Output += VLString("Exit code for process was ") + VLString::IntToString(ExitCode) + '\n';

	CmdOutput_Out = Output;
	
	return !ExitCode;
}

NetCmdStatus Processes::KillProcessByName(const char *ProcessName)
{
#if defined(WIN32)
	VLScopedPtr<uint8_t*, decltype(&CloseHandle)> ProcSnap { (uint8_t*)CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0lu), CloseHandle };
	
	if (!ProcSnap) return NetCmdStatus(false, STATUS_IERR);
	
	
	PROCESSENTRY32 Entry{};
	
	Entry.dwSize = sizeof Entry;
	
	bool HasSomething = Process32First(ProcSnap, &Entry);
	
	bool Found = false;
	
	for (; HasSomething; HasSomething = Process32Next(ProcSnap, &Entry))
	{
		if (!strcmp(Entry.szExeFile, ProcessName))
		{
			VLScopedPtr<uint8_t*, decltype(&CloseHandle)> Handle
			{
				(uint8_t*)OpenProcess(PROCESS_TERMINATE, 0, (DWORD)Entry.th32ProcessID),
				CloseHandle
			};
			
			if (Handle)
			{
				TerminateProcess(Handle, 9); //Signal 9, as in UNIX's SIGKILL? Huh.
			}
			else
			{
				return NetCmdStatus(false, STATUS_FAILED);
			}
			
			
			Found = true;
		}
	}
	
	return Found ? NetCmdStatus(true) : NetCmdStatus(false, STATUS_MISSING);

#elif defined(LINUX)

	VLScopedPtr<DIR*, decltype(&closedir)> CurDir { opendir("/proc"), closedir };
	
	if (!CurDir) return NetCmdStatus(false, STATUS_MISSING); // our /proc isn't mounted.
	
	struct dirent *DirPtr = nullptr;
	
	bool Found = false;
	while ((DirPtr = readdir(CurDir)))
	{
		if (!Utils::StringAllNumeric(DirPtr->d_name)) continue;
		
		const VLString &Path = VLString("/proc/") + const_cast<const char*>(DirPtr->d_name) + "/cmdline";
		
		std::vector<char> Buffer;
		Buffer.reserve(2048);
	
		VLScopedPtr<FILE*, int(*)(FILE*)> Desc { fopen(Path, "rb"), fclose };
		if (!Desc) continue;
		
		int Char = 0;
		while ((Char = getc(Desc)) != EOF && Char != '\0')
		{
			Buffer.push_back(Char);
		}
		Buffer.push_back('\0'); //Null terminate
		
		if (!strcmp(&Buffer[0], ProcessName))
		{
			if (kill(atol(DirPtr->d_name), SIGKILL) != 0)
			{
				switch (errno)
				{
					case EPERM:
						return NetCmdStatus(false, STATUS_ACCESSDENIED);
						break;
					case ESRCH:
						return NetCmdStatus(false, STATUS_IERR);
						break;
					default:
						return false;
				}
			}
			
			Found = true;
			
		}
	}
	
	return Found ? NetCmdStatus(true) : NetCmdStatus(false, STATUS_FAILED);
#else
	return NetCmdStatus(false, STATUS_UNSUPPORTED);
	
#endif
}

NetCmdStatus Processes::GetProcessesList(std::list<Processes::ProcessListMember> *OutList_)
{
	std::list<Processes::ProcessListMember> &OutList = *OutList_;
	
	OutList.clear();
#if defined(WIN32)
	VLScopedPtr<uint8_t*, decltype(&CloseHandle)> ProcSnap { (uint8_t*)CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0lu), CloseHandle};
	
	if (!ProcSnap) return NetCmdStatus(false, STATUS_IERR);
	
	
	PROCESSENTRY32 Entry = PROCESSENTRY32();
	
	Entry.dwSize = sizeof Entry;
	
	bool HasSomething = Process32First(ProcSnap, &Entry);
	
	for (; HasSomething; HasSomething = Process32Next(ProcSnap, &Entry))
	{
		OutList.push_back(ProcessListMember());
		
		ProcessListMember *Ptr = &OutList.back();
		
		Ptr->ProcessName = Entry.szExeFile;
		Ptr->PID = Entry.th32ProcessID;
		
		//We have no idea how to detect these.
		Ptr->KernelProcess = false;
		
		//Now find the user that owns this.
		VLScopedPtr<uint8_t*, decltype(&CloseHandle)> Handle { (uint8_t*)OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, (DWORD)Entry.th32ProcessID), CloseHandle };
		
		if (Handle)
		{
			HANDLE TokenHandle_{};
			
			if (!OpenProcessToken(Handle, TOKEN_QUERY, &TokenHandle_))
			{
				goto CantGetUser;
			}

			VLScopedPtr<uint8_t*, decltype(&CloseHandle)> TokenHandle { (uint8_t*)TokenHandle_, CloseHandle };

			DWORD BufferLength = 0;
			
			if (!GetTokenInformation(TokenHandle, TokenUser, nullptr, 0, &BufferLength))
			{
				if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
				{
					goto CantGetUser;
				}
			}
			
			VLScopedPtr<TOKEN_USER*> UserTokenPtr { (TOKEN_USER*)malloc(BufferLength), VL_ALLOCTYPE_MALLOC };
			
			if (!GetTokenInformation(TokenHandle, TokenUser, UserTokenPtr, BufferLength, &BufferLength))
			{
				goto CantGetUser;
			}
			
			VLString Username(1024);
			VLString Domain(Username.GetCapacity());
			
			DWORD Capacity = Username.GetCapacity() - 1;
			DWORD DomainCapacity = Capacity;
			
			SID_NAME_USE SIDType{};
			
			if (!LookupAccountSid(nullptr, UserTokenPtr->User.Sid, Username.GetBuffer(), &Capacity, Domain.GetBuffer(), &DomainCapacity, &SIDType))
			{
				goto CantGetUser;
			}
			
			Ptr->User = Username;
		}
		else
		{
		CantGetUser:
			Ptr->User = "<unknown>";
		}
	}
	
	return true; //Everything went perfectly.
#elif defined(LINUX)

	VLScopedPtr<DIR*, decltype(&closedir)> CurDir { opendir("/proc"), closedir }; 
	
	if (!CurDir) return NetCmdStatus(false, STATUS_MISSING); // our /proc isn't mounted.
	
	struct dirent *DirPtr = nullptr;
	
	while ((DirPtr = readdir(CurDir)))
	{
		if (!Utils::StringAllNumeric(DirPtr->d_name)) continue;
		
		const VLString &CmdlinePath = VLString("/proc/") + const_cast<const char*>(DirPtr->d_name) + "/cmdline";
		
		VLScopedPtr<std::vector<uint8_t>*> FileBuffer { Utils::Slurp(CmdlinePath) };
		
		if (!FileBuffer)
		{
			const NetCmdStatus Ret(false, STATUS_FAILED);
			OutList.clear();
			return Ret;
		}
		
		OutList.push_back(ProcessListMember());
		
		ProcessListMember *NewMember = &OutList.back();
		
		NewMember->PID = atol(DirPtr->d_name);
		NewMember->ProcessName = (const char*)&FileBuffer->at(0);
		
		const VLString &StatusPath = VLString("/proc/") + const_cast<const char*>(DirPtr->d_name) + "/status";
		
		FileBuffer = Utils::Slurp(StatusPath);
		
		if (!FileBuffer)
		{
			const NetCmdStatus Ret(false, STATUS_FAILED);
			OutList.clear();
			return Ret;
		}
		
		const VLString &LookFor = VLString("Uid:") + '\t';
		
		const char *Search = strstr((const char*)&FileBuffer->at(0), LookFor);
		
		if (!Search)
		{
			OutList.clear();
			const NetCmdStatus Ret(false, STATUS_FAILED);
			return Ret;
		}
		
		Search += strlen(LookFor);
		
		while (isblank(*Search)) ++Search;
		
		size_t Inc = 0;
		
		char UIDBuf[64]{};
		
		for (; Search[Inc] && isdigit(Search[Inc]) && Inc < sizeof UIDBuf - 1; ++Inc)
		{
			UIDBuf[Inc] = Search[Inc];
		}
		UIDBuf[Inc] = '\0';
		
		//Get the kernel thread name if our name is empty.
		if (!NewMember->ProcessName)
		{
			const VLString &LookFor_Name = VLString("Name:") + '\t';
			
			Search = strstr((const char*)FileBuffer->data(), LookFor_Name);
			
			if (!Search)
			{
				OutList.clear();
				const NetCmdStatus Ret(false, STATUS_IERR);
				return Ret;
			}
	
			Search += strlen(LookFor_Name);
			
			while (isblank(*Search)) ++Search;
			
			VLString KernelThreadNameBuf(1024);
			
			for (Inc = 0; Inc < KernelThreadNameBuf.GetCapacity() - 1 && Search[Inc] && Search[Inc] != '\n'; ++Inc)
			{
				*(KernelThreadNameBuf++) = Search[Inc];
			}
			*KernelThreadNameBuf = '\0';
			
			KernelThreadNameBuf.Rewind();
			
			NewMember->ProcessName = KernelThreadNameBuf;
			NewMember->KernelProcess = true;
		}
		else NewMember->KernelProcess = false;
			
		
		if (!*UIDBuf || !Utils::StringAllNumeric(UIDBuf))
		{
			OutList.clear();
			const NetCmdStatus Ret(false, STATUS_FAILED);
			return Ret;
		}
		
		struct passwd *UserLookup = getpwuid(atol(UIDBuf));
		
		if (!UserLookup)
		{
			const NetCmdStatus Ret(false, STATUS_FAILED);
			OutList.clear();
			return Ret;
		}
		
		NewMember->User = UserLookup->pw_name;
	}
	
	return true;
#else
	return NetCmdStatus(false, STATUS_UNSUPPORTED);
#endif //defined(WIN32)
}
