GCC_VERSION = $(shell $(CXX) -dumpversion)

CXXFLAGS += -fstack-protector-all -ftrapv -D_GLIBCXX_ASSERTIONS -Wformat-security
LDFLAGS += -fstack-protector-all -ftrapv

ifndef DEBUG
	CXXFLAGS += -ffunction-sections -fdata-sections -D_FORTIFY_SOURCE=2
	LDFLAGS += -Wl,-gc-sections
endif

ifeq ($(OS),windows)
	CXXFLAGS += -mthreads
	LDFLAGS += -Wl,--nxcompat -Wl,--dynamicbase
ifeq ($(CPU),x86_64)
	LDFLAGS += -Wl,--high-entropy-va
endif

else
	CXXFLAGS += -fPIC
	LDFLAGS += -pie -fPIE -Wl,-z,relro -Wl,-z,now
endif

#Best stack protection we can get


#x86 CPU?
ifeq ($(CPU),$(filter $(CPU),x86_64 amd64 i386 i486 i586 i686))

#If it's x86 linux, we can enable this.
ifeq ($(OS),linux)
	CXXFLAGS += -fsplit-stack
endif

#Spectre v2 mitigation and whatnot
ifeq ($(shell expr $(GCC_VERSION) \>= 7),1)
	CXXFLAGS += -mindirect-branch=thunk -mfunction-return=thunk
endif

#GCC 8 has some extra stuff we want for x86
ifeq ($(shell expr $(GCC_VERSION) \>= 8),1)
	CXXFLAGS += -fstack-clash-protection
endif

endif
#end of x86 only stuff.

ifneq ($(shell expr $(GCC_VERSION) \>= 7),1)
	CXXFLAGS += -fstack-check
	LDFLAGS += -fstack-check
endif
