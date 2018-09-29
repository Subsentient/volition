RANLIB = ranlib
WINDRES = windres
PKGCONFIG = pkg-config
STRIP = strip

ifdef TOOLCHAIN
	CC = $(TOOLCHAIN)-cc
	CXX = $(TOOLCHAIN)-c++
	RANLIB = $(TOOLCHAIN)-ranlib
	WINDRES = $(TOOLCHAIN)-windres
	PKGCONFIG = $(TOOLCHAIN)-pkg-config
	STRIP = $(TOOLCHAIN)-strip
else ifndef CXX
	CC = `which gcc`
	CXX = `which g++`
endif


all:
	@echo "You must choose a component to build. Just cat this Makefile to see targets. It's not long."
trinity: clientnode serverbinary controlbinary
	@echo "Trinity compiled successfully."
library:
	$(MAKE) -C libvolition CC="$(CC)" CXX="$(CXX)" RANLIB=$(RANLIB) WINDRES=$(WINDRES) PKGCONFIG=$(PKGCONFIG) STRIP=$(STRIP) EXTRACXX=$(EXTRACXX) EXTRALDFLAGS=$(EXTRALDFLAGS)
clientnode:
	$(MAKE) -C node CC="$(CC)" CXX="$(CXX)" RANLIB=$(RANLIB) WINDRES=$(WINDRES) PKGCONFIG=$(PKGCONFIG) STRIP=$(STRIP) EXTRACXX=$(EXTRACXX) EXTRALDFLAGS=$(EXTRALDFLAGS)
serverbinary:
	$(MAKE) -C server CC="$(CC)" CXX="$(CXX)" RANLIB=$(RANLIB) WINDRES=$(WINDRES) PKGCONFIG=$(PKGCONFIG) STRIP=$(STRIP) EXTRACXX=$(EXTRACXX) EXTRALDFLAGS=$(EXTRALDFLAGS)
controlbinary:
	$(MAKE) -C control CC="$(CC)" CXX="$(CXX)" RANLIB=$(RANLIB) WINDRES=$(WINDRES) PKGCONFIG=$(PKGCONFIG) STRIP=$(STRIP) EXTRACXX=$(EXTRACXX) EXTRALDFLAGS=$(EXTRALDFLAGS)
utilities:
	$(MAKE) -C utils CC="$(CC)" CXX="$(CXX)" RANLIB=$(RANLIB) WINDRES=$(WINDRES) PKGCONFIG=$(PKGCONFIG) STRIP=$(STRIP) EXTRACXX=$(EXTRACXX) EXTRALDFLAGS=$(EXTRALDFLAGS)

killgch:
	sh -c 'for f in `find . -name *.gch`; do rm -fv $$f; done'
	
clean:
	$(MAKE) -C libvolition clean
	$(MAKE) -C node clean
	$(MAKE) -C server clean
	$(MAKE) -C control clean
	$(MAKE) -C utils clean
distclean: clean
