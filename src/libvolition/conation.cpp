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
Welcome to the beating heart of all Volition systems and networks.
All other components are entirely dependent on the conation protocol,
and Volition wouldn't be itself without it.




										 .+ssssoooooooooo++++++++++:::::::::::::::-
							oossssssssooohhhhhhhhyyyyyyyyyyyyy*//**sssoooo++++++++/:
							`````````````/hdhhhhhhhhyyyyyyyyssssssssoooooooo+++++++/
										  ````````````````````````````````````++++++
																			  +oo+++
																			  **//*+
																			  ooooo+
																			  oooooo
																			  ossooo
																			  osssso
																			  ssssso
																			  ssssss
																			  s*//**
																			  syyyys
																			  yyyyys
																			  yyyyyy
																			  yhhyyy
																			  yhhhhy
																			  hhhhhy
																			  hdhhhh
																			  h**//*
																			  hddddh
																			  hddddh
																			  hddddd
																			  hddddd
																			  hddddd
																			  hddddd
																			  yhhddh
																			  shhhhy
																			  .+ss+.




*/


///Implementation details for the Conation protocol.

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdarg.h>
#include "include/netcore.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif //WIN32

#include "include/conation.h"
#include "include/utils.h"

uint64_t Conation::ConationStream::MaxStreamArgsSize = (1024*1024) * 512;

//Constructors

Conation::ConationStream::ConationStream(void) : Bytes(new std::vector<uint8_t>(STREAM_HEADER_SIZE)), Index(), ExtraInteger(), ExtraPointer()
{
}

Conation::ConationStream::ConationStream(const CommandCode Cmd, const uint8_t IdentFlags, const uint64_t Ident, const size_t ExtraBytesAfter) : Bytes(new std::vector<uint8_t>(STREAM_HEADER_SIZE)), Index(), ExtraInteger(), ExtraPointer()
{
	Bytes->reserve(STREAM_HEADER_SIZE + ExtraBytesAfter);
	Bytes->resize(STREAM_HEADER_SIZE);

	const StreamHeader Hdr { 0, Ident, IdentFlags, Cmd };
	
	this->AlterHeader(Hdr);

	this->IntegrityCheck();
}

Conation::ConationStream::ConationStream(const uint8_t *Stream, const size_t ExtraBytesAfter) : Bytes(new std::vector<uint8_t>(STREAM_HEADER_SIZE)), Index(), ExtraInteger(), ExtraPointer()
{
	//Get stream size.
	uint64_t StreamArgsSize = 0;

	memcpy(&StreamArgsSize, Stream + 1, sizeof(uint64_t));

	StreamArgsSize = Utils::vl_ntohll(StreamArgsSize);
	StreamArgsSize += STREAM_HEADER_SIZE;

	Bytes->reserve(StreamArgsSize + ExtraBytesAfter);
	Bytes->resize(StreamArgsSize);
	memcpy(&(*Bytes)[0], Stream, StreamArgsSize);

	this->IntegrityCheck();
}

Conation::ConationStream::ConationStream(const StreamHeader &Header, const uint8_t *Stream, const size_t ExtraBytesAfter) : Bytes(new std::vector<uint8_t>(STREAM_HEADER_SIZE)), Index(), ExtraInteger(), ExtraPointer()
{
	Bytes->reserve(Header.StreamArgsSize + STREAM_HEADER_SIZE + ExtraBytesAfter);
	Bytes->resize(Header.StreamArgsSize + STREAM_HEADER_SIZE);

	uint8_t *Buf = &(*Bytes)[0];
	const uint64_t EncodedSize = Utils::vl_htonll(Header.StreamArgsSize);

	//Encode command code.
	memcpy(Buf, &Header.CmdCode, sizeof Header.CmdCode);

	//Encode stream size.
	memcpy(Buf + sizeof(CommandCode), &EncodedSize, sizeof(EncodedSize));

	//Encode ident.
	uint64_t NewComposite = Utils::vl_htonll(BuildIdentComposite(Header.CmdIdentFlags, Header.CmdIdent));

	memcpy(Buf + sizeof(CommandCode) + sizeof(uint64_t), &NewComposite, sizeof NewComposite);

	//Copy the rest of the stream.
	if (Stream)
	{
		memcpy(Buf + STREAM_HEADER_SIZE, Stream, Header.StreamArgsSize);
	}

	this->IntegrityCheck();
}

Conation::ConationStream::ConationStream(const Net::ClientDescriptor &SocketDescriptor, Net::NetRWStatusForEachFunc StatusFunc, void *UserData) : Bytes(new std::vector<uint8_t>(STREAM_HEADER_SIZE)), Index(), ExtraInteger(), ExtraPointer()
{

	if (!SocketDescriptor.Internal)
	{
		throw Err_Misused();
	}

	std::vector<uint8_t> &Buffer = *this->Bytes;

	Buffer.resize(STREAM_HEADER_SIZE);

	if (!Net::Read(SocketDescriptor, &Buffer[0], STREAM_HEADER_SIZE))
	{
		throw Err_StreamDownloadFailure();
	}

	//Get command code
	StreamHeader Header{};
	memcpy(&Header.CmdCode, &Buffer[0], sizeof Header.CmdCode);

	//Get stream size
	uint64_t Size = 0;
	memcpy(&Size, &Buffer[sizeof(CommandCode)], sizeof Size);
	Header.StreamArgsSize = Utils::vl_ntohll(Size);

	///CHECK IF STREAM SIZE IS DDOS LENGTH!!!
	if (Header.StreamArgsSize > MaxStreamArgsSize)
	{
		throw Err_MaxStreamArgsSizeExceeded(MaxStreamArgsSize);
	}

	//Get ident.
	uint64_t IdentComposite = 0ul;
	memcpy(&IdentComposite, &Buffer[sizeof(CommandCode) + sizeof(uint64_t)], sizeof IdentComposite);

	DecodeIdentComposite(Utils::vl_ntohll(IdentComposite), &Header.CmdIdentFlags, &Header.CmdIdent);

	//Resize to fit
	Buffer.resize(STREAM_HEADER_SIZE + Header.StreamArgsSize);

	if (Header.StreamArgsSize > 0)
	{
		//Download the rest
		if (!Net::Read(SocketDescriptor, &(*Bytes)[STREAM_HEADER_SIZE], Header.StreamArgsSize, StatusFunc, (int64_t)UserData == -1 ? &Header : UserData))
		{
			throw Err_StreamDownloadFailure();
		}
	}

	this->IntegrityCheck();
}

Conation::ConationStream::ConationStream(const ConationStream &Ref) : Bytes(new std::vector<uint8_t>(*Ref.Bytes)), Index(Ref.Index ? (Bytes->data() + (Ref.Index - Ref.Bytes->data())) : nullptr), ExtraInteger(Ref.ExtraInteger), ExtraPointer(Ref.ExtraPointer)
{
	this->IntegrityCheck();
}

Conation::ConationStream &Conation::ConationStream::operator=(const ConationStream &Ref)
{
	if (&Ref == this) return *this;

	*this->Bytes = *Ref.Bytes;
	this->Index = Ref.Index ? (this->Bytes->data() + (Ref.Index - Ref.Bytes->data())) : nullptr;
	this->ExtraInteger = Ref.ExtraInteger;
	this->ExtraPointer = Ref.ExtraPointer;

	this->IntegrityCheck();
	
	return *this;
}

Conation::ConationStream &Conation::ConationStream::operator=(ConationStream &&Ref)
{

	if (&Ref == this) return *this;
	
	this->Bytes = Ref.Bytes;
	this->Index = Ref.Index;
	this->ExtraInteger = Ref.ExtraInteger;
	this->ExtraPointer = Ref.ExtraPointer;

	this->IntegrityCheck();

	Ref.Bytes = nullptr;
	Ref.Index = nullptr;
	return *this;
}

Conation::ConationStream::ConationStream(ConationStream &&Ref) : Bytes(Ref.Bytes), Index(Ref.Index), ExtraInteger(Ref.ExtraInteger), ExtraPointer(Ref.ExtraPointer)
{
	Ref.Bytes = nullptr;
	Ref.Index = nullptr;
	this->IntegrityCheck();
}

//Push functions for ConationStream
void Conation::ConationStream::Push_Bool(const bool Value,  const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_BOOL, Bytes);
	EncodeSize(sizeof(bool), Bytes); //Better be fucking one.
	EncodeBlob(&Value, 1, Bytes, ExtraSpaceAfter);
	this->AutoSetStreamArgsSize();
}

void Conation::ConationStream::Push_NetCmdStatus(const NetCmdStatus &Code, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_NETCMDSTATUS, Bytes);

	//WE DO NOT INCLUDE THE BOOL! It's redundant in this context.
	const size_t Size = sizeof(Code.Status) + Code.Msg.size();
	EncodeSize(Size, Bytes); //Better be fucking one.

	EncodeBlob(&Code.Status, sizeof Code.Status, Bytes);

	EncodeBlob(+Code.Msg, Code.Msg.size(), Bytes, ExtraSpaceAfter);

	this->AutoSetStreamArgsSize();
}

void Conation::ConationStream::Push_Int32(const int32_t Integer, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_INT32, Bytes);
	EncodeSize(sizeof(int32_t), Bytes);

	int32_t Data;
	*(uint32_t*)&Data = htonl(Integer);

	EncodeBlob(&Data, sizeof Data, Bytes, ExtraSpaceAfter);
	this->AutoSetStreamArgsSize();
}
void Conation::ConationStream::Push_Uint32(const uint32_t Integer, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_UINT32, Bytes);
	EncodeSize(sizeof(uint32_t), Bytes);

	const uint32_t Data = htonl(Integer);

	EncodeBlob(&Data, sizeof Data, Bytes, ExtraSpaceAfter);
	this->AutoSetStreamArgsSize();

}
void Conation::ConationStream::Push_Int64(const int64_t Integer, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_INT64, Bytes);
	EncodeSize(sizeof(int64_t), Bytes);
	int64_t Data;
	*(uint64_t*)&Data = Utils::vl_htonll(Integer);

	EncodeBlob(&Data, sizeof Data, Bytes, ExtraSpaceAfter);
	this->AutoSetStreamArgsSize();
}
void Conation::ConationStream::Push_Uint64(const uint64_t Integer, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_UINT64, Bytes);
	EncodeSize(sizeof(uint64_t), Bytes);
	const uint64_t Data = Utils::vl_htonll(Integer);

	EncodeBlob(&Data, sizeof Data, Bytes, ExtraSpaceAfter);
	this->AutoSetStreamArgsSize();
}

void Conation::ConationStream::Push_File(const char *Filename, const uint8_t *FileData, const size_t FileSize, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_FILE, Bytes);
	EncodeSize(FileSize + strlen(Filename) + 1, Bytes);

	EncodeBlob(Filename, strlen(Filename) + 1, Bytes, FileSize + 20); //Include null terminator.
	EncodeBlob(FileData, FileSize, Bytes, ExtraSpaceAfter);

	this->AutoSetStreamArgsSize();
}

void Conation::ConationStream::Push_File(const char *Path, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_FILE, Bytes);

	//Get file size
	uint64_t FileSize = 0u;
	if (!Utils::GetFileSize(Path, &FileSize)) throw Err_Misused();

	//Get base filename.
	VLString Filename = Utils::StripPathFromFilename(Path);

	//Set stream size.
	EncodeSize(FileSize + Filename.size() + 1, Bytes);

	EncodeBlob(Filename, Filename.Length() + 1, Bytes, FileSize + 20); //Extra cushion

	//Allocate space for file data
	uint8_t *FileSpace = AllocateBlobOnly(FileSize, Bytes, ExtraSpaceAfter);

	//Actually fetch file data.
	if (!Utils::Slurp(Path, FileSpace, FileSize))
	{
		throw Err_Misused();
	}
	this->AutoSetStreamArgsSize();
}




void Conation::ConationStream::Push_BinStream(const void *Stream, const size_t Size, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_BINSTREAM, Bytes);
	EncodeSize(Size, Bytes);
	EncodeBlob(Stream, Size, Bytes, ExtraSpaceAfter);
	this->AutoSetStreamArgsSize();
}
void Conation::ConationStream::Push_ODHeader(const char *Origin, const char *Destination, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_ODHEADER, Bytes);

	const size_t OriginSize = strlen(Origin) + 1;
	const size_t DestinationSize = strlen(Destination) + 1;

	const size_t Size = OriginSize + DestinationSize;
	EncodeSize(Size, Bytes);


	char *Buf = new char[OriginSize + DestinationSize];
	memcpy(Buf, Origin, OriginSize);
	memcpy(Buf + OriginSize, Destination, DestinationSize);


	EncodeBlob(Buf, Size, Bytes, ExtraSpaceAfter);

	delete[] Buf;

	this->AutoSetStreamArgsSize();
}
void Conation::ConationStream::Push_String(const char *String, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_STRING, Bytes);
	const size_t Size = strlen(String);
	EncodeSize(Size, Bytes);


	EncodeBlob(String, Size, Bytes, ExtraSpaceAfter);

	this->AutoSetStreamArgsSize();
}
void Conation::ConationStream::Push_Script(const char *ScriptText, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_SCRIPT, Bytes);
	const size_t Size = strlen(ScriptText);
	EncodeSize(Size, Bytes);

	EncodeBlob(ScriptText, Size, Bytes, ExtraSpaceAfter);

	this->AutoSetStreamArgsSize();
}
void Conation::ConationStream::Push_FilePath(const char *Path, const size_t ExtraSpaceAfter)
{
	EncodeArgType(ARGTYPE_FILEPATH, Bytes);
	const size_t Size = strlen(Path);

	EncodeSize(Size, Bytes);

	EncodeBlob(Path, Size, Bytes, ExtraSpaceAfter);

	this->AutoSetStreamArgsSize();
}

//More misc stuff
uint64_t Conation::ConationStream::GetCmdIdentComposite(void) const
{
	uint64_t Original;

	memcpy(&Original, &(*this->Bytes)[STREAM_HEADER_SIZE - sizeof(Original)], sizeof Original);

	return Utils::vl_ntohll(Original);
}

void Conation::ConationStream::GetCommandIdent(uint8_t *OutFlags, uint64_t *OutIdent) const
{
	uint64_t Original;

	memcpy(&Original, &(*this->Bytes)[STREAM_HEADER_SIZE - sizeof(Original)], sizeof Original);

	DecodeIdentComposite(Utils::vl_ntohll(Original), OutFlags, OutIdent);
}

uint64_t Conation::ConationStream::GetCmdIdentOnly(void) const
{
	uint64_t Original = 0;

	memcpy(&Original, &(*this->Bytes)[STREAM_HEADER_SIZE - sizeof(Original)], sizeof Original);

	uint64_t RetVal = 0;
	DecodeIdentComposite(Utils::vl_ntohll(Original), nullptr, &RetVal);

	return RetVal;
}

uint8_t Conation::ConationStream::GetCmdIdentFlags(void) const
{
	uint64_t Original = 0;

	memcpy(&Original, &(*this->Bytes)[STREAM_HEADER_SIZE - sizeof(Original)], sizeof Original);

	uint8_t Flags = 0;
	DecodeIdentComposite(Utils::vl_ntohll(Original), &Flags, nullptr);

	return Flags;
}


Conation::ConationStream::StreamHeader Conation::ConationStream::GetHeader(void) const
{
	StreamHeader RetVal;
	//Get command code
	memcpy(&RetVal.CmdCode, Bytes->data(), sizeof RetVal.CmdCode);

	//Get stream size
	uint64_t Size = 0u;
	memcpy(&Size, &Bytes->at(sizeof(CommandCode)), sizeof Size);
	RetVal.StreamArgsSize = Utils::vl_ntohll(Size);

	//Get ident.
	uint64_t IdentComposite = 0ul;
	memcpy(&IdentComposite, &Bytes->at(sizeof(CommandCode) + sizeof(uint64_t)), sizeof IdentComposite);

	DecodeIdentComposite(Utils::vl_ntohll(IdentComposite), &RetVal.CmdIdentFlags, &RetVal.CmdIdent);

	return RetVal;
}

void Conation::ConationStream::EncodeSize(const uint64_t Size, std::vector<uint8_t> *Vector)
{
	uint64_t NetSize = Utils::vl_htonll(Size);

	for (size_t Inc = 0; Inc < sizeof(NetSize); ++Inc)
	{
		Vector->push_back(*((uint8_t*)&NetSize + Inc));
	}
}



bool Conation::ConationStream::CheckValidSize(const ArgType Type, const uint64_t Size)
{
	switch (Type)
	{
		case ARGTYPE_INT32:
		case ARGTYPE_UINT32:
			return Size == sizeof(uint32_t);
		case ARGTYPE_INT64:
		case ARGTYPE_UINT64:
			return Size == sizeof(uint64_t);
		case ARGTYPE_BOOL:
			return Size == 1;
		default:
			return true; //Anything else has no fixed size.
	}
}

uint64_t Conation::ConationStream::DecodeSize(const bool AdjustIndex)
{
	uint64_t Size = 0;

	memcpy(&Size, Index, sizeof(Size));
	if (AdjustIndex) Index += sizeof(Size);
	return Utils::vl_ntohll(Size);
}

bool Conation::ConationStream::CheckInRange(uint64_t Size)
{

	//These are supposed to match, dammit.
	if (Bytes->size() != (GetStreamArgsSize() + STREAM_HEADER_SIZE))
	{
#ifdef DEBUG
		puts("ERROR: Bytes->size() != (GetStreamArgsSize() + STREAM_HEADER_SIZE)");
#endif //DEBUG
		throw Err_CorruptStream();
	}

	if (!Size) return true;

	return Index + Size <= &Bytes->at(Bytes->size() - 1) + 1; //Technically undefined behaviour, but you can suck my dick.
}

void Conation::ConationStream::EncodeArgType(const ArgType Type, std::vector<uint8_t> *Vector)
{

	const ArgTypeUint Encoded = htons(Type);

	for (size_t Inc = 0; Inc < sizeof(ArgTypeUint); ++Inc)
	{
		Vector->push_back(*((const uint8_t*)&Encoded + Inc));
	}
}


Conation::ArgType Conation::ConationStream::DecodeArgType(const bool AdjustIndex)
{
	ArgType Arg;

	memcpy(&Arg, Index, sizeof(ArgType));

	if (AdjustIndex) Index += sizeof(ArgType);

	return static_cast<Conation::ArgType>(ntohs(Arg));
}

uint8_t *Conation::ConationStream::AllocateBlobOnly( const uint64_t RequestedSize,
																std::vector<uint8_t> *Vector,
																const size_t ExtraSpaceAfter)
{
	if (Vector->capacity() < Vector->size() + RequestedSize)
	{
		Vector->reserve(Vector->size() + RequestedSize + ExtraSpaceAfter);
	}

	const size_t PreSize = Vector->size();
	Vector->resize(PreSize + RequestedSize);

	return &(*Vector)[PreSize];
}

void Conation::ConationStream::EncodeBlob(const void *Stream, const uint64_t NumBytes,
								std::vector<uint8_t> *Vector, const size_t ExtraSpaceAfter)
{
	if (!NumBytes) return; //There are times we will be passed an empty chunk.
	
	if (Vector->capacity() < Vector->size() + NumBytes)
	{
		Vector->reserve(Vector->size() + NumBytes + ExtraSpaceAfter);
	}

	const size_t PreSize = Vector->size();
	Vector->resize(PreSize + NumBytes);

	uint8_t *Buf = &(*Vector)[PreSize];

	memcpy(Buf, Stream, NumBytes);
}

void Conation::ConationStream::SetStreamArgsSize(const uint64_t NewSize)
{
	const uint64_t Size = Utils::vl_htonll(NewSize);

	memcpy(&(*Bytes)[1], &Size, sizeof Size);
}

void Conation::ConationStream::AutoSetStreamArgsSize(void)
{
	if (Bytes->size() < STREAM_HEADER_SIZE) throw Err_StreamNotReady(); //Not ready for calculation.

	const uint64_t Size = Utils::vl_htonll(Bytes->size() - STREAM_HEADER_SIZE);

	memcpy(&Bytes->at(1), &Size, sizeof Size);
}

uint64_t Conation::ConationStream::GetStreamArgsSize(void) const
{
	uint64_t Size = 0;

	memcpy(&Size, &Bytes->at(1), sizeof Size);

	Size = Utils::vl_ntohll(Size);

	return Size;
}

auto Conation::ConationStream::PopArgument(const bool Peek) -> BaseArg*
{
	if (Bytes->size() < STREAM_HEADER_SIZE)
	{
#ifdef DEBUG
		puts("Conation::ConationStream::PopArgument(): Bytes->size() < STREAM_HEADER_SIZE");
#endif
		return nullptr; //Empty is bad, mmkay?
	}
	uint8_t *const OriginalIndex = Index;
	if (Index == nullptr) //First time popping.
	{
		Index = &Bytes->at(STREAM_HEADER_SIZE);
	}

	if (Index == &Bytes->at(Bytes->size() - 1) + 1) //Out of arguments
	{
#ifdef DEBUG
		puts("Conation::ConationStream::PopArgument(): Out of arguments.");
#endif
		return nullptr;
	}

	const ArgType Type = DecodeArgType(true);

	BaseArg *RetVal = nullptr;

	const uint64_t Size = DecodeSize(true); //Adds sizeof(uint64_t) to Index.

	//if ((&Bytes[Bytes.size() - 1] - Index) < Size); //Fix this later

	if (!CheckValidSize(Type, Size) || !CheckInRange(Size) ||
		Bytes->size() - STREAM_HEADER_SIZE != this->GetStreamArgsSize()) //Not sure this one belongs here but fuck it.
	{
#ifdef DEBUG
		if (!CheckInRange(Size)) puts("Range bad");
		else if (!CheckValidSize(Type, Size)) puts("Size bad");
		else printf("Mismatch. Bytes->size(): %u, GetStreamArgsSize(): %u\n", (unsigned)Bytes->size() - (unsigned)STREAM_HEADER_SIZE, (unsigned)GetStreamArgsSize());
#endif //DEBUG
		Index = OriginalIndex;
		throw Err_CorruptStream();
	}

	switch (Type)
	{
		case ARGTYPE_NETCMDSTATUS:
		{
			NetCmdStatusArg *Temp = new NetCmdStatusArg;

			Temp->Code = static_cast<StatusCode>(*Index);

			char *Buf = new char[Size + 1];

			memset(Buf, 0, Size + 1);

			memcpy(Buf, Index + 1, Size - 1);

			Temp->Msg = Buf;

			delete[] Buf;

			RetVal = Temp;
			break;
		}
		case ARGTYPE_BOOL:
		{
			BoolArg *Temp = new BoolArg;

			Temp->Value = *Index;
			RetVal = Temp;
			break;
		}
		case ARGTYPE_FILE:
		{
			FileArg *Temp = new FileArg;

			const size_t FilenameLen = strlen((char*)Index);

			char *FilenameBuf = new char[FilenameLen + 1]();

			memcpy(FilenameBuf, Index, FilenameLen);

			Temp->Filename = FilenameBuf; //Runs on until a null terminator.

			delete[] FilenameBuf;

			Temp->Data = Index + FilenameLen + 1;
			Temp->DataSize = Size - FilenameLen - 1;

			RetVal = Temp;
			break;
		}
		case ARGTYPE_STRING:
		{
			StringArg *Temp = new StringArg;

			char *Buffer = new char[Size + 1];

			memcpy(Buffer, Index, Size);
			Buffer[Size] = '\0';

			Temp->String = Buffer;

			delete[] Buffer;
			RetVal = Temp;
			break;
		}
		case ARGTYPE_BINSTREAM:
		{
			BinStreamArg *Temp = new BinStreamArg;

			Temp->Data = Index;
			Temp->DataSize = Size;

			RetVal = Temp;
			break;
		}
		case ARGTYPE_SCRIPT:
		{
			ScriptArg *Temp = new ScriptArg;

			char *Buffer = new char[Size + 1];

			memcpy(Buffer, Index, Size);
			Buffer[Size] = '\0';

			Temp->String = Buffer;

			delete[] Buffer;
			RetVal = Temp;
			break;
		}
		case ARGTYPE_FILEPATH:
		{
			FilePathArg *Temp = new FilePathArg;

			char *Buffer = new char[Size + 1];

			memcpy(Buffer, Index, Size);
			Buffer[Size] = '\0';

			Temp->Path = Buffer;

			RetVal = Temp;
			delete[] Buffer;
			break;
		}
		case ARGTYPE_INT32:
		{

			Int32Arg *Temp = new Int32Arg;

			memcpy(&Temp->Value, Index, Size);
			*(uint32_t*)&Temp->Value = ntohl(Temp->Value);

			RetVal = Temp;
			break;
		}
		case ARGTYPE_UINT32:
		{

			Uint32Arg *Temp = new Uint32Arg;

			memcpy(&Temp->Value, Index, Size);
			Temp->Value = ntohl(Temp->Value);

			RetVal = Temp;
			break;
		}

		case ARGTYPE_INT64:
		{

			Int64Arg *Temp = new Int64Arg;

			memcpy(&Temp->Value, Index, Size);
			*(uint64_t*)&Temp->Value = Utils::vl_ntohll(Temp->Value);

			RetVal = Temp;
			break;
		}
		case ARGTYPE_UINT64:
		{

			Uint64Arg *Temp = new Uint64Arg;

			memcpy(&Temp->Value, Index, Size);
			Temp->Value = Utils::vl_ntohll(Temp->Value);

			RetVal = Temp;
			break;
		}
		case ARGTYPE_ODHEADER:
		{
			ODHeaderArg *Temp = new ODHeaderArg;

			const size_t OriginSize = strlen((char*)Index);
			const size_t DestinationSize = strlen((char*)Index + OriginSize + 1);

			char *Origin = new char[OriginSize + 1];
			char *Destination = new char[DestinationSize + 1];

			memcpy(Origin, Index, OriginSize + 1);
			memcpy(Destination, Index + OriginSize + 1, DestinationSize + 1);

			Temp->Hdr.Origin = Origin;
			Temp->Hdr.Destination = Destination;
			delete[] Origin;
			delete[] Destination;

			RetVal = Temp;
			break;
		}

		default:
#ifdef DEBUG
			printf("Unrecognized argument type, unsigned decimal %u\n", static_cast<unsigned>(Type));
#endif
			break;
	}

	if (!Peek) Index += Size;
	if (RetVal) RetVal->Type = Type;

#ifdef DEBUG
	else puts("Conation::ConationStream::PopArgument(): RetVal is empty!");
#endif
	return RetVal;

}

bool Conation::ConationStream::Transmit(const Net::ClientDescriptor &Descriptor, Net::NetRWStatusForEachFunc StatusFunc, void *PassAlongWith) const
{
	StreamHeader Header = this->GetHeader();

	bool RetVal{};

	try
	{
		RetVal = Net::Write(Descriptor, this->Bytes->data(), this->Bytes->size(), StatusFunc, (int64_t)PassAlongWith == -1 ? &Header : PassAlongWith);
	}
	catch (Net::Errors::Any &)
	{
		return false;
	}

	return RetVal;
}

size_t Conation::ConationStream::CountArguments(void) const
{
	size_t ArgCount = 0;

	const uint8_t *Stream = (&this->Bytes->at(STREAM_HEADER_SIZE - 1) + 1);

	for (; Stream != &this->Bytes->at(this->Bytes->size() - 1) + 1; ++ArgCount)
	{
		Stream += sizeof(ArgType);

		uint64_t Size = 0;
		memcpy(&Size, Stream, sizeof Size);
		Size = Utils::vl_ntohll(Size);

		Stream += Size;
		Stream += sizeof(uint64_t);
	}

	return ArgCount;
}

std::vector<Conation::ArgType> *Conation::ConationStream::GetArgTypes(void) const
{
	if (!this->GetArgData()) return nullptr;

	const uint8_t *Stream = (&this->Bytes->at(STREAM_HEADER_SIZE - 1) + 1);

	std::vector<ArgType> *RetVal = new std::vector<ArgType>;

	while (Stream != &this->Bytes->at(this->Bytes->size() - 1) + 1)
	{
		ArgType Type{};

		memcpy(&Type, Stream, sizeof(ArgType));

		//Get argument type
		RetVal->push_back(static_cast<ArgType>(ntohs(Type)));

		Stream += sizeof(ArgType);

		uint64_t Size = 0;
		memcpy(&Size, Stream, sizeof Size);
		Size = Utils::vl_ntohll(Size);

		Stream += sizeof(Size);
		Stream += Size;
	}

	return RetVal;
}

void Conation::ConationStream::IntegrityCheck(void) const
{
	const bool HasArgData = this->GetArgData();
	
	if (!HasArgData)
	{
		if (this->Bytes->size() < STREAM_HEADER_SIZE) throw Err_CorruptStream();

		return;
	}

	if (this->Bytes->size() < STREAM_HEADER_SIZE + sizeof(ArgType) + sizeof(uint64_t))
	{ //Stream has argument data, but it's not big enough for the first argument's header!!!
		throw Err_CorruptStream();
	}
	
	const uint8_t *Worker = (&this->Bytes->at(STREAM_HEADER_SIZE - 1) + 1);

	const uint8_t *const ArgsEnd = &this->Bytes->at(this->Bytes->size() - 1) + 1;
	
	while (Worker != ArgsEnd)
	{

		if ((uint64_t)(ArgsEnd - Worker) < sizeof(ArgType) + sizeof(uint64_t))
		{ //Another half-baked argument that's too short for its own header...
			throw Err_CorruptStream();
		}
			
		ArgType Type{};

		memcpy(&Type, Worker, sizeof(ArgType));

		Worker += sizeof(ArgType);

		uint64_t ArgSize = 0;
		memcpy(&ArgSize, Worker, sizeof ArgSize);
		ArgSize = Utils::vl_ntohll(ArgSize);

		if ((uint64_t)(ArgsEnd - (Worker + sizeof(ArgSize))) < ArgSize)
		{ //Argument flies off the end into unallocated memory.
			throw Err_CorruptStream();
		}
		
		Worker += sizeof(ArgSize);
		Worker += ArgSize;
	}
	//Guess we passed all the tests! Yay!
}

bool Conation::ConationStream::VerifyArgTypePattern(const size_t ArgOffset, const std::vector<ArgType> &List) const
{ //Checks for a repeating pattern for arguments.
	std::vector<ArgType> *Ptr = this->GetArgTypes();

	if (!Ptr || Ptr->size() < ArgOffset + 1 ||
		Ptr->size() - ArgOffset < List.size() ||
		(!List.size() && Ptr->size() - ArgOffset))
	{
		delete Ptr;
		return false;
	}

	size_t PatternInc = 0;

	for (size_t Inc = ArgOffset; Inc < Ptr->size(); ++Inc, ++PatternInc)
	{
		if (PatternInc == List.size()) PatternInc = 0;

		if (Ptr->at(Inc) != List[PatternInc])
		{
			delete Ptr;
			return false;
		}
	}

	delete Ptr;

	if (PatternInc != List.size()) return false;

	return true;
}

bool Conation::ConationStream::VerifyArgTypePattern(const size_t ArgOffset, ...) const
{ ///LISTEN: You must put ARGTYPE_NONE as the last parameter in any call to this function, or you'll get a segfault!
	va_list VaArgType;
	std::vector<ArgType> List;

	va_start(VaArgType, ArgOffset);

	ArgType Arg{};

	while ((Arg = (ArgType)va_arg(VaArgType, int)) != ARGTYPE_NONE)
	{
		List.push_back(Arg);
	}

	va_end(VaArgType);

	return this->VerifyArgTypePattern(ArgOffset, List);
}

bool Conation::ConationStream::VerifyArgTypesStartWith(const std::vector<ArgType> &List) const
{
	VLScopedPtr<std::vector<ArgType>*> Ptr { this->GetArgTypes() };

	if (!Ptr && List[0] != ARGTYPE_NONE) return false;

	if (Ptr->size() < List.size())
	{
		return false;
	}

	for (size_t Inc = 0; Inc < List.size(); ++Inc)
	{
		if (List[Inc] != Ptr->at(Inc))
		{
			return false;
		}
	}

	return true;
}

bool Conation::ConationStream::VerifyArgTypes(const std::vector<ArgType> &List) const
{
	VLScopedPtr<std::vector<ArgType>*> Ptr { this->GetArgTypes() };

	if (!Ptr && List[0] != ARGTYPE_NONE) return false;

	if (Ptr->size() != List.size())
	{
		return false;
	}


	for (size_t Inc = 0; Inc < List.size() && Inc < Ptr->size(); ++Inc)
	{
		if (List[Inc] != Ptr->at(Inc))
		{
			return false;
		}
	}

	return true;
}


bool Conation::ConationStream::VerifyArgTypes(const ArgType One, ...) const
{ ///LISTEN: You must put ARGTYPE_NONE as the last parameter in any call to this function, or you'll get a segfault!
	va_list VaArgType;
	std::vector<ArgType> List;

	if (One == ARGTYPE_NONE) return !this->GetStreamArgsSize();

	List.push_back(One);

	va_start(VaArgType, (int)One); //STFU clang...

	ArgType Arg{};

	while ((Arg = (ArgType)va_arg(VaArgType, int)) != ARGTYPE_NONE) //Promoted to int implicitly so deal with that hehe
	{
		List.push_back(Arg);
	}

	va_end(VaArgType);

	return this->VerifyArgTypes(List);
}

const uint8_t *Conation::ConationStream::GetArgData(void) const
{
	if (this->Bytes->size() == STREAM_HEADER_SIZE) return nullptr;

	return &this->Bytes->at(STREAM_HEADER_SIZE);
}

const uint8_t *Conation::ConationStream::GetSeekedArgData(void) const
{
	if (this->Bytes->size() == STREAM_HEADER_SIZE) return nullptr;

	return this->Index ? this->Index : this->Bytes->data();
}

NetCmdStatus Conation::ConationStream::Pop_NetCmdStatus(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_NETCMDSTATUS) throw Err_Misused();

	NetCmdStatusArg *Ptr = static_cast<NetCmdStatusArg*>(+Arg);

	NetCmdStatus Result(false);

	Result.WorkedAtAll = Ptr->Code == STATUS_OK || Ptr->Code == STATUS_WARN;
	Result.Status = Ptr->Code;
	Result.Msg = Ptr->Msg;

	return Result;
}

uint64_t Conation::ConationStream::Pop_Uint64(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_UINT64) throw Err_Misused();

	const uint64_t Result = Arg->ReadAs<Uint64Arg>().Value;

	return Result;
}
int64_t Conation::ConationStream::Pop_Int64(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_INT64) throw Err_Misused();

	const int64_t Result = Arg->ReadAs<Int64Arg>().Value;

	return Result;
}

uint32_t Conation::ConationStream::Pop_Uint32(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_UINT32) throw Err_Misused();

	const uint32_t Result = Arg->ReadAs<Int32Arg>().Value;

	return Result;
}

int32_t Conation::ConationStream::Pop_Int32(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_INT32) throw Err_Misused();

	const int32_t Result = Arg->ReadAs<Int32Arg>().Value;

	return Result;
}

VLString Conation::ConationStream::Pop_String(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

#ifdef DEBUG
	if (Arg->Type != ARGTYPE_STRING)
	{
		printf("Expected value %uh, got value %uh\n", ARGTYPE_STRING, Arg->Type);
	}
#endif //DEBUG
	if (!Arg || Arg->Type != ARGTYPE_STRING) throw Err_Misused();

	const VLString Result = Arg->ReadAs<StringArg>().String;

	return Result;
}

VLString Conation::ConationStream::Pop_Script(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_SCRIPT) throw Err_Misused();

	const VLString Result = Arg->ReadAs<ScriptArg>().String;

	return Result;
}

Conation::ConationStream::ODHeader Conation::ConationStream::Pop_ODHeader(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_ODHEADER) throw Err_Misused();

	ODHeader RetVal = Arg->ReadAs<ODHeaderArg>().Hdr;

	return RetVal;
}

Conation::ConationStream::BinStreamArg Conation::ConationStream::Pop_BinStream(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_BINSTREAM) throw Err_Misused();

	BinStreamArg RetVal = Arg->ReadAs<BinStreamArg>();

	return RetVal;
}

VLString Conation::ConationStream::Pop_FilePath(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_FILEPATH) throw Err_Misused();

	const VLString Result = Arg->ReadAs<FilePathArg>().Path;


	return Result;
}

bool Conation::ConationStream::Pop_Bool(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_BOOL) throw Err_Misused();

	const bool Result = Arg->ReadAs<BoolArg>().Value;

	return Result;
}

Conation::ConationStream::FileArg Conation::ConationStream::Pop_File(void)
{
	VLScopedPtr<BaseArg*> Arg { this->PopArgument() };

	if (!Arg || Arg->Type != ARGTYPE_FILE) throw Err_Misused();

	const FileArg RetVal = Arg->ReadAs<FileArg>();

	return RetVal;
}

void Conation::ConationStream::WipeArgs(void)
{
	this->Bytes->resize(STREAM_HEADER_SIZE);

	this->SetStreamArgsSize(0);
}

void Conation::ConationStream::PushArgument(Conation::ConationStream::BaseArg *Arg, const size_t ExtraSpaceAfter)
{ //Despite the elegance and symmetry of this function with PopArgument(), it wasn't written till very close to Volition's completion.
	switch (Arg->Type)
	{
		case ARGTYPE_BOOL:
			this->Push_Bool(Arg->ReadAs<BoolArg>().Value, ExtraSpaceAfter);
			break;
		case ARGTYPE_NETCMDSTATUS:
		{
			const NetCmdStatusArg &Ref = Arg->ReadAs<NetCmdStatusArg>();
			this->Push_NetCmdStatus({Ref.Code == STATUS_OK || Ref.Code == STATUS_WARN, Ref.Code, Ref.Msg});
			break;
		}
		case ARGTYPE_INT32:
			this->Push_Int32(Arg->ReadAs<Int32Arg>().Value, ExtraSpaceAfter);
			break;
		case ARGTYPE_UINT32:
			this->Push_Uint32(Arg->ReadAs<Uint32Arg>().Value, ExtraSpaceAfter);
			break;
		case ARGTYPE_INT64:
			this->Push_Int64(Arg->ReadAs<Int64Arg>().Value, ExtraSpaceAfter);
			break;
		case ARGTYPE_UINT64:
			this->Push_Uint64(Arg->ReadAs<Uint64Arg>().Value, ExtraSpaceAfter);
			break;
		case ARGTYPE_STRING:
			this->Push_String(Arg->ReadAs<StringArg>().String, ExtraSpaceAfter);
			break;
		case ARGTYPE_FILEPATH:
			this->Push_FilePath(Arg->ReadAs<FilePathArg>().Path, ExtraSpaceAfter);
			break;
		case ARGTYPE_SCRIPT:
			this->Push_Script(Arg->ReadAs<ScriptArg>().String, ExtraSpaceAfter);
			break;
		case ARGTYPE_ODHEADER:
		{
			const ODHeader &Ref = Arg->ReadAs<ODHeaderArg>().Hdr;

			this->Push_ODHeader(Ref.Origin, Ref.Destination, ExtraSpaceAfter);
			break;
		}
		case ARGTYPE_BINSTREAM:
		{
			const BinStreamArg &Ref = Arg->ReadAs<BinStreamArg>();

			this->Push_BinStream(Ref.Data, Ref.DataSize, ExtraSpaceAfter);
			break;
		}
		case ARGTYPE_FILE:
		{
			const FileArg &Ref = Arg->ReadAs<FileArg>();

			this->Push_File(Ref.Filename, Ref.Data, Ref.DataSize, ExtraSpaceAfter);
			break;
		}
		default:
			break;
	}
}

//Cuz a number of other things will find this convenient.
Conation::ConationStream::BaseArg *Conation::ConationStream::NewArgStructFromType(const ArgType Type)
{
	BaseArg *RetVal = nullptr;

	switch (Type)
	{
		case ARGTYPE_BOOL:
			RetVal = new BoolArg;
			break;
		case ARGTYPE_NETCMDSTATUS:
			RetVal = new NetCmdStatusArg;
			break;
		case ARGTYPE_STRING:
			RetVal = new StringArg;
			break;
		case ARGTYPE_SCRIPT:
			RetVal = new ScriptArg;
			break;
		case ARGTYPE_FILEPATH:
			RetVal = new FilePathArg;
			break;
		case ARGTYPE_BINSTREAM:
			RetVal = new BinStreamArg;
			break;
		case ARGTYPE_FILE:
			RetVal = new FileArg;
			break;
		case ARGTYPE_ODHEADER:
			RetVal = new ODHeaderArg;
			break;
		case ARGTYPE_INT64:
			RetVal = new Int64Arg;
			break;
		case ARGTYPE_UINT64:
			RetVal = new Uint64Arg;
			break;
		case ARGTYPE_INT32:
			RetVal = new Int32Arg;
			break;
		case ARGTYPE_UINT32:
			RetVal = new Uint32Arg;
			break;
		default:
			return nullptr;
	}

	RetVal->Type = Type;

	return RetVal;
}

Conation::ConationStream::BinStreamArg::BinStreamArg(const BinStreamArg &Ref)
{
	if (Ref.MustFree && Ref.Data != nullptr)
	{
		this->Data = new uint8_t[Ref.DataSize];
		this->DataSize = Ref.DataSize;
		memcpy(this->Data, Ref.Data, this->DataSize);

		this->MustFree = true;
		this->UseMallocFree = false;
	}
	else
	{
		this->MustFree = false;
		this->UseMallocFree = false;

		this->Data = Ref.Data;
		this->DataSize = Ref.DataSize;
	}
}

Conation::ConationStream::BinStreamArg::BinStreamArg(BinStreamArg &&Ref)
{
	this->Data = Ref.Data;
	this->DataSize = Ref.DataSize;
	this->MustFree = Ref.MustFree;
	this->UseMallocFree = Ref.UseMallocFree;

	Ref.Data = nullptr;
	Ref.DataSize = 0;
	Ref.MustFree = false;
	Ref.UseMallocFree = false;
}

Conation::ConationStream::BinStreamArg &Conation::ConationStream::BinStreamArg::operator=(const BinStreamArg &Ref)
{
	if (&Ref == this) return *this;

	if (Ref.MustFree && Ref.Data != nullptr)
	{
		this->Data = new uint8_t[Ref.DataSize];
		this->DataSize = Ref.DataSize;
		memcpy(this->Data, Ref.Data, this->DataSize);

		this->MustFree = true;
		this->UseMallocFree = false;
	}
	else
	{
		this->MustFree = false;
		this->UseMallocFree = false;

		this->Data = Ref.Data;
		this->DataSize = Ref.DataSize;
	}

	return *this;
}

Conation::ConationStream::BinStreamArg &Conation::ConationStream::BinStreamArg::operator=(BinStreamArg &&Ref)
{
	if (&Ref == this) return *this;

	this->Data = Ref.Data;
	this->DataSize = Ref.DataSize;
	this->MustFree = Ref.MustFree;
	this->UseMallocFree = Ref.UseMallocFree;

	Ref.Data = nullptr;
	Ref.DataSize = 0;
	Ref.MustFree = false;
	Ref.UseMallocFree = false;

	return *this;
}

Conation::ConationStream::BinStreamArg::~BinStreamArg(void)
{
	if (!this->MustFree || !this->Data) return;

#ifdef DEBUG
	puts("Conation::ConationStream::BinStreamArg::~BinStreamArg(): Deleting this->Data");
#endif

	if (this->UseMallocFree)
	{
		free(this->Data);
	}
	else
	{
		delete[] this->Data;
	}
}

void Conation::ConationStream::AlterHeader(const StreamHeader &Header)
{ //The edits are consistent with the layout of the binary-encoded header, btw.
	Bytes->at(0) = Header.CmdCode;

	const uint64_t StreamArgsSize = Utils::vl_htonll(Header.StreamArgsSize);
	const uint64_t CommandIdent = Utils::vl_htonll(BuildIdentComposite(Header.CmdIdentFlags, Header.CmdIdent));

	memcpy(&Bytes->at(STREAM_HEADER_SIZE - (sizeof(uint64_t) * 2)), &StreamArgsSize, sizeof StreamArgsSize);
	memcpy(&Bytes->at(STREAM_HEADER_SIZE - sizeof(uint64_t)), &CommandIdent, sizeof CommandIdent);
}

void Conation::ConationStream::AppendArgData(const Conation::ConationStream &Input)
{
	if (Input.Index == (Input.Bytes->data() + Input.Bytes->size())) return;
	
	const uint64_t InputArgsSize = (Input.Bytes->size() - STREAM_HEADER_SIZE) - ((Input.Index ? Input.Index : Input.Bytes->data())  - Input.Bytes->data());

	this->Bytes->reserve(this->Bytes->capacity() + InputArgsSize);

	const uint64_t OurOriginalSize = this->Bytes->size();
	
	this->Bytes->resize(OurOriginalSize + InputArgsSize);

	memcpy(this->Bytes->data() + OurOriginalSize, Input.Index ? Input.Index : (Input.Bytes->data() + STREAM_HEADER_SIZE), InputArgsSize);

	this->AutoSetStreamArgsSize();

	this->Rewind();

	this->IntegrityCheck();
}
