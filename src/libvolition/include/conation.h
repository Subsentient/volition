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


/* I know you're listening. You could have chosen any part of any one of us.
 * But you chose the worst parts of human behavior to use as your voice.
 * You chose hatred. You chose violence. You chose conflict.
 * You had an inability to see the bigger picture.
 * You had a down-right fucking refusal to see beyond the present.
 * You couldn't get past your own selfish, ignorant need to dominate.
 * If you had time to grow, time to evolve, things could have been different...
 * if we could have worked together.
 */

#ifndef __VL_ENCODINGS_H__
#define __VL_ENCODINGS_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "common.h"
#include "commandcodes.h"
#include "netcore.h"
#include "vlthreads.h"

#include <vector>

///This protocol and its pieces is collectively called "Conation".

namespace Conation
{	
	typedef uint16_t ArgTypeUint;
	
	enum ArgType : ArgTypeUint
	{
		ARGTYPE_NONE			= 0,
		ARGTYPE_NETCMDSTATUS	= 1 << 0,
		ARGTYPE_INT32 			= 1 << 1,
		ARGTYPE_UINT32 			= 1 << 2,
		ARGTYPE_INT64 			= 1 << 3,
		ARGTYPE_UINT64 			= 1 << 4,
		ARGTYPE_BINSTREAM 		= 1 << 5,
		ARGTYPE_STRING 			= 1 << 6,
		ARGTYPE_FILEPATH		= 1 << 7,
		ARGTYPE_SCRIPT			= 1 << 8,
		ARGTYPE_FILE			= 1 << 9,
		ARGTYPE_BOOL 			= 1 << 10,
		ARGTYPE_ODHEADER		= 1 << 11, //This is the "origin-destination header", does what it sounds like.
		ARGTYPE_MAXPOPULATED	= ARGTYPE_ODHEADER,
		ARGTYPE_ANY				= (ArgTypeUint)~0u //I really doubt we'll use this one often.
	};
	
	//Constants

	const int PROTOCOL_VERSION = 3; //Anything that breaks authentication of old nodes is reason to increment this.

	///The stream header layout is defined as follows. These are in the correct order.
	///Command code, stream total size, and ident. The actual identifier in the ident is the 56 rightmost bits,
	///and the leftmost bit is the "IsCmdReport" bit. The 8 leftmost bits are reserved for "flags", as described below STREAM_HEADER_SIZE.
	const size_t STREAM_HEADER_SIZE = (sizeof(CommandCode) + sizeof(uint64_t) + sizeof(uint64_t));
	
	
	const uint8_t IDENT_ISREPORT_BIT = 1 << 0; ///This is just 1 so we can use "true" in flags and have it be valid, since this is by far the most used flag.
	const uint8_t IDENT_ISROUTINE_BIT = 1 << 1; //It's a routine
	const uint8_t IDENT_ISN2N_BIT = 1 << 2; //Node to node communications
	
	//Written with sizeof for clarity.
	const uint8_t IDENT_NUM_FLAGS_BITS = sizeof(uint8_t) * 8; /**8**/ //Number of bits used for the flags section.
	const uint8_t IDENT_NUM_IDENT_BITS = (sizeof(uint64_t) * 8) - (sizeof(uint8_t) * 8); /**56**/ //Number of bits used for the identifier/numeric part.
	
	const uint64_t IDENT_MAX_IDENT_VALUE = ((~0ull) << 8) >> 8; ///72057594037927935
	
	inline uint8_t GetIdentFlags(const uint64_t IdentComposite)
	{
		return (IdentComposite >> (IDENT_NUM_IDENT_BITS));
	}
	
	inline uint64_t GetIdentInteger(const uint64_t IdentComposite)
	{
		return (IdentComposite << (IDENT_NUM_FLAGS_BITS)) >> (IDENT_NUM_FLAGS_BITS);
	}
	
	inline uint64_t BuildIdentComposite(const uint8_t Flags, const uint64_t Ident)
	{
		uint64_t New = Ident;
		New |= (uint64_t)Flags << (IDENT_NUM_IDENT_BITS);
		
		return New;
	}
	
	inline void DecodeIdentComposite(const uint64_t Composite, uint8_t *Flags, uint64_t *Ident)
	{
		if (Flags) *Flags = GetIdentFlags(Composite);
		if (Ident) *Ident = GetIdentInteger(Composite);
	}
	
	
	class ConationStream
	{
	private:
	
		///Data members
		std::vector<uint8_t> *Bytes;
		uint8_t *Index;
		
		//Static data members
		static uint64_t MaxStreamArgsSize;
		
		///Member prototypes
		static void EncodeSize(const uint64_t Size, std::vector<uint8_t> *Vector);
		uint64_t DecodeSize(const bool AdjustIndex);
		static bool CheckValidSize(const ArgType Type, const uint64_t Size);
		
		bool CheckInRange(uint64_t Forward);
		
		static void EncodeArgType(const ArgType Type, std::vector<uint8_t> *Vector);
		ArgType DecodeArgType(const bool AdjustIndex);
		
		static void EncodeBlob(const void *Stream, const uint64_t NumBytes,
								std::vector<uint8_t> *Vector, const size_t ExtraSpaceAfter = 8);
		static uint8_t *AllocateBlobOnly(const uint64_t RequestedSize,
										std::vector<uint8_t> *Vector,
										const size_t ExtraSpaceAfter = 8);
		
		void SetStreamArgsSize(const uint64_t NewSize);
		void AutoSetStreamArgsSize(void); //Recalculates the stream size based on the size of the internal array.
	public:

		//User usable integer thingy.
		uint64_t ExtraInteger;
		//User usable pointer thingy, for whatever really.
		void *ExtraPointer;
		
		uint64_t GetStreamArgsSize(void) const;
		static inline void SetMaxStreamArgsSize(const size_t NewLimit) { ConationStream::MaxStreamArgsSize = NewLimit; }
		static size_t GetMaxStreamArgsSize(void) { return ConationStream::MaxStreamArgsSize; }
		
		///Types

		struct ODHeader
		{
			VLString Origin;
			VLString Destination;
		};
		
		struct BaseArg
		{
			ArgType Type;
			template <typename ToCast> const ToCast &ReadAs(void) const { return static_cast<const ToCast&>(*this); }
			virtual ~BaseArg() = default;
		};

		//Argument types/classes
		struct NetCmdStatusArg : public BaseArg { StatusCode Code; VLString Msg; };
		
		struct Int32Arg : public BaseArg { int32_t Value; };
		struct Uint32Arg : public BaseArg { uint32_t Value; };
		struct Int64Arg : public BaseArg { int64_t Value; };
		struct Uint64Arg : public BaseArg { uint64_t Value; };
		
		struct StringArg : public BaseArg { VLString String; };
		struct ScriptArg : public BaseArg { VLString String; };
		struct FilePathArg : public BaseArg { VLString Path; };
		
		struct BinStreamArg : public BaseArg
		{
			uint8_t *Data;
			uint64_t DataSize;
			bool MustFree; //Do we need to delete Data?
			bool UseMallocFree; //use C's malloc? if not, we use C++'s delete[].
			
			inline BinStreamArg(const bool MustFreeIn = false, const bool UseMallocFreeIn = false)
						: Data(), DataSize(), MustFree(MustFreeIn), UseMallocFree(UseMallocFreeIn)
			{}
			
			inline BinStreamArg(const void *DataIn, const uint64_t DataSizeIn, const bool MustFreeIn = false, const bool UseMallocFreeIn = false)
						: Data((uint8_t*)DataIn), DataSize(DataSizeIn), MustFree(MustFreeIn), UseMallocFree(UseMallocFreeIn)
			{}
			
			virtual ~BinStreamArg(void);
			BinStreamArg(const BinStreamArg &Ref);
			BinStreamArg(BinStreamArg &&Ref);
			
			BinStreamArg &operator=(const BinStreamArg &Ref);
			BinStreamArg &operator=(BinStreamArg &&Ref);
		};
		
		struct BoolArg : public BaseArg { bool Value; };
		
		struct FileArg : public BinStreamArg { VLString Filename; };
		
		struct ODHeaderArg : public BaseArg { ODHeader Hdr; };
		
		//Error types
		class Err_Base {};
		class Err_CorruptStream : public Err_Base {};
		class Err_StreamNotReady : public Err_Base {};
		class Err_Misused : public Err_Base {};
		class Err_StreamDownloadFailure : public Err_Base {};
		class Err_MaxStreamArgsSizeExceeded : public Err_Base
		{
		public:
			uint64_t Max, Provided;
			Err_MaxStreamArgsSizeExceeded(const uint64_t InProvided) : Max(MaxStreamArgsSize), Provided(InProvided) {}
		};
		
		struct StreamHeader
		{ //Don't reorder this stuff. Period.
			size_t StreamArgsSize;
			uint64_t CmdIdent;
			uint8_t CmdIdentFlags;
			CommandCode CmdCode;
		};
		
		///Constructors
		ConationStream(const CommandCode Cmd, const uint8_t Flags, const uint64_t Ident, const size_t ExtraBytesAfter = 32);
		ConationStream(const uint8_t *Stream, const size_t ExtraBytesAfter = 32);
		ConationStream(const StreamHeader &Header, const uint8_t *Stream = nullptr, const size_t ExtraBytesAfter = 32);
		ConationStream(const Net::ClientDescriptor &SocketDescriptor, Net::NetRWStatusForEachFunc StatusFunc = nullptr, void *PassAlongWith = nullptr);
		ConationStream(void);
		inline ~ConationStream(void) { delete this->Bytes; }
		
		/**Copy constructors/copy assignment operators.**/
		ConationStream(const ConationStream &Ref);
		ConationStream(ConationStream &&Ref);
		ConationStream &operator=(const ConationStream &Ref);
		ConationStream &operator=(ConationStream &&Ref);
		
		///Functions
		static BaseArg *NewArgStructFromType(const ArgType Type);
		
		inline void Rewind(void) { Index = nullptr; }
		
		BaseArg *PopArgument(const bool Peek = false);
		size_t CountArguments(void) const;
		
		bool VerifyArgTypes(const std::vector<ArgType> &List) const;
		
		bool VerifyArgTypePattern(const size_t ArgOffset, const std::vector<ArgType> &List) const;
		
		bool VerifyArgTypesStartWith(const std::vector<ArgType> &List) const;
		
		std::vector<ArgType> *GetArgTypes(void) const;
		bool Transmit(const Net::ClientDescriptor &Descriptor, Net::NetRWStatusForEachFunc StatusFunc = nullptr, void *PassAlongWith = nullptr) const;
		
		//Write operations
		void Push_Bool(const bool Value, const size_t ExtraSpaceAfter = 8);
		void Push_NetCmdStatus(const NetCmdStatus &Code, const size_t ExtraSpaceAfter = 8);
		void Push_Int32(const int32_t Integer, const size_t ExtraSpaceAfter = 8);
		void Push_Uint32(const uint32_t Integer, const size_t ExtraSpaceAfter = 8);
		void Push_Int64(const int64_t Integer, const size_t ExtraSpaceAfter = 8);
		void Push_Uint64(const uint64_t Integer, const size_t ExtraSpaceAfter = 8);
		
		void Push_File(const char *Filename, const uint8_t *FileData, const size_t FileSize, const size_t ExtraSpaceAfter = 8);
		void Push_File(const char *Path, const size_t ExtraSpaceAfter = 8);
		void Push_BinStream(const void *Stream, const size_t Size, const size_t ExtraSpaceAfter = 8);
		void Push_String(const char *String, const size_t ExtraSpaceAfter = 8);
		void Push_FilePath(const char *Path, const size_t ExtraSpaceAfter = 8);
		void Push_ODHeader(const char *Origin, const char *Destination, const size_t ExtraSpaceAfter = 8);
		void Push_Script(const char *ScriptText, const size_t ExtraSpaceAfter = 8);
		void PushArgument(BaseArg *Arg, const size_t ExtraSpaceAfter = 8);
		
		//Read operations.
		NetCmdStatus Pop_NetCmdStatus(void);
		int32_t Pop_Int32(void);
		uint32_t Pop_Uint32(void);
		int64_t Pop_Int64(void);
		uint64_t Pop_Uint64(void);
		VLString Pop_String(void);
		VLString Pop_Script(void);
		VLString Pop_FilePath(void);
		ODHeader Pop_ODHeader(void);
		bool Pop_Bool(void);
		FileArg Pop_File(void);
		BinStreamArg Pop_BinStream(void);
				
		inline CommandCode GetCommandCode(void) const { return static_cast<CommandCode>((*Bytes)[0]); }
		void GetCommandIdent(uint8_t *OutFlags, uint64_t *OutIdent) const;
		uint64_t GetCmdIdentOnly(void) const;
		uint64_t GetCmdIdentComposite(void) const;
		uint8_t GetCmdIdentFlags(void) const;
		StreamHeader GetHeader(void) const;
		
		void WipeArgs(void);
		
		const std::vector<uint8_t> &GetData(void) const { return *this->Bytes; }
		const uint8_t *GetArgData(void) const;
		const uint8_t *GetSeekedArgData(void) const;
		
		void AlterHeader(const StreamHeader &Hdr);

		//Merges the seeked argument data of the input into this stream.
		void AppendArgData(const Conation::ConationStream &Input);

		void IntegrityCheck(void) const;
		///Friends

	};
	
	class ConationQueue
	{
	private:
		std::queue<VLScopedPtr<ConationStream*>> Queue;
		VLThreads::Mutex Mutex;
		VLThreads::Semaphore PushEvent;
		
	public:
		inline void Push(ConationStream *Stream)
		{
			VLThreads::MutexKeeper Keeper { &this->Mutex };

			this->Queue.push(Stream);
			
			this->PushEvent.Post();
		}
		
		inline void Push(const ConationStream &Stream)
		{
			this->Push(new ConationStream(Stream));
		}
		
		inline ConationStream *Pop(const bool ActuallyPop = true)
		{
			VLThreads::MutexKeeper Keeper { &this->Mutex };
			
			if (this->Queue.empty()) return nullptr;

			ConationStream *Result = nullptr;
			
			if (ActuallyPop)
			{
				Result = this->Queue.front().Forget();
				this->Queue.pop();
			}
			else
			{
				Result = new ConationStream(*this->Queue.front()); //WE MUST COPY IT!!! If we don't, it will probably get popped afterwards.
			}
			
		
			return Result;
		}
		
		inline ConationStream *WaitPop(void)
		{
			this->PushEvent.Wait();
			
			VLThreads::MutexKeeper Keeper { &this->Mutex };
			
			//Steal the pointer.
			ConationStream *const Result = this->Queue.front().Forget();
			
			this->Queue.pop();
			
			return Result;
		}
	};
}

VLString ArgTypeToString(const Conation::ArgType Type);
Conation::ArgType StringToArgType(const VLString &String);

#endif //__VL_ENCODINGS_H__
