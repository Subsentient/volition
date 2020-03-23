cmake_minimum_required(VERSION 3.1.0)
project("VLControl")


set(EXTRA_CXXFLAGS)
set(TGT_LDFLAGS)
set(TGT_CXXFLAGS)

execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE REVISION_)
string(STRIP ${REVISION_} REVISION)

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
	set(TGT_LDFLAGS -lws2_32 -mwindows)
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
	add_compile_options(-DCTL_REVISION="${REVISION}" -std=gnu++14 -fPIC -DDEBUG -pedantic -Wall -g3 -Og -fno-strict-aliasing -Wno-strict-aliasing -Wno-parentheses -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${TGT_CXXFLAGS} ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-DCTL_REVISION="${REVISION}" -std=gnu++14 -fPIC -pedantic -Wall -Ofast -fno-strict-aliasing -Wno-strict-aliasing -Wno-parentheses -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${TGT_CXXFLAGS} ${EXTRA_CXXFLAGS})
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../libvolition ${CMAKE_CURRENT_BINARY_DIR}/libvolition/build)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
message("== Searching for pkg-config...")
find_package(PkgConfig)
message("== Ok, found pkg-config")

message("== Searching for OpenSSL...")
pkg_check_modules(OPENSSL REQUIRED openssl)
message("== OK, found OpenSSL.")

message("== Searching for GTK+3...")
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
message("== OK, found GTK+3")

set(sourcefiles main.cpp gui_base.cpp gui_icons.cpp interface.cpp config.cpp cmdhandling.cpp clienttracker.cpp gui_mainwindow.cpp gui_menus.cpp gui_dialogs.cpp orders.cpp ticker.cpp mediarecv.cpp scriptscanner.cpp)
if (WIN32)
	set(sourcefiles ${CMAKE_CURRENT_LIST_DIR}/win32/win.rc ${sourcefiles})
endif()
add_executable(volition_control ${sourcefiles})

target_include_directories(volition_control PUBLIC ${GTK3_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})


target_link_libraries(volition_control libvolition ${TGT_LDFLAGS} ${OPENSSL_LIBRARIES} ${GTK3_LIBRARIES} ${EXTRA_LD} -Wl,-R -Wl,. -Wl,-R -Wl,../build -Wl,-R -Wl,../winbuild)

set_property(TARGET volition_control PROPERTY CXX_STANDARD 14)
set_property(TARGET volition_control PROPERTY CXX_STANDARD_REQUIRED ON)
