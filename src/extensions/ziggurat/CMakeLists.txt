cmake_minimum_required(VERSION 3.1.0)
project("VLZiggurat")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../libvolition ${CMAKE_CURRENT_LIST_DIR}/../../libvolition/build)

set(EXTRA_CXXFLAGS)
set(EXTRA_LD "")

if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -fPIC -DDEBUG -pedantic -Wall -g3 -Og -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -Ofast -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()


find_package(PkgConfig)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-R  -Wl,.")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
message("== Searching for Qt5Widgets...")
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (WIN32)
	find_package(Qt5 COMPONENTS Widgets Core Gui FontDatabaseSupport WindowsUIAutomationSupport EventDispatcherSupport WinExtras ThemeSupport)
else()
	find_package(Qt5 COMPONENTS Widgets Core Gui)
endif()

message("== Ok, found Qt5Widgets")

message("== Searching for Lua...")
pkg_check_modules(LUA REQUIRED lua)
message("== OK, found Lua")

message("== Searching for freetype2...")
pkg_check_modules(FREETYPE REQUIRED freetype2)
message("== OK, found freetype2...")

set(sourcefiles ziggurat.h luadelegate.cpp widgets.cpp zigresources.qrc)
qt5_wrap_ui(sourcefiles zigmessenger.ui zigmainwindow.ui zigtextchooser.ui)

add_library(Ziggurat SHARED ${sourcefiles})
add_library(Ziggurat_static STATIC ${sourcefiles})

target_include_directories(Ziggurat PRIVATE "../../")
target_include_directories(Ziggurat_static PRIVATE "../..")

target_include_directories(Ziggurat PUBLIC ${LUA_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})
target_include_directories(Ziggurat_static PUBLIC ${LUA_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
add_compile_options(${LUA_CFLAGS} ${OPENSSL_CFLAGS})

set_property(TARGET Ziggurat_static PROPERTY CXX_STANDARD 14)
set_property(TARGET Ziggurat_static PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET Ziggurat PROPERTY CXX_STANDARD 14)
set_property(TARGET Ziggurat PROPERTY CXX_STANDARD_REQUIRED ON)

qt5_use_modules(Ziggurat Core Widgets)
qt5_use_modules(Ziggurat_static Core Widgets)

if (WIN32)
	set(EXTRA_LD ${EXTRA_LD} -lws2_32 Qt5::WindowsUIAutomationSupport Qt5::QWindowsIntegrationPlugin Qt5::WinExtras Qt5::ThemeSupport Qt5::FontDatabaseSupport Qt5::EventDispatcherSupport -lwtsapi32)
endif()

target_link_libraries(Ziggurat libvolition_node ${EXTRA_LD} ${OPENSSL_LIBRARIES} ${LUA_LIBRARIES} ${FREETYPE_LIBRARIES})
