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
#include "../libvolition/include/vlstrings.h" //Eat my nipple pus

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAX_TOKEN_LENGTH 128
#define MIN_TOKEN_LENGTH 32

int main(const int argc, const char **argv)
{
	srand(time(nullptr) ^ clock());
	if (argc > 2)
	{
		fputs("Takes one or no arguments.\n", stderr);
		return 1;
	}
	
	VLString Buffer(1024);
	
	if (argc == 2) Buffer = VLString(argv[1]) + '_';
	
	int NumCharsRand = 0;
	
	while ((NumCharsRand = (uint8_t)rand()) > MAX_TOKEN_LENGTH || NumCharsRand < MIN_TOKEN_LENGTH);
	
	for (int Inc = 0; Inc < NumCharsRand; ++Inc)
	{
	Retry:
		;
		uint8_t Byte = (uint8_t)rand();
		
		//Force to be 7 bit.
		Byte <<= 1;
		Byte >>= 1;
		
		if (Byte < '0' || Byte > 'z' || !isalnum(Byte)) goto Retry;
		
		Buffer += (char)Byte; //Cast is important! Otherwise VLString thinks it's pointer arithmetic.
	}
	
	puts(Buffer);
	
	return 0;
	
}
