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


#ifndef _VL_VLSTRINGS_H_
#define _VL_VLSTRINGS_H_

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string> //For compatibility.


/** VLStrings have a number of advantages. They are meant to combine the best features
 * of C strings and C++ strings, while keeping the ability to mess with the raw buffer
 * if desired. They are also designed to emulate the usefulness of incrementing/decrementing a char* pointer.
 * 
 * It is perfectly safe to directly edit the contents of the raw buffer.
 * Just make sure you null terminate!
 **/
 
struct VLStringPODCheat
{
	size_t BufferCapacity;
	char *Buffer;
	const char *SeekPtr;
};

class VLString
{ //Our super powerful, super awesome string class. ^^
private:
	size_t BufferCapacity;
	char *Buffer;
	const char *SeekPtr;
	
	///This class must stay a standard-layout class!!!
	///Not being trivial is fine. So, no virtuals or base classes etc.
public:

	class Error_Base
	{
	};

	class Error_InvalidAccess : public Error_Base
	{
	};
	
	//Conversion functions.
	operator bool			(void) const { return *this->SeekPtr != '\0'; }
	operator const char *	(void) const { return this->SeekPtr; }
	operator std::string	(void) const { return std::string(this->SeekPtr); }

	//Operators for element access
	
	//Useful for e.g. in printf() calls. E.g. printf("%s", +MyString);
	const char *operator+	(void) const { return this->SeekPtr; }
	
	//Return a new substring of what we're currently pointed at.
	VLString 	operator~	(void) const { return VLString(this->SeekPtr); }
	
	const char &operator*	(void) const { return *this->SeekPtr; }
	char &operator*			(void)		 { return *const_cast<char*>(this->SeekPtr); }
	
	//Addition operators.
	
	///Adding string types
	VLString operator+	(const VLString &Ref)	const;
	VLString operator+	(const std::string &Ref)const;
	VLString operator+	(const char *In) 		const;
	
	///Adding characters
	VLString operator+	(const char Character) 	const;
	
	
	///Adding integer types
	VLString operator+	(const int Indice) const { return this->operator+((size_t)Indice); }
#if SIZE_MAX != UINT64_MAX
	VLString operator+	(const uint64_t Indice) const { return this->operator+((size_t)Indice); }
#endif
	VLString operator+	(const size_t Indice) const;
	
	//Integer manipulation
	static VLString IntToString(const int64_t Integer);
	static VLString UintToString(const uint64_t Integer);
	static int64_t StringToInt(const VLString &String);
	static uint64_t StringToUint(const VLString &String);
	
	//Floating point manipulation
	static double StringToDouble(const VLString &String);
	static VLString DoubleToString(const double Double);
	
	//Constructors
	VLString(const char *Stringy);
	VLString(const std::string &Stringy);
	VLString(const size_t Size = 0);
	
	//Copy/move constructors
	VLString(const VLString &Ref);
	VLString(VLString &&Ref);
	
	//Destructor
	///DO NOT MAKE THIS VIRTUAL!!! We can't have virtual functions, otherwise the memory layout won't match VLStringPODCheat!
	~VLString(void);
	
	//Assignment operators
	VLString &operator=(const char *String);
	VLString &operator=(const std::string &Ref);
	///Copy/move assignment operators.
	VLString &operator=(const VLString &Ref);
	VLString &operator=(VLString &&Ref);

	//Addition-assignment operators
	VLString &operator+=(const char *String);
	VLString &operator+=(const VLString &Ref);
	VLString &operator+=(const std::string &Ref);
	VLString &operator+=(const char Character);

	///Not like the others. These are used for pointer arithmetic.
	///There's multiple so we don't get an ambiguous overload for the char.
	VLString &operator+=(const size_t Val);
	VLString &operator+=(const int Val) { return (*this) += (size_t)Val; }
#if SIZE_MAX != UINT64_MAX
	VLString &operator+=(const uint64_t Val) { return (*this) += (size_t)Val; }
#endif
	VLString &operator-=(const size_t Val);
	
	//Equality operators
	bool operator==(const char *Text) const;
	bool operator==(const VLString &Text) const;
	bool operator==(const std::string &Text) const;
	bool operator!=(const char *Text) const;
	bool operator!=(const VLString &Text) const;
	bool operator!=(const std::string &Text) const;

	//Comparison operator, needed for some STL stuff. It's basically a shitty hack.
	bool operator<(const VLString &Ref) const;
	
	//Subscript operators
	char &operator[](const size_t Index);
	const char &operator[](const size_t Index) const;
	
	const char *operator++(int);
	const char *operator++(void);
	const char *operator--(int);
	const char *operator--(void);
	
	bool Empty(void) const { return !this->Buffer[0]; }
	bool empty(void) const { return this->Empty(); }
	
	size_t Length(void) const;
	size_t RealLength(void) const;
	size_t length(void) const { return this->Length(); }
	size_t size(void) const { return this->Length(); }
	
	void ChangeCapacity(const size_t NewCapacity);
	void reserve(const size_t NewCapacity) { this->ChangeCapacity(NewCapacity); }
	
	void ShrinkToFit(void);
	
	void Seek(const size_t);
	void Rewind(void) { this->SeekPtr = this->Buffer; }
	size_t GetPosition(void) const;
	
	//There is no 'resize' function because we'd be lying about the actual length of the string.
	
	size_t GetCapacity(void) const { return this->BufferCapacity; }
	size_t capacity(void) const { return this->GetCapacity(); }
	
	void Wipe(const bool ReallocateToOne);
	void clear(void) { this->Wipe(true); }
	
	char *GetBuffer(void) { return this->Buffer; }
	const char *GetBuffer(void) const { return this->Buffer; }
	
	//Find/replace/etc functions
	const char *Find(const char *Text) const;
	const char *FindCharacter(const char Character) const;
	bool Contains(const char *Text) const;
	bool StartsWith(const char *Text) const;
	bool EndsWith(const char *Text) const;
	void StripLeading(const char *Tokens);
	void StripTrailing(const char *Tokens);
	
	VLStringPODCheat ToPOD(void)
	{ //So we can pass it through variable length functions (e.g. stdarg.h stuff)
		VLStringPODCheat RetVal = *reinterpret_cast<VLStringPODCheat*>(this);
		return RetVal;
	}
	
	const VLStringPODCheat ToPOD(void) const
	{ //So we can pass it through variable length functions (e.g. stdarg.h stuff)
		const VLStringPODCheat RetVal = *reinterpret_cast<const VLStringPODCheat*>(this);
		return RetVal;
	}
	
};

static_assert(sizeof(VLString) == sizeof(VLStringPODCheat), "VLStringPODCheat is not the same size as VLString. Did you make VLString non-standard-layout?");

#endif //_VL_VLSTRINGS_H_
