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


#ifndef _VL_UTILS_H_
#define _VL_UTILS_H_

#include "common.h"

#include <stdint.h>
#include <vector>
#include <stddef.h>

namespace Utils
{
	bool StringAllNumeric(const char *String);
	std::vector<uint8_t> *Slurp(const char *FilePath, const bool Binary = false);
	bool Slurp(const char *FilePath, void *OutBuffer, const size_t OutCapacity);
	bool WriteFile(const char *OutPath, const void *Buffer, const size_t FileSize);
	uint64_t vl_htonll(const uint64_t Original);
	uint64_t vl_ntohll(const uint64_t Original);
	void vl_sleep(uint64_t Milliseconds);
	bool FileExists(const char *Path);
	bool GetFileSize(const char *Path, uint64_t *Output);
	VLString StripPathFromFilename(const char *FilePath);
	std::vector<VLString> *SplitTextByCharacter(const char *Text, const char Character);
	std::vector<VLString> *SplitTextBySubstring(const char *Text, const char *Substring);
	
	VLString JoinTextByCharacter(const std::vector<VLString> *Strings, const char Character);
	VLString JoinTextBySubstring(const std::vector<VLString> *Strings, const char *Substring);
	
	VLString GetTempDirectory(void);
	VLString GetHomeDirectory(void);
	bool IsDirectory(const char *Path);
	VLString GetSelfBinaryPath(void);
	
	VLString GetSha512(const void *Buffer, const uint64_t Length);
	inline VLString GetSha512(const VLString &String) { return GetSha512(+String, String.Length()); }
	
	template <typename I>
	VLString ToBinaryString(const I Value)
	{
		const size_t NumBits = sizeof(I) * 8;
		VLString Text(NumBits + 1);
		
		for (size_t Inc = 0; Inc < NumBits; ++Inc)
		{
			const bool Present = (Value >> Inc) & 1;
			
			Text[(NumBits - Inc) - 1] = Present ? '1' : '0';
		}
		
		return VLString("0b") + Text;
	}
}
#endif //_VL_UTILS_H_
