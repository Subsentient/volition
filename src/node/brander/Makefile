include ../../Security.mk

all:
	c++ -std=gnu++14 -pedantic -Wall -g3 -O0 `$(PKGCONFIG) --cflags openssl` `$(PKGCONFIG) --libs openssl` brander.cpp ../../libvolition/brander.cpp ../../libvolition/utils.cpp ../../libvolition/vlstrings.cpp  -o brander

clean:
	rm -fv *.o brander
