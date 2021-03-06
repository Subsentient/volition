cmake_minimum_required(VERSION 3.1.0)
project("VLNode")

set(TGT_LDFLAGS)
set(TGT_CXXFLAGS)

execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE REVISION_)
execute_process(COMMAND date +%s OUTPUT_VARIABLE COMPILETIME_)
string(STRIP ${REVISION_} REVISION)
string(STRIP ${COMPILETIME_} COMPILETIME)

if (NOT OS)
	execute_process(COMMAND uname OUTPUT_VARIABLE DETECTEDOS)
	string(STRIP ${DETECTEDOS} OS)
endif()

if (NOT CPU)
	execute_process(COMMAND uname -m OUTPUT_VARIABLE DETECTEDCPU)
	string(STRIP ${DETECTEDCPU} CPU_)
	string(TOUPPER ${CPU_} CPU)
endif()

string(TOLOWER ${CPU} LOWERCPU)

if (WIN32)
	set(TGT_LDFLAGS -lws2_32 -ldl -mwindows -Wl,--stack,8388608)
	set(TGT_CXXFLAGS -DWIN32 -DARCH_${CPU})
	set(OS windows)
	set(LOWEROS windows)
else()
	set(TGT_LDFLAGS -pthread -ldl)
	string(TOLOWER ${OS} LOWEROS)
	string(TOUPPER ${OS} OSDEF)

	set(TGT_CXXFLAGS -D${OSDEF} -DARCH_${CPU})
endif()


if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -Wno-ignored-attributes -fPIC -DDEBUG -pedantic -Wall -g3 -Og -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${TGT_CXXFLAGS} ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -Wno-ignored-attributes -fPIC -pedantic -Wall -Ofast -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${TGT_CXXFLAGS} ${EXTRA_CXXFLAGS})
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../libvolition ${CMAKE_CURRENT_BINARY_DIR}/libvolition/build EXCLUDE_FROM_ALL)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
message("== Searching for pkg-config...")
find_package(PkgConfig)
message("== Ok, found pkg-config")

message("== Searching for OpenSSL...")
pkg_check_modules(OPENSSL REQUIRED openssl)
message("== OK, found OpenSSL.")

message("== Searching for Lua...")
pkg_check_modules(LUA REQUIRED lua)
message("== OK, found Lua")

message("== Searching for curl...")
pkg_check_modules(CURL libcurl)


if (LUAFFI)
	message("== Searching for libffi...")
	pkg_check_modules(LIBFFI libffi)
	add_compile_options("-DLUAFFI")
endif()

set(CURLFLAGS "-DNOCURL")
set(CURLLD)

set(sourcefiles processes.cpp interface.cpp main.cpp files.cpp cmdhandling.cpp jobs.cpp updates.cpp identity_module.cpp script.cpp)

if (WIN32)
	set(sourcefiles ${CMAKE_CURRENT_LIST_DIR}/win32/win.rc ${sourcefiles})
endif()

if (CURL_FOUND)
	message("== OK, found curl.")
	set(CURLFLAGS ${CURL_CFLAGS})
	set(CURLLD ${CURL_LIBRARIES} ${CURL_LIBRARIES_OTHER})
	set(sourcefiles web.cpp ${sourcefiles})
else()
	message("== WARNING: curl not found, but curl is optional. Disabling web.cpp")
endif()

add_compile_options(${CURLFLAGS})


add_executable(volition_node ${sourcefiles})
add_library(libnode STATIC EXCLUDE_FROM_ALL ${sourcefiles})
set_target_properties(libnode PROPERTIES PREFIX "")

if (LUAFFI)
	set(EXTENSION_FLAGS "${CMAKE_CURRENT_LIST_DIR}/luaffi/luaffi.a")
endif()

if (IDENTITY)
	add_custom_command(TARGET volition_node POST_BUILD COMMAND ${CMAKE_CURRENT_LIST_DIR}/../utils/build/brander $<TARGET_FILE:volition_node> --id ${IDENTITY})
endif()

if (SERVER)
	add_custom_command(TARGET volition_node POST_BUILD COMMAND ${CMAKE_CURRENT_LIST_DIR}/../utils/build/brander $<TARGET_FILE:volition_node> --server ${SERVER})
endif()

if (CERT)
	add_custom_command(TARGET volition_node POST_BUILD COMMAND ${CMAKE_CURRENT_LIST_DIR}/../utils/build/brander $<TARGET_FILE:volition_node> --cert ${CERT})
endif()
if (AUTHTOKEN)
	add_custom_command(TARGET volition_node POST_BUILD COMMAND ${CMAKE_CURRENT_LIST_DIR}/../utils/build/brander $<TARGET_FILE:volition_node> --authtoken ${AUTHTOKEN})
endif()


add_custom_command(TARGET volition_node POST_BUILD COMMAND ${CMAKE_CURRENT_LIST_DIR}/../utils/build/brander $<TARGET_FILE:volition_node> --platformstring ${LOWEROS}.${LOWERCPU} --revision ${REVISION} --compiletime ${COMPILETIME})

if (IDENTITY)
	add_custom_command(TARGET volition_node POST_BUILD COMMAND mv -fv $<TARGET_FILE:volition_node> volition_node_${IDENTITY}-${LOWEROS}.${LOWERCPU}${CMAKE_EXECUTABLE_SUFFIX})
endif()
target_include_directories(volition_node PUBLIC ${LUA_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${LIBFFI_INCLUDE_DIRS})

target_compile_options(libnode PUBLIC ${LIBNODE_CXXFLAGS})

target_link_libraries(volition_node libvolition_node ${CURLFLAGS} ${OPENSSL_LIBRARIES} ${LUA_LIBRARIES} ${CURLLD} ${TGT_LDFLAGS} ${EXTRA_LD} ${EXTENSION_FLAGS} ${LIBFFI_LIBRARIES})

set_property(TARGET volition_node PROPERTY CXX_STANDARD 14)
set_property(TARGET volition_node PROPERTY CXX_STANDARD_REQUIRED ON)
