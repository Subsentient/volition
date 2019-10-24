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
 * That's the sort of a question an inter-dimensional tourist would ask.
 * "Does your reality contain linear causality, or normal causality?"
 * -Imgurian meahotep
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/vlstrings.h"

VLString::VLString(const char *String)
	: BufferCapacity(String ? strlen(String) + 1 : 1), Buffer(String ? strdup(String) : (char*)calloc(1, 1)), SeekPtr(Buffer)
{
}

VLString::VLString(const std::string &Ref) : VLString((const char*)Ref.c_str())
{
}

VLString::VLString(const size_t Capacity) : BufferCapacity(Capacity ? Capacity : 1), Buffer((char*)calloc(BufferCapacity, 1)), SeekPtr(Buffer)
{
}

VLString::~VLString(void)
{
	free(this->Buffer);
}

size_t VLString::RealLength(void) const
{
	return strlen(this->Buffer);
}
size_t VLString::Length(void) const
{
	if (this->SeekPtr >= this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return strlen(this->SeekPtr);
}

VLString VLString::operator+(const char *String) const
{
	if (this->SeekPtr >= this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	VLString New(strlen(String) + this->Length() + 1);
	
	strcpy(New.GetBuffer(), +*this);
	strcat(New.GetBuffer(), String);
	
	return New;
}

VLString VLString::operator+(const std::string &Ref) const
{
	if (this->SeekPtr >= this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	VLString New(Ref.length() + this->Length() + 1);
	
	strcpy(New.GetBuffer(), this->SeekPtr);
	strcat(New.GetBuffer(), Ref.c_str());
	
	return New;
}

VLString VLString::operator+(const VLString &Ref) const
{
	if (this->SeekPtr >= this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	VLString New(Ref.Length() + this->Length() + 1);
	
	strcpy(New.GetBuffer(), this->SeekPtr);
	strcat(New.GetBuffer(), +Ref);
	
	return New;
}

VLString VLString::operator+(const char Character) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	VLString New(this->Length() + 2);
	
	strcpy(New.GetBuffer(), this->SeekPtr);
	New.GetBuffer()[this->Length()] = Character;
	
	return New;
}

VLString VLString::operator+(const size_t Indice) const
{
	if (Indice >= this->BufferCapacity - (this->SeekPtr - this->Buffer)) throw Error_InvalidAccess();
	
	return VLString(this->SeekPtr + Indice);
}

VLString VLString::IntToString(const int64_t Integer)
{
	return std::to_string(Integer);
}

VLString VLString::UintToString(const uint64_t Integer)
{
	return std::to_string(Integer);
}

int64_t VLString::StringToInt(const VLString &String)
{
	long long RetVal = strtoll(String, nullptr, 10);
	
	return RetVal;
}

uint64_t VLString::StringToUint(const VLString &String)
{
	unsigned long long RetVal = strtoull(String, nullptr, 10);
	
	return RetVal;
}

double VLString::StringToDouble(const VLString &String)
{
	return atof(String);
}

VLString VLString::DoubleToString(const double Double)
{
	char Buffer[2048]{};
	
	snprintf(Buffer, sizeof Buffer, "%g", Double);
	
	return Buffer;
}

void VLString::ChangeCapacity(const size_t NewCapacity)
{
	const size_t OldCapacity = this->BufferCapacity;
	
	//Same as the old capacity? Just return.
	if (NewCapacity == OldCapacity) return;
	const size_t Offset = this->SeekPtr - this->Buffer;
	
	//Reallocate.
	this->Buffer = (char*)realloc(this->Buffer, NewCapacity ? NewCapacity : 1);
	
	if (Offset >= (NewCapacity ? NewCapacity : 1))
	{
		this->SeekPtr = this->Buffer;
	}
	else
	{
		this->SeekPtr = this->Buffer + Offset;
	}
	
	if (NewCapacity > OldCapacity) //Growing
	{ //Zero the new space out.
		memset(this->Buffer + OldCapacity, 0, NewCapacity - OldCapacity);
	}
	else if (!NewCapacity) //We're truncating to a null string.
	{ //Don't move this block.
		*this->Buffer = '\0';
	}
	else if (NewCapacity < OldCapacity) //Shrinking
	{
		this->Buffer[NewCapacity - 1] = '\0'; //Truncate the string.
	}

	//Update capacity.
	this->BufferCapacity = NewCapacity ? NewCapacity : 1;
}

void VLString::Wipe(const bool ReallocateToOne)
{
	if (ReallocateToOne)
	{
		this->ChangeCapacity(0); //Zero is faster than one
		this->SeekPtr = this->Buffer;
	}
	
	memset(this->Buffer, 0, this->GetCapacity());
}

void VLString::Seek(const size_t Location)
{
	if (Location >= this->BufferCapacity) throw Error_InvalidAccess();

	this->SeekPtr = Buffer + Location;
}

const char &VLString::operator[](const size_t Index) const
{
	if (Index >= this->BufferCapacity - (this->SeekPtr - this->Buffer))
	{
		throw Error_InvalidAccess();
	}

	return this->SeekPtr[Index];
}

char &VLString::operator[](const size_t Index)
{
	if (Index >= this->BufferCapacity - (this->SeekPtr - this->Buffer))
	{
		throw Error_InvalidAccess();
	}

	return const_cast<char&>(this->SeekPtr[Index]);
}

VLString &VLString::operator+=(const VLString &Ref)
{
	const size_t RealLength = this->RealLength();
	
	if (this->BufferCapacity < RealLength + Ref.Length() + 1)
	{
		this->ChangeCapacity(RealLength + Ref.Length() + 1);
	}

	strcat(this->Buffer, Ref.SeekPtr);

	return *this;
}
VLString &VLString::operator+=(const std::string &Ref)
{
	const size_t RealLength = this->RealLength();

	if (this->BufferCapacity < RealLength + Ref.length() + 1)
	{
		this->ChangeCapacity(RealLength + Ref.length() + 1);
	}

	strcat(this->Buffer, Ref.c_str());

	return *this;
}
VLString &VLString::operator+=(const char Character)
{
	const size_t RealLength = this->RealLength();
	if (this->BufferCapacity < RealLength + sizeof Character + 1)
	{
		this->ChangeCapacity(RealLength + sizeof Character + 1);
	}

	this->Buffer[RealLength] = Character;
	this->Buffer[RealLength + 1] = '\0';
	
	return *this;
}

VLString &VLString::operator+=(const char *String)
{
	const size_t Len = strlen(String);
	const size_t RealLength = this->RealLength();
	
	if (this->BufferCapacity < RealLength + Len + 1)
	{
		this->ChangeCapacity(RealLength + Len + 1);
	}

	strcat(this->Buffer, String);

	return *this;
}

VLString &VLString::operator+=(const size_t Uint)
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity && Uint) throw Error_InvalidAccess();
	
	if (this->SeekPtr + Uint >= this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	this->SeekPtr += Uint;
	
	return *this;
}

VLString &VLString::operator-=(const size_t Val)
{
	if (Val > (size_t)(this->SeekPtr - this->Buffer)) throw Error_InvalidAccess();
	
	this->SeekPtr -= Val;
	
	return *this;
}
	
	
VLString &VLString::operator=(const char *NewString)
{
	const size_t NewStringLength = strlen(NewString);

	if (NewStringLength + 1 > this->BufferCapacity)
	{
		this->ChangeCapacity(NewStringLength + 1);
	}

	this->Wipe(false);

	strcpy(this->Buffer, NewString);

	return *this;
}

VLString &VLString::operator=(const std::string &Ref)
{
	const size_t NewStringLength = Ref.length();

	if (NewStringLength + 1 > this->BufferCapacity)
	{
		this->ChangeCapacity(NewStringLength + 1);
	}

	this->Wipe(false);

	strcpy(this->Buffer, Ref.c_str());

	return *this;
}

VLString &VLString::operator=(const VLString &Input)
{ //Copy assignment operator
	if (&Input == this) return *this; //You've gotta be pretty retarded to do this.

	this->Wipe(false);

	if (Input.RealLength() + 1 > this->BufferCapacity)
	{
		this->ChangeCapacity(Input.RealLength() + 1);
	}

	memcpy(this->Buffer, Input.Buffer, Input.RealLength() + 1);
	this->SeekPtr = this->Buffer + Input.GetPosition();
	
	return *this;
}

VLString &VLString::operator=(VLString &&Input)
{ //Move assignment operator
	free(this->Buffer);

	this->Buffer = Input.Buffer;
	this->SeekPtr = Input.SeekPtr; //We can safely steal this.
	this->BufferCapacity = Input.BufferCapacity;

	Input.Buffer = nullptr;
	Input.SeekPtr = nullptr;

	return *this;
}

bool VLString::operator==(const std::string &Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return !strcmp(Text.c_str(), this->SeekPtr);
}

bool VLString::operator==(const char *Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return !strcmp(Text, this->SeekPtr);
}

bool VLString::operator==(const VLString &Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return !strcmp(Text.SeekPtr, this->SeekPtr);
}
bool VLString::operator!=(const std::string &Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return strcmp(Text.c_str(), this->SeekPtr);
}

bool VLString::operator!=(const char *Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return strcmp(Text, this->SeekPtr);
}

bool VLString::operator!=(const VLString &Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return strcmp(Text.SeekPtr, this->SeekPtr);
}

VLString::VLString(const VLString &Ref) : BufferCapacity(Ref.GetCapacity()), Buffer(strdup(Ref.Buffer)), SeekPtr(Buffer + Ref.GetPosition())
{
}

VLString::VLString(VLString &&Ref) : BufferCapacity(Ref.BufferCapacity), Buffer(Ref.Buffer), SeekPtr(Ref.SeekPtr)
{
	Ref.Buffer = nullptr;
	Ref.SeekPtr = nullptr;
}

bool VLString::operator<(const VLString &Ref) const
{
	return strcmp(Ref.SeekPtr, this->SeekPtr) < 0;
}

char *VLString::operator++(void)
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return const_cast<char*>(++this->SeekPtr);
}
char *VLString::operator++(int)
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();

	return const_cast<char*>(this->SeekPtr++);
}

char *VLString::operator--(void)
{
	if (this->SeekPtr == this->Buffer) throw Error_InvalidAccess();
	
	return const_cast<char*>(--this->SeekPtr);
}
char *VLString::operator--(int)
{
	if (this->SeekPtr == this->Buffer) throw Error_InvalidAccess();

	return const_cast<char*>(this->SeekPtr--);
}

const char *VLString::Find(const char *Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return strstr(this->SeekPtr, Text);
}

const char *VLString::FindCharacter(const char Character) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return strchr(this->SeekPtr, Character);
}

bool VLString::Contains(const char *Text) const
{
	if (this->SeekPtr == this->Buffer + this->BufferCapacity) throw Error_InvalidAccess();
	
	return !strncmp(this->SeekPtr, Text, strlen(Text));
}

size_t VLString::GetPosition(void) const
{
	return this->SeekPtr - this->Buffer;
}

void VLString::ShrinkToFit(void)
{
	this->ChangeCapacity(this->RealLength() + 1);
}

bool VLString::StartsWith(const char *Text) const
{
	return !strncmp(this->SeekPtr, Text, strlen(Text));
}

bool VLString::EndsWith(const char *Text) const
{
	const size_t TextLength = strlen(Text);
	const size_t OurLength = this->Length();
	
	if (TextLength > OurLength) return false;
	
	const char *const Worker = (this->SeekPtr + (OurLength - 1)) - (TextLength - 1);
	
	return !strcmp(Worker, Text);
}


void VLString::StripLeading(const char *Tokens)
{
	if (!*this->SeekPtr) return;
	
	const char *Worker = this->SeekPtr;

	const char *NewTarget = this->SeekPtr;
	
	for (; *Worker; ++Worker)
	{
		const char *TokenSeek = Tokens;
		bool Found = false;
		for (; *TokenSeek; ++TokenSeek)
		{
			if (*TokenSeek == *Worker)
			{
				NewTarget = Worker + 1;
				Found = true;
			}
		}

		if (!Found) break;
	}

	memmove(const_cast<char*>(this->SeekPtr), NewTarget, strlen(NewTarget) + 1);
}

void VLString::StripTrailing(const char *Tokens)
{
	if (!*this->SeekPtr) return;

	//Signed for a reason
	for (ssize_t Dec = this->Length() - 1; Dec > 0; --Dec)
	{
		bool Found = false;
		const char *TokenSeek = Tokens;
		for (; *TokenSeek; ++TokenSeek)
		{
			if (*TokenSeek == this->SeekPtr[Dec])
			{
				Found = true;
				const_cast<char*>(this->SeekPtr)[Dec] = '\0';
			}
	
		}
		if (!Found) break;
	}
}
