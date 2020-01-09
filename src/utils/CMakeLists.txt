cmake_minimum_required(VERSION 3.1.0)
project("VLUtils")


set(EXTRA_CXXFLAGS)
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
	set(TGT_LDFLAGS -lws2_32)
	set(TGT_CXXFLAGS -DWIN32 -DARCH_${CPU})
	set(OS windows)
	set(LOWEROS windows)
else()
	set(TGT_LDFLAGS -pthread)
	string(TOLOWER ${OS} LOWEROS)
	string(TOUPPER ${OS} OSDEF)

	set(TGT_CXXFLAGS -D${OSDEF} -DARCH_${CPU})
endif()


if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -fPIC -DDEBUG -pedantic -Wall -g3 -Og -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${TGT_CXXFLAGS} ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -Ofast -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${TGT_CXXFLAGS} ${EXTRA_CXXFLAGS})
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../libvolition ${CMAKE_CURRENT_LIST_DIR}/../libvolition/build)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
message("== Searching for pkg-config...")
find_package(PkgConfig)
message("== Ok, found pkg-config")

message("== Searching for OpenSSL...")
pkg_check_modules(OPENSSL REQUIRED openssl)
message("== OK, found OpenSSL.")


add_executable(brander brander.cpp)
add_executable(genauthtoken genauthtoken.cpp)
add_executable(tumoreditor tumoreditor.cpp)

target_include_directories(genauthtoken PUBLIC ${OPENSSL_INCLUDE_DIRS})
target_include_directories(brander PUBLIC ${OPENSSL_INCLUDE_DIRS})
target_include_directories(tumoreditor PUBLIC ${OPENSSL_INCLUDE_DIRS})


target_link_libraries(genauthtoken libvolition ${TGT_LDFLAGS} ${OPENSSL_LIBRARIES} ${EXTRA_LD} -Wl,-R -Wl,.)
target_link_libraries(brander libvolition ${TGT_LDFLAGS} ${OPENSSL_LIBRARIES} ${EXTRA_LD} -Wl,-R -Wl,.)
target_link_libraries(tumoreditor libvolition ${TGT_LDFLAGS} ${OPENSSL_LIBRARIES} ${EXTRA_LD} -Wl,-R -Wl,.)

set_property(TARGET genauthtoken PROPERTY CXX_STANDARD 14)
set_property(TARGET genauthtoken PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET tumoreditor PROPERTY CXX_STANDARD 14)
set_property(TARGET tumoreditor PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET brander PROPERTY CXX_STANDARD 14)
set_property(TARGET brander PROPERTY CXX_STANDARD_REQUIRED ON)
