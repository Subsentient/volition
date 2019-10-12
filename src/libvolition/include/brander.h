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


#ifndef __VL_BRANDER_H__
#define __VL_BRANDER_H__

#include <time.h>
#include "common.h"

#include <map>
#include <string.h>

namespace Brander
{
	enum class AttributeTypes
	{
		INVALID = 0,
		IDENTITY,
		SERVERADDR,
		REVISION,
		PLATFORMSTRING,
		COMPILETIME,
		AUTHTOKEN,
		CERT,
		ATTR_MAX
	};
	
	class AttrValue
	{
	public:
		enum ValueType { TYPE_INT64, TYPE_STRING }; //We treat uint64 and int64 the same at this level. Make conversion seamless.
	private:
		
		ValueType Type;
		union
		{
			int64_t Int64;
			uint64_t UInt64;
			const char *String;
		};
	public:
		AttrValue(const int64_t Value) : Type(TYPE_INT64), Int64(Value) {}
		AttrValue(const uint64_t Value) : Type(TYPE_INT64), UInt64(Value) {}
		
		/* Don't change this to const char*. If you do, you will suffer.*/
		AttrValue(const VLString &Value) : Type(TYPE_STRING), String(strdup(Value)) {}
		
		virtual ~AttrValue(void) { if (this->Type == TYPE_STRING) free((void*)this->String); }
		
		inline VLString Get_String(void) const { return String; }
		inline int64_t Get_Int64(void) const { return Int64; }
		inline uint64_t Get_UInt64(void) const { return UInt64; }
		
		inline ValueType Get_ValueType(void) const { return this->Type; }
	};
	
	
	bool BrandBinaryViaBuffer(void *Buf, const size_t BufSize, const std::map<AttributeTypes, AttrValue> &Values);
	bool BrandBinaryViaFile(const char *Path, std::map<AttributeTypes, AttrValue> &Values);
	AttrValue *ReadBrandedBinaryViaBuffer(const void *Buf, const size_t BufSize, const AttributeTypes Attribute);
	AttrValue *ReadBrandedBinaryViaFile(const char *Path, const AttributeTypes Attribute);

}
#endif //__VL_BRANDER_H__
