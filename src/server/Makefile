CXXFLAGS = -std=gnu++14 -pedantic -Wall -fno-rtti -fno-strict-aliasing -Wno-strict-aliasing \
			-Wno-ignored-attributes -Wno-implicit-fallthrough `$(PKGCONFIG) --cflags openssl` `$(PKGCONFIG) --cflags sqlite3`
OUTFILE = ./volition_server
OBJECTS = core.o clients.o cmdhandling.o db.o nodeupdates.o logger.o routines.o
LDFLAGS = ./.libvl/libvolition.a `$(PKGCONFIG) --libs sqlite3` `$(PKGCONFIG) --libs openssl`

#Include security CXXFLAG and LDFLAG options.
include ../Security.mk

EXTRA_LDFLAGS =

ifdef DEBUG
	CXXFLAGS += -DDEBUG -g3 -pg -Og
	LDFLAGS += -pg
else
	CXXFLAGS += -O3
endif

ifdef COREFREQ
	CXXFLAGS += -DSERVER_CORE_FREQUENCY_MS=$(COREFREQ)
endif

ifdef EXTRACXX
	CXXFLAGS += $(EXTRACXX)
endif

ifdef EXTRALDFLAGS
	LDFLAGS += $(EXTRALDFLAGS)
endif

ifeq ($(ASAN),1)
	CXXFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address -static-libasan
endif

ifeq ($(OS),windows)
	LDFLAGS += -lws2_32 -static
	OUTFILE = ./volition_server.exe
	OBJECTS += ./win32/win.o
else ifeq ($(OS),haikuos)
	LDFLAGS += -lnetwork -pthread
else
	LDFLAGS += -pthread 
endif

	
all: corecpp clientscpp dbcpp cmdhandlingcpp nodeupdatescpp routinescpp loggercpp
	mkdir -p .libvl
	$(MAKE) -C ../libvolition OBJ_OUT_DIR=`pwd`/.libvl
ifeq ($(OS),windows)
	$(MAKE) -C ./win32
endif
	$(CXX) $(OBJECTS) -o $(OUTFILE) $(LDFLAGS) $(EXTRA_LDFLAGS)


corecpp:
	$(CXX) core.cpp -c $(CXXFLAGS)
routinescpp:
	$(CXX) routines.cpp -c $(CXXFLAGS)
dbcpp:
	$(CXX) db.cpp -c $(CXXFLAGS)
loggercpp:
	$(CXX) logger.cpp -c $(CXXFLAGS)
clientscpp:
	$(CXX) clients.cpp -c $(CXXFLAGS)
nodeupdatescpp:
	$(CXX) nodeupdates.cpp -c $(CXXFLAGS)
cmdhandlingcpp:
	$(CXX) cmdhandling.cpp -c $(CXXFLAGS)

clean:
	rm -f *.o *.gch volition_server*
	$(MAKE) -C win32 clean
