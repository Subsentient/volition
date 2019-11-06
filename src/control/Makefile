PKGCONFIG = pkg-config
GTKCHOICE = 2
REVISION = $(shell git rev-parse --short HEAD)
CXXFLAGS = -std=gnu++14 -pedantic -Wall -Wno-ignored-attributes -Wno-implicit-fallthrough `$(PKGCONFIG) --cflags gtk+-$(GTKCHOICE).0` `$(PKGCONFIG) --cflags openssl` -fno-rtti -fno-strict-aliasing -Wno-strict-aliasing -Wno-parentheses -DCTL_REVISION="\"$(REVISION)\""
OUTFILE = ./volition_control
OBJECTS = main.o gui_base.o gui_icons.o interface.o config.o cmdhandling.o clienttracker.o gui_mainwindow.o gui_menus.o gui_dialogs.o orders.o ticker.o mediarecv.o scriptscanner.o
LDFLAGS = ./.libvl/libvolition.a `$(PKGCONFIG) --libs gtk+-$(GTKCHOICE).0` `$(PKGCONFIG) --libs openssl`
STRIP = strip
EXTRA_LDFLAGS =

ifneq ($(NO_SEC_CXXFLAGS),1)
	include ../Security.mk
endif

ifdef DEBUG
	CXXFLAGS += -DDEBUG -g3 -pg -Og
	LDFLAGS += -pg
else
	CXXFLAGS += -O3
endif

ifeq ($(OS),windows)
	LDFLAGS += -lws2_32 -mwindows -Wl,--stack,8000000
	OUTFILE = ./volition_control.exe
	OBJECTS += ./win32/win.o
else
	LDFLAGS += -pthread
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

all: maincpp gui_basecpp gui_iconscpp gui_mainwindowcpp gui_menuscpp gui_dialogscpp tickercpp scriptscannercpp interfacecpp configcpp cmdhandlingcpp clienttrackercpp orderscpp mediarecvcpp
	mkdir -p .libvl
	$(MAKE) -C ../libvolition OBJ_OUT_DIR=`pwd`/.libvl
ifeq ($(OS),windows)
	$(MAKE) -C ./win32 all
endif

	$(CXX) $(OBJECTS) -o $(OUTFILE) $(LDFLAGS) $(EXTRA_LDFLAGS)
ifndef DEBUG
ifndef NOSTRIP
	$(STRIP) --strip-all $(OUTFILE)
endif
endif

gui_mainwindowcpp:
	$(CXX) gui_mainwindow.cpp -c $(CXXFLAGS)
gui_basecpp:
	$(CXX) gui_base.cpp -c $(CXXFLAGS)
gui_iconscpp:
	$(CXX) gui_icons.cpp -c $(CXXFLAGS)
gui_dialogscpp:
	$(CXX) gui_dialogs.cpp -c $(CXXFLAGS)
gui_menuscpp:
	$(CXX) gui_menus.cpp -c $(CXXFLAGS)
scriptscannercpp:
	$(CXX) scriptscanner.cpp -c $(CXXFLAGS)
tickercpp:
	$(CXX) ticker.cpp -c $(CXXFLAGS)
configcpp:
	$(CXX) config.cpp -c $(CXXFLAGS)
maincpp:
	$(CXX) main.cpp -c $(CXXFLAGS)
cmdhandlingcpp:
	$(CXX) cmdhandling.cpp -c $(CXXFLAGS)
clienttrackercpp:
	$(CXX) clienttracker.cpp -c $(CXXFLAGS)
mediarecvcpp:
	$(CXX) mediarecv.cpp -c $(CXXFLAGS)
orderscpp:
	$(CXX) orders.cpp -c $(CXXFLAGS)
interfacecpp:
	$(CXX) interface.cpp -c $(CXXFLAGS)

clean:
	rm -f *.o *.gch volition_control*
	rm -rf .libvl
	$(MAKE) -C win32 clean
