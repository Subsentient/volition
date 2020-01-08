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


//From the fires of yesterday until the Hell of tomorrow
//The memory of light is all we have

#include <iostream>
#include <stdlib.h>

#define VLASSERT(Condition) if (!(Condition)) { std::cerr << \
		"Volition: CRITICAL: " << __FILE__ << "/" << __PRETTY_FUNCTION__ << " line " << \
		__LINE__ << ": Assertion failed." << std::endl; abort(); }
		
#define VLCRITICAL(Message) { (std::cerr << \
		"Volition: CRITICAL: " << __FILE__ << "/" << __PRETTY_FUNCTION__ << " line " << \
		__LINE__ << ": " << +(VLString() + Message) << std::endl); abort(); }

#define VLASSERT_ERRMSG(Condition, Message) if (!(Condition)) { std::cerr << \
		"Volition: CRITICAL: " << __FILE__ << "/" << __PRETTY_FUNCTION__ << " line " << \
		__LINE__ << ": " << +(VLString() + Message) << std::endl; abort(); }

#define VLASSERT_WARNMSG(Condition, Message) if (!(Condition)) { std::cerr << \
		"Volition: WARN: " << __FILE__ << "/" << __PRETTY_FUNCTION__ << " line " << \
		__LINE__ << ": " << +(VLString() + Message) << std::endl; }


#define VLWARN(Message) (std::cerr << "Volition: WARNING: " << __FILE__ << "/" \
		<< __PRETTY_FUNCTION__ << " line " << \
		__LINE__ << ": " << +(VLString() + Message) << std::endl)
		
#define VLERROR(Message) (std::cerr << "Volition: ERROR: " << __FILE__ << "/" \
		<< __PRETTY_FUNCTION__ << " line " << \
		__LINE__ << ": " << +(VLString() + Message) << std::endl)

#ifdef DEBUG
#define VLDEBUG(Message) (std::cout << "Volition: DEBUG: " << __FILE__ << "/" \
		<< __PRETTY_FUNCTION__ << " line " << \
		__LINE__ << ": " << +(VLString() + Message) << std::endl)
#else //not DEBUG
#define VLDEBUG(Message)
#endif //DEBUG
