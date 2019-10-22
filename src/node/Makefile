PKGCONFIG = pkg-config
CXXFLAGS = -std=gnu++14 -pedantic -Wall -fno-rtti -fno-strict-aliasing -Wno-strict-aliasing\
			`$(PKGCONFIG) --cflags lua` `$(PKGCONFIG) --cflags openssl`
OBJECTS = processes.o interface.o main.o files.o cmdhandling.o jobs.o updates.o identity_module.o script.o
LDFLAGS = `$(PKGCONFIG) --libs openssl` `$(PKGCONFIG) --libs lua` ./.libvl/libvolition.a
IDENTITY = ""
AUTHTOKEN = ""
OUTFILE = ./volition_node_$(IDENTITY)-$(OS).$(CPU)
REVISION = $(shell git rev-parse --short HEAD)
EXTRA_LDFLAGS =
STRIP = strip

#CXXFLAG and LDFLAG appends to harden the binary.
include ../Security.mk

ifeq ($(OS),windows)
	LDFLAGS += -lws2_32 -ldl -lpsapi -static
ifneq ($(WINRC),0)
	LDFLAGS +=  ./win32/win.o
endif
	OUTFILE = ./volition_node_$(IDENTITY)-$(OS).$(CPU).exe
	
ifneq ($(MWINDOWS),0)
	LDFLAGS += -mwindows
endif

PLATFORM_CXXFLAGS =

else ifeq ($(OS),linux)
	PLATFORM_CXXFLAGS += -DLINUX -rdynamic
	LDFLAGS += -lpthread -rdynamic
else ifeq ($(OS),android)
	PLATFORM_CXXFLAGS += -DANDROID -DLINUX
	LDFLAGS += -lpthread
else ifeq ($(OS),openbsd)
	PLATFORM_CXXFLAGS += -DOPENBSD
	LDFLAGS += -lpthread
else ifeq ($(OS),freebsd) 
	PLATFORM_CXXFLAGS += -DFREEBSD
	LDFLAGS += -lpthread
else ifeq ($(OS),macosx)
	PLATFORM_CXXFLAGS += -DMACOSX
	LDFLAGS += -lpthread
else ifeq ($(OS),haikuos)
	PLATFORM_CXXFLAGS += -DHAIKUOS
	LDFLAGS += -lpthread -lnetwork
endif

CXXFLAGS += $(PLATFORM_CXXFLAGS)

ifeq ($(FORCE_STATIC),1)
	LDFLAGS += -static
endif

ifdef DEBUG
	CXXFLAGS += -DDEBUG -g3 -Og
else
	CXXFLAGS += -O3
endif

ifeq ($(ASAN),1)
	CXXFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address -static-libasan
endif

ifdef EXTRACXX
	CXXFLAGS += $(EXTRACXX)
endif

ifdef EXTRALDFLAGS
	LDFLAGS += $(EXTRALDFLAGS)
endif

ifneq ($(NOCURL),1)
	OBJECTS += web.o
	CXXFLAGS += `$(PKGCONFIG) --cflags libcurl`
	LDFLAGS += `$(PKGCONFIG) --libs libcurl`
else
	CXXFLAGS += -DNOCURL
endif

all:
ifndef OS
	@echo "No operating system specified"
	exit 1
endif

ifndef CPU
	@echo "No CPU architecture specified"
	exit 1
endif

ifndef SERVER
	@echo "No server domain name specified"
	exit 1
endif
	$(MAKE) -C . objects
	

objects: identity_modulecpp filescpp scriptcpp interfacecpp jobscpp updatescpp maincpp processescpp cmdhandlingcpp
ifeq ($(OS),windows)
	$(MAKE) -C ./win32 all
endif

ifneq ($(NOCURL),1)
	$(MAKE) -C . webcpp
endif
	
	mkdir -p .libvl
	$(MAKE) -C ../libvolition HIDE_GUTS=1 OBJ_OUT_DIR="`pwd`/.libvl" EXTRACXX="$(PLATFORM_CXXFLAGS)"
	$(MAKE) -C brander
	$(CXX) $(OBJECTS) -o $(OUTFILE) $(LDFLAGS) $(EXTRA_LDFLAGS)
	@echo
	@echo "Branding binary with specified node identity..."
	./brander/brander $(OUTFILE) --id $(IDENTITY) --authtoken $(AUTHTOKEN) --server $(SERVER) --revision $(REVISION) --platformstring $(OS).$(CPU) --compiletime $(shell date +%s) --cert $(ROOTCERT)
	@echo "Done."
	@echo


jobscpp:
	$(CXX) jobs.cpp -c $(CXXFLAGS)
updatescpp:
	$(CXX) updates.cpp -c $(CXXFLAGS)
scriptcpp:
	$(CXX) script.cpp -c $(CXXFLAGS)
webcpp:
	$(CXX) web.cpp -c $(CXXFLAGS)
filescpp:
	$(CXX) files.cpp -c $(CXXFLAGS)
interfacecpp:
	$(CXX) interface.cpp -c $(CXXFLAGS)
identity_modulecpp:
	$(CXX) identity_module.cpp -c $(CXXFLAGS)
maincpp:
	$(CXX) main.cpp -c $(CXXFLAGS)
processescpp:
	$(CXX) processes.cpp -c $(CXXFLAGS)
cmdhandlingcpp:
	$(CXX) cmdhandling.cpp -c $(CXXFLAGS)


clean:
	$(MAKE) -C win32 clean
	$(MAKE) -C brander clean
	rm -f *.o *.gch volition_node*
	rm -rf .libvl
