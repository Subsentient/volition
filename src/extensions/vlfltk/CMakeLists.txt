cmake_minimum_required(VERSION 3.1.0)
project("VLZiggurat")

set(LIBNODE_CXXFLAGS -DAPPENDSCRIPTFUNC=luaopen_fltk4lua)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../node ${CMAKE_CURRENT_BINARY_DIR}/libnode EXCLUDE_FROM_ALL)

set(EXTRA_CXXFLAGS)
set(EXTRA_LD "")

if (NOT DEFINED FLTKCONFIG)
	set(FLTKCONFIG "fltk-config")
endif()

if (STATIC)
	set(EXTRA_CXXFLAGS ${EXTRA_CXXFLAGS} -DSTATIC)
endif()

if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -fPIC -DDEBUG -pedantic -Wall -g3 -Og -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -Ofast -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()


if (WIN32)
	set(EXTRA_LD ${EXTRA_LD} -lws2_32 -Wl,--stack,8388608)
else()
	set(EXTRA_LD ${EXTRA_LD} -pthread)
endif()

message("== Searching for FLTK...")

execute_process(COMMAND ${FLTKCONFIG} --cxxflags --use-images OUTPUT_VARIABLE FLTK_CXXFLAGS_)
string(STRIP ${FLTK_CXXFLAGS_} FLTK_CXXFLAGS)

if (STATIC)
	execute_process(COMMAND ${FLTKCONFIG} --ldstaticflags --use-images OUTPUT_VARIABLE FLTK_LDFLAGS_)
else()
	execute_process(COMMAND ${FLTKCONFIG} --ldflags --use-images OUTPUT_VARIABLE FLTK_LDFLAGS_)
endif()

string(STRIP ${FLTK_LDFLAGS_} FLTK_LDFLAGS)

message("== OK, found FLTK")

find_package(PkgConfig)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-R  -Wl,. -Wl,-R  -Wl,.. -Wl,-R -Wl,vlfltk")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

message("== Searching for OpenSSL...")
pkg_check_modules(OPENSSL REQUIRED openssl)
message("== OK, found OpenSSL.")

message("== Searching for Lua...")
pkg_check_modules(LUA REQUIRED lua)
message("== OK, found Lua")

message("== Searching for curl...")
pkg_check_modules(CURL REQUIRED libcurl)
message("== OK, found curl.")

if (WIN32)
	add_executable(vlfltk_node empty.cpp ${CMAKE_CURRENT_LIST_DIR}/../../node/win32/win.rc)
else()
	add_executable(vlfltk_node empty.cpp)
endif()
#add_custom_command(TARGET vlfltk_node PRE_BUILD COMMAND make ARGS CC=${CC} RANLIB=${RANLIB} AR=${AR} static -C ${CMAKE_CURRENT_LIST_DIR}/fltk4lua)


target_include_directories(vlfltk_node PRIVATE "../..")

target_include_directories(vlfltk_node PUBLIC ${LUA_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})
add_compile_options(${LUA_CFLAGS} ${CURL_CFLAGS} ${OPENSSL_CFLAGS} ${FLTK_CXXFLAGS})

set_property(TARGET vlfltk_node PROPERTY CXX_STANDARD 14)
set_property(TARGET vlfltk_node PROPERTY CXX_STANDARD_REQUIRED ON)



if (WIN32)
	if (NOT DEBUG)
		set(EXTRA_LD ${EXTRA_LD} -mwindows)
	endif()
endif()

target_link_libraries(vlfltk_node libnode libvolition_node ${EXTRA_LD} ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} ${LUA_LIBRARIES} -ldl ${CMAKE_CURRENT_LIST_DIR}/fltk4lua/fltk4lua.a ${FLTK_LDFLAGS})
