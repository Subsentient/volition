CXXFLAGS = -std=gnu++14 -pedantic -Wall  -fno-rtti -fno-strict-aliasing -Wno-strict-aliasing `$(PKGCONFIG) --cflags openssl`
OBJECTS = conation.o vlstrings.o netcore.o utils.o vlthreads.o netscheduler.o common_introspect.o 
AR = ar
RANLIB = ranlib
STRIP = strip
OBJ_OUT_DIR = ./

include ../Security.mk

ifdef DEBUG
	CXXFLAGS += -DDEBUG -g3 -pg -O0
else
	CXXFLAGS += -O3
endif
ifndef HIDE_GUTS
	OBJECTS += brander.o 
endif

ifeq ($(IPV6),1)
	CXXFLAGS += -DVL_IPV6
endif

ifeq ($(ASAN),1)
	CXXFLAGS += -fsanitize=address
endif

ifdef EXTRACXX
	CXXFLAGS += $(EXTRACXX)
endif
all: conationcpp netcorecpp vlstringscpp vlthreadscpp utilscpp common_introspectcpp netschedulercpp
ifndef HIDE_GUTS
	$(MAKE) -C . brandercpp
endif
	rm -f $(OBJ_OUT_DIR)/libvolition.a
	cd $(OBJ_OUT_DIR); $(AR) rc $(OBJ_OUT_DIR)/libvolition.a $(OBJECTS); $(RANLIB) $(OBJ_OUT_DIR)/libvolition.a
ifndef DEBUG
	cd $(OBJ_OUT_DIR); $(STRIP) --strip-debug libvolition.a
endif


conationcpp:
	$(CXX) conation.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/conation.o
netcorecpp:
	$(CXX) netcore.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/netcore.o
vlstringscpp:
	$(CXX) vlstrings.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/vlstrings.o
vlthreadscpp:
	$(CXX) vlthreads.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/vlthreads.o
utilscpp:
	$(CXX) utils.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/utils.o
common_introspectcpp:
	$(CXX) common_introspect.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/common_introspect.o
netschedulercpp:
	$(CXX) netscheduler.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/netscheduler.o
brandercpp:
	$(CXX) brander.cpp -c $(CXXFLAGS) -o $(OBJ_OUT_DIR)/brander.o
clean:
	rm -f *.o *.gch include/*.o include/*.gch $(OBJ_OUT_DIR)/*.o $(OBJ_OUT_DIR)/*.a
