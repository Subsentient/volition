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



#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <memory>
#ifdef WIN32
#include <windows.h>
#define VL_HOME_ENV "USERPROFILE"
#else
#include <unistd.h>
#define VL_HOME_ENV "HOME"
#endif //WIN32

#if defined(LINUX) || defined(NETBSD)
#include <fcntl.h>
#elif defined(FREEBSD)
#include <sys/sysctl.h>
#endif

#include <openssl/sha.h>

#include "include/utils.h"
#include "include/common.h"

static inline uint64_t Swap64bit(const uint64_t Original);

VLString Utils::GetSha512(const void *Buffer, const uint64_t Length)
{
    SHA512_CTX Context{};

	uint8_t BinBuf[SHA512_DIGEST_LENGTH + 1] {};

    SHA512_Init(&Context);
    SHA512_Update(&Context, Buffer, Length);
    SHA512_Final(BinBuf, &Context);

	VLString RetVal((sizeof BinBuf * 2) + 1);

	uint8_t *Worker = BinBuf, *const Stopper = BinBuf + SHA512_DIGEST_LENGTH;

	char Tmp[32]{};

	for (; Worker != Stopper; ++Worker)
	{
		snprintf(Tmp, sizeof Tmp, "%02x", *Worker);

		RetVal += (const char*)Tmp;
	}

	VLDEBUG("Returning SHA512 " + RetVal);
	return RetVal;
}

VLString Utils::GetFileSha512(const VLString &Path)
{
	SHA512_CTX Context{};

	uint8_t BinBuf[SHA512_DIGEST_LENGTH + 1] {};

	SHA512_Init(&Context);

	VLScopedPtr<FILE*, int(*)(FILE*)> Desc { fopen(Path, "rb"), fclose };

	if (!Desc)
	{
		VLWARN("Failed to open path " + Path);
		return {};
	}

	uint64_t Size = 0;

	if (!Utils::GetFileSize(Path, &Size))
	{
		VLWARN("Failed to get file size of path " + Path);
		return {};
	}

	const uint64_t ChunkSize = (1024 * 1024) * 32;

	std::vector<uint8_t> Buf;

	Buf.resize(ChunkSize);

	uint64_t TotalRead = 0;

	do
	{
		const uint64_t ActuallyRead = fread(Buf.data(), 1, ChunkSize, Desc);

		if (ActuallyRead <= 0)
		{
			VLWARN("Failed to read data for path " + Path);
			return {};
		}

		SHA512_Update(&Context, Buf.data(), ActuallyRead);

		TotalRead += ActuallyRead;
	} while (Size > TotalRead);

	SHA512_Final(BinBuf, &Context);

	VLString RetVal((sizeof BinBuf * 2) + 1);

	uint8_t *Worker = BinBuf, *const Stopper = BinBuf + SHA512_DIGEST_LENGTH;

	char Tmp[32]{};

	for (; Worker != Stopper; ++Worker)
	{
		snprintf(Tmp, sizeof Tmp, "%02x", *Worker);

		RetVal += (const char*)Tmp;
	}

	VLDEBUG("Returning SHA512 " + RetVal);
	return RetVal;
}

bool Utils::StringAllNumeric(const char *String)
{
	if (!*String) return false; //Empty

	const char *Worker = String;

	for (; *Worker; ++Worker)
	{
		if (*Worker < '0' || *Worker > '9') return false;
	}

	return true;
}

bool Utils::Slurp(const char *FilePath, void *OutBuffer, const size_t OutCapacity)
{ //Null terminates, so a size of 1 is actually empty.
	struct stat FileStat;

	if (stat(FilePath, &FileStat) != 0)
	{
		return false;
	}

	if ((size_t)FileStat.st_size > OutCapacity)
	{ //Not enough space.
		return false;
	}

	VLScopedPtr<FILE*, int(*)(FILE*)> Desc { fopen(FilePath, "rb"), fclose };

	if (!Desc)
	{
		return false;
	}

	uint8_t *Worker = static_cast<uint8_t*>(OutBuffer);

	if (FileStat.st_size == 0) //If zero size, good chance it's in /proc or something, so keep reading till we get EOF.
	{
		int Char = 0;

		size_t Inc = 0u;

		for(; Inc < OutCapacity && (Char = getc(Desc)) != EOF; ++Inc)
		{
			Worker[Inc] = Char;
		}

		if (Inc < OutCapacity) Worker[Inc] = '\0'; //Null terminate in case it's a text file.
	}
	else
	{

		fread(Worker, 1, FileStat.st_size, Desc);

		if (OutCapacity - FileStat.st_size > 0) Worker[FileStat.st_size] = '\0';
	}

	return true;
}


std::vector<uint8_t> *Utils::Slurp(const char *FilePath, const bool Binary)
{ //Null terminates, so a size of 1 is actually empty.
	struct stat FileStat;

	if (stat(FilePath, &FileStat) != 0)
	{
		return nullptr;
	}

	FILE *Desc = fopen(FilePath, "rb");

	if (!Desc)
	{
		return nullptr;
	}

	std::vector<uint8_t> *RetVal = new std::vector<uint8_t>;

	RetVal->reserve(FileStat.st_size ? FileStat.st_size + 1 : 4096);

	if (FileStat.st_size == 0) //If zero size, good chance it's in /proc or something, so keep reading till we get EOF.
	{
		int Char = 0;

		while ((Char = getc(Desc)) != EOF)
		{
			RetVal->push_back(Char);
		}
		fclose(Desc);

		if (!Binary) RetVal->push_back('\0'); //Null terminate in case it's a text file.
	}
	else
	{
		RetVal->resize(FileStat.st_size + 1); //Make it fit.

		fread(&RetVal->at(0), 1, FileStat.st_size, Desc);
		fclose(Desc);

		if (!Binary) RetVal->at(FileStat.st_size) = '\0'; //Null terminate.
	}

	RetVal->shrink_to_fit();

	return RetVal;
}

static constexpr bool ArchIsBigEndian(void)
{
	const uint16_t Test = 1;
	return *(const uint8_t*)&Test == 0;
}

uint64_t Utils::vl_htonll(const uint64_t Original)
{
	return ArchIsBigEndian() ? Original : Swap64bit(Original); //Already big endian
}

uint64_t Utils::vl_ntohll(const uint64_t Original)
{
	return ArchIsBigEndian() ? Original : Swap64bit(Original);
}

static inline uint64_t Swap64bit(const uint64_t Original)
{
	uint64_t Edited = 0;
	const uint8_t *Ptr1 = reinterpret_cast<const uint8_t*>(&Original);
	uint8_t *Ptr2 = reinterpret_cast<uint8_t*>(&Edited) + sizeof Edited - 1;;

	for (; Ptr1 != (const uint8_t*)&Original + sizeof Original; ++Ptr1, --Ptr2)
	{
		*Ptr2 = *Ptr1;
	}

	return Edited;
}

void Utils::vl_sleep(uint64_t Milliseconds)
{
#ifdef WIN32
	Sleep(Milliseconds);
#else
	usleep(Milliseconds * 1000);
#endif //WIN32
}

bool Utils::WriteFile(const char *OutPath, const void *Buffer, const size_t FileSize)
{
	VLScopedPtr<FILE*, int(*)(FILE*)> Descriptor { fopen(OutPath, "wb"), fclose };

	if (!Descriptor) return false;

	if (Buffer && FileSize) fwrite(Buffer, 1, FileSize, Descriptor);

	return true;
}

bool Utils::FileExists(const char *Path)
{
	struct stat FileStat{};

	return !stat(Path, &FileStat);
}

bool Utils::GetFileSize(const char *Path, uint64_t *Output)
{
	struct stat FileStat{};

	if (stat(Path, &FileStat) != 0) return false;

	*Output = FileStat.st_size;

	return true;
}

VLString Utils::StripPathFromFilename(const char *FilePath)
{ //Make sure the file exists. We don't check for that at all.
	VLString Buffer = FilePath;

	if (Buffer[Buffer.size() - 1] == PATH_DIVIDER
#ifdef WIN32
		|| Buffer[Buffer.size() - 1] == '/'
#endif
	)
	{
		Buffer[Buffer.size() - 1] = '\0';
	}

	const char *Worker = Buffer;
	const char *Stopper = +Buffer + Buffer.size();
	const char *Seek = Buffer;

	for (; Worker != Stopper; ++Worker)
	{
		if (*Worker == PATH_DIVIDER
#ifdef WIN32
			|| *Worker == '/'
#endif
			)
		{
			Seek = Worker + 1;
		}
	}

	return Seek;
}


std::vector<VLString> *Utils::SplitTextByCharacter(const char *Text, const char Character)
{
	std::vector<VLString> *RetVal = new std::vector<VLString>;

	if (!*Text) return RetVal; //Give us the empty vector we asked for, like the retards we are.

	VLScopedPtr<char *> Buffer = new char[strlen(Text) + 1];

	const char *Worker = Text;

	do
	{
		if (*Worker == Character)
		{
			while (*Worker == Character) ++Worker;

			if (*Worker == '\0') break; //Don't try and store the empty line if we have trailing triggers.
		}

		size_t Inc = 0;
		for (; Worker[Inc] != Character && Worker[Inc] != '\0'; ++Inc)
		{
			Buffer[Inc] = Worker[Inc];
		}
		Buffer[Inc] = 0;

		RetVal->push_back((const char*)Buffer);
	} while ((Worker = strchr(Worker, Character)));

	return RetVal;

}

std::vector<VLString> *Utils::SplitTextBySubstring(const char *Text, const char *Substring)
{
	std::vector<VLString> *RetVal = new std::vector<VLString>;

	if (!*Text) return RetVal; //Give us the empty vector we asked for, like the retards we are.

	VLString Buffer(strlen(Text) + 1);

	const char *Worker = Text;

	const uint32_t SSLen = strlen(Substring);

	do
	{
		if (!strncmp(Worker, Substring, SSLen))
		{
			Worker += SSLen;
			if (*Worker == '\0') break; //Don't try and store the empty line if we have trailing triggers.
		}

		size_t Inc = 0;
		for (; strncmp(Worker + Inc, Substring, SSLen) != 0 && Worker[Inc] != '\0'; ++Inc)
		{
			Buffer[Inc] = Worker[Inc];
		}

		Buffer[Inc] = 0;

		RetVal->push_back((const char*)Buffer);
	} while ((Worker = strstr(Worker, Substring)));

	return RetVal;
}

VLString Utils::JoinTextByCharacter(const std::vector<VLString> *Strings, const char Character)
{
	VLString RetVal(8192);

	for (size_t Inc = 0; Inc < Strings->size(); ++Inc)
	{
		RetVal += Strings->at(Inc);

		if (Inc + 1 < Strings->size())
		{
			RetVal += Character;
		}
	}

	return RetVal;
}

VLString Utils::JoinTextBySubstring(const std::vector<VLString> *Strings, const char *Substring)
{
	VLString RetVal(8192);

	for (size_t Inc = 0; Inc < Strings->size(); ++Inc)
	{
		RetVal += Strings->at(Inc);

		if (Inc + 1 < Strings->size())
		{
			RetVal += Substring;
		}
	}

	return RetVal;
}

VLString Utils::GetHomeDirectory(void)
{
	return getenv(VL_HOME_ENV);
}

VLString Utils::GetTempDirectory(void)
{
	VLString NewPath(2048);
#ifdef WIN32

	GetTempPath(NewPath.GetCapacity(), NewPath.GetBuffer());
#elif defined(LINUX) && defined(ANDROID) //we need to be running as root to use this usually. Some android versions it doesn't even exist on. T_T
	//It really just seems like there's no global /tmp-like directory for android...
	NewPath = "/data/local/tmp/";
#else
	NewPath = "/tmp/";
#endif //WIN32

	NewPath.ShrinkToFit();

	return NewPath;
}

bool Utils::IsDirectory(const char *Path)
{
	struct stat FileStat{};

	if (stat(Path, &FileStat) != 0) return false;

	return S_ISDIR(FileStat.st_mode);
}

VLString Utils::GetSelfBinaryPath(void)
{
	char Buffer[2048]{};
#ifdef LINUX

	if (readlink("/proc/self/exe", Buffer, sizeof Buffer - 1) == -1) return {};
#elif defined(NETBSD)
	if (readlink("/proc/curproc/exe", Buffer, sizeof Buffer - 1) == -1) return {};
#elif defined(FREEBSD)
	int CallArgs[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };

	size_t Size = sizeof Buffer - 1;

	sysctl(CallArgs, sizeof CallArgs / sizeof *CallArgs, Buffer, &Size, nullptr, 0);
#elif defined(WIN32)
	GetModuleFileName(nullptr, Buffer, sizeof Buffer - 1);
#endif
	return Buffer;

}
