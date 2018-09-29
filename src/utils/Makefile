#include ../Security.mk

all: authtokenutil
	
authtokenutil:
	c++ $(CXXFLAGS) -std=gnu++14 -pedantic -Wall -g3 -O0 genauthtoken.cpp -o genauthtoken
clean:
	rm -fv *.o *.gch *.exe genauthtoken
