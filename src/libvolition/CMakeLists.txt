cmake_minimum_required(VERSION 3.1.0)
project("VLLib")


set(EXTRA_CXXFLAGS "")

set(TGT_LDFLAGS)
set(TGT_CXXFLAGS)

if (NOT OS)
	execute_process(COMMAND uname OUTPUT_VARIABLE DETECTEDOS)
	string(STRIP ${DETECTEDOS} OS)
endif()

if (NOT CPU)
	execute_process(COMMAND uname -m OUTPUT_VARIABLE DETECTEDCPU)
	string(STRIP ${DETECTEDCPU} CPU_)
	string(TOUPPER ${CPU_} CPU)
endif()

if (WIN32)
	set(TGT_CXXFLAGS -DWIN32 -DARCH_${CPU})
	set(OS windows)
	set(LOWEROS windows)
else()
	string(TOLOWER ${OS} LOWEROS)
	string(TOUPPER ${OS} OSDEF)

	set(TGT_CXXFLAGS -D${OSDEF} -DARCH_${CPU})
endif()

if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -fPIC -DDEBUG -pedantic -Wall -g3 -Og -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${EXTRA_CXXFLAGS} ${TGT_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -Ofast -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS} ${TGT_CXXFLAGS})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
message("== Searching for pkg-config...")
find_package(PkgConfig)
message("== Ok, found pkg-config")

message("== Searching for OpenSSL...")
pkg_check_modules(OPENSSL REQUIRED openssl)
message("== OK, found OpenSSL.")

set(node_sourcefiles conation.cpp vlstrings.cpp netcore.cpp utils.cpp vlthreads.cpp netscheduler.cpp common_introspect.cpp)
set(full_sourcefiles ${node_sourcefiles} brander.cpp)

add_library(libvolition STATIC ${full_sourcefiles})
add_library(libvolition_node STATIC ${node_sourcefiles})

target_include_directories(libvolition PUBLIC ${OPENSSL_INCLUDE_DIRS})
target_include_directories(libvolition_node PUBLIC ${OPENSSL_INCLUDE_DIRS})
target_include_directories(libvolition PRIVATE "../../")
target_include_directories(libvolition_node PRIVATE "../../")


set_property(TARGET libvolition_node PROPERTY PREFIX "")
set_property(TARGET libvolition_node PROPERTY CXX_STANDARD 14)
set_property(TARGET libvolition_node PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET libvolition PROPERTY PREFIX "")
set_property(TARGET libvolition PROPERTY CXX_STANDARD 14)
set_property(TARGET libvolition PROPERTY CXX_STANDARD_REQUIRED ON)
