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


/* This moment right now, Whit, is life. And life is nothing, without choice. 
 */


#ifndef __VL_COMMON_H__
#define __VL_COMMON_H__

#define MASTER_PORT 4124
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#define PATH_DIVIDER '\\'
#define PATH_DIVIDER_STRING "\\"
#else
#define PATH_DIVIDER '/'
#define PATH_DIVIDER_STRING "/"
#endif


//These strings have no special meaning, just something unique I could stuff in there. Took some inspiration from a PowerMac's Forth interpreter.
#define NODE_IDENTITY_DELIMSTART "TBXI://{{SMVLID_BI::"
#define NODE_IDENTITY_DELIMSTART_LENGTH (sizeof NODE_IDENTITY_DELIMSTART - 1)
#define NODE_IDENTITY_CAPACITY 128

#define NODE_REVISION_DELIMSTART "SGMA://{{SMVLRV_FR::"
#define NODE_REVISION_DELIMSTART_LENGTH (sizeof NODE_REVISION_DELIMSTART - 1)
#define NODE_REVISION_CAPACITY 64
#define NODE_REVISION_LENGTH 7

#define NODE_PLATFORM_STRING_DELIMSTART "UTLL://{{SMVLPS_SF::"
#define NODE_PLATFORM_STRING_DELIMSTART_LENGTH (sizeof NODE_PLATFORM_STRING_DELIMSTART - 1)
#define NODE_PLATFORM_STRING_CAPACITY 128

#define NODE_SERVERADDR_DELIMSTART "SFRR://{{SMVLSA_NN::"
#define NODE_SERVERADDR_DELIMSTART_LENGTH (sizeof NODE_SERVERADDR_DELIMSTART - 1)
#define NODE_SERVERADDR_CAPACITY 128

#define NODE_AUTHTOKEN_DELIMSTART "ESAT://{{SMSCAT_FA::"
#define NODE_AUTHTOKEN_DELIMSTART_LENGTH (sizeof NODE_AUTHTOKEN_DELIMSTART - 1)
#define NODE_AUTHTOKEN_CAPACITY 512

#define NODE_COMPILETIME_DELIMSTART "SISF:\\{{SMVLCT_TT::"
#define NODE_COMPILETIME_DELIMSTART_LENGTH (sizeof NODE_COMPILETIME_DELIMSTART - 1)
#define NODE_COMPILETIME_CAPACITY (NODE_COMPILETIME_DELIMSTART_LENGTH + sizeof(int64_t) + 1)

#define NODE_CERT_DELIMSTART "SSCX://{{SSLCBF_NO::"
#define NODE_CERT_DELIMSTART_LENGTH (sizeof NODE_CERT_DELIMSTART - 1)
#define NODE_CERT_CAPACITY 8192

#define NODE_INITSCRIPT_DELIMSTART "SLSC://{{LNIIBS_XE::"
#define NODE_INITSCRIPT_DELIMSTART_LENGTH (sizeof NODE_INITSCRIPT_DELIMSTART - 1)
#define NODE_INITSCRIPT_CAPACITY (1024 * 128) //128KiB max script size

#include "vldebug.h"
#include "vlstrings.h"

#define TOKEN_KEYVALPAIR(a) a, #a


enum StatusCode : uint8_t //I shouldn't need to say it
{
	STATUS_INVALID		= 0		, //Invalid obviously.
	STATUS_OK			= 1 << 0, //Everything worked as commanded.
	STATUS_FAILED		= 1 << 1, //Generic error that isn't volition's fault.
	STATUS_WARN 		= 1 << 2, //Operation succeeded, but potential problems.
	STATUS_IERR 		= 1 << 3, //Internal error: Error that is the fault of Volition.
	STATUS_UNSUPPORTED 	= 1 << 4, //Node's platform doesn't support this operation, lacks necessary hardware, or required modules not embedded.
	STATUS_ACCESSDENIED = 1 << 5, //Ran into permissions issues trying to execute the operation.
	STATUS_MISSING		= 1 << 6, //Some file or other resource is missing.
	STATUS_MISUSED		= 1 << 7, //Someone tried to misuse this command. Didn't end well.
	STATUS_MAXPOPULATED = STATUS_MISUSED
};

struct NetCmdStatusPODCheat
{
	bool WorkedAtAll;
	StatusCode Status;
	VLStringPODCheat Msg;
};

struct NetCmdStatus
{
	bool WorkedAtAll;
	StatusCode Status;
	VLString Msg;

	operator bool(void) const { return this->WorkedAtAll; }
	
	NetCmdStatus(const bool WorkedAtAllIn, const StatusCode Details = STATUS_INVALID, const char *InMsg = nullptr)
				: WorkedAtAll(WorkedAtAllIn),
				Status( (Details ? Details : (WorkedAtAll ? STATUS_OK : STATUS_FAILED) ) ), Msg(InMsg)
	{
		if (!Msg.empty()) return;
		
		switch (this->Status)
		{ //No message specified, so set up a default.
			case STATUS_OK:
				this->Msg = "Request executed successfully and without incident.";
				break;
			case STATUS_FAILED:
				this->Msg = "Generic failure executing request.";
				break;
			case STATUS_IERR:
				this->Msg = "Internal error in node core executing request.";
				break;
			case STATUS_UNSUPPORTED:
				this->Msg = "Operation needed for request execution not supported on platform or is unimplemented.";
				break;
			case STATUS_MISSING:
				this->Msg = "File or other resource needed for request execution is missing.";
				break;
			case STATUS_MISUSED:
				this->Msg = "Request is malformed and/or requests a nonsensical or impossible operation.";
				break;
			case STATUS_ACCESSDENIED:
				this->Msg = "Request produced access denied error of unspecified origin.";
				break;
			default:
				this->Msg = "Unmapped request status, this is likely a serious error.";
				break;
		}
	}
};

template <typename PtrType, typename DeallocFuncType = void(*)(PtrType)>
class VLScopedPtr
{
public:
	enum AllocatorType : uint8_t
	{
		ALLOCTYPE_MALLOC,
		ALLOCTYPE_NEW,
		ALLOCTYPE_ARRAYNEW,
		ALLOCTYPE_LAMBDA,
		ALLOCTYPE_MAX
	};
private:
	PtrType Ptr;
	AllocatorType Allocator;
	DeallocFuncType DeallocFunc;

public:
	void Release(void)
	{
		if (!this->Ptr) return;
		
		this->DeallocFunc(this->Ptr);
		this->Ptr = nullptr;
		this->Allocator = ALLOCTYPE_NEW;
	}
	
	VLScopedPtr(const PtrType Input = nullptr, const AllocatorType AllocatorIn = ALLOCTYPE_NEW)
		: Ptr(Input), Allocator(AllocatorIn), DeallocFunc()
	{
		switch (this->Allocator)
		{
			case ALLOCTYPE_LAMBDA: //We shouldn't be here but let's try and do something useful
				abort();
				break;
			case ALLOCTYPE_NEW:
				this->DeallocFunc = [] (PtrType Ptr) { delete Ptr; };
				break;
			case ALLOCTYPE_ARRAYNEW:
				this->DeallocFunc = [] (PtrType Ptr) { delete[] Ptr; };
				break;
			case ALLOCTYPE_MALLOC:
				this->DeallocFunc = [] (PtrType Ptr) { free((void*)Ptr); };
				break;
			default:
				break;
		}
				
	}

	VLScopedPtr(const PtrType Input, const DeallocFuncType DeallocFunc)
		: Ptr(Input),
		Allocator(ALLOCTYPE_LAMBDA),
		DeallocFunc(DeallocFunc)
	{
	}
	
	~VLScopedPtr(void)
	{	
		this->Release();
	}

	void Encase(const PtrType Input, const AllocatorType AllocatorIn = ALLOCTYPE_NEW)
	{ //Put an existing pointer under our care
		this->Release();
		this->Ptr = Input;
		this->Allocator = AllocatorIn;
	}
	
	void Encase(const PtrType Input, const DeallocFuncType DeallocFunc)
	{ //Put an existing pointer under our care
		this->Release();
		this->Ptr = Input;
		this->Allocator = ALLOCTYPE_LAMBDA;
		this->DeallocFunc = DeallocFunc;
	}

	PtrType Forget(void)
	{ //To abort freeing the pointer so something else can take control.
		const PtrType RetVal = this->Ptr;
		this->Ptr = nullptr;
		this->Allocator = ALLOCTYPE_NEW;
		this->DeallocFunc = nullptr;
		
		return RetVal;
	}

	operator PtrType(void) const { return this->Ptr; }

	decltype(*(PtrType)0) &operator[](const size_t Element) const
	{ //Some of you consider this ugly, but I consider it clever.
		return this->Ptr[Element];
	}
	decltype(*(PtrType)0) &operator*(void) const
	{
		return *this->Ptr;
	}
	
	PtrType operator->(void) const { return this->Ptr; }
	
	VLScopedPtr(VLScopedPtr &&Ref) : Ptr(Ref.Ptr), Allocator(Ref.Allocator), DeallocFunc(Ref.DeallocFunc)
	{
		Ref.Ptr = nullptr;
		Ref.Allocator = ALLOCTYPE_NEW;
		Ref.DeallocFunc = nullptr;
	}

	VLScopedPtr &operator=(VLScopedPtr &&Ref)
	{
		if (this == &Ref) return *this;
		
		this->Release();
		
		this->Ptr = Ref.Ptr;
		this->Allocator = Ref.Allocator;
		this->DeallocFunc = Ref.DeallocFunc;
		
		Ref.Ptr = nullptr;
		Ref.Allocator = ALLOCTYPE_NEW;
		Ref.DeallocFunc = nullptr;
		
		return *this;
	}
		
	
private: //No copy operations please.
	VLScopedPtr &operator=(const VLScopedPtr&);
	VLScopedPtr(const VLScopedPtr&);
};



VLString StatusCodeToString(const StatusCode Code);
StatusCode StringToStatusCode(const VLString &String);

static_assert(sizeof(NetCmdStatus) == sizeof(NetCmdStatusPODCheat), "NetCmdStatusPODCheat is not the same size as NetCmdStatus. Did you make NetCmdStatus non-standard-layout?");

#endif //__VL_COMMON_H__
