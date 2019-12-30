cmake_minimum_required(VERSION 3.1.0)
project("VLZiggurat")


set(EXTRA_CXXFLAGS)
set(EXTRA_LD "")

if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -fPIC -DDEBUG -pedantic -Wall -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -Ofast -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()


	
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-R  -Wl,.")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
message("== Searching for Qt5Widgets...")
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Widgets Core)

message("== Ok, found Qt5Widgets")

set(sourcefiles ziggurat.h luadelegate.cpp widgets.cpp zigresources.qrc)
qt5_wrap_ui(sourcefiles zigmessenger.ui zigmainwindow.ui zigtextchooser.ui)

add_library(Ziggurat SHARED ${sourcefiles})
add_library(Ziggurat_static STATIC ${sourcefiles})

target_include_directories(Ziggurat PRIVATE "../../")
target_include_directories(Ziggurat_static PRIVATE "../..")

set_property(TARGET Ziggurat_static PROPERTY CXX_STANDARD 14)
set_property(TARGET Ziggurat_static PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET Ziggurat PROPERTY CXX_STANDARD 14)
set_property(TARGET Ziggurat PROPERTY CXX_STANDARD_REQUIRED ON)

qt5_use_modules(Ziggurat Core Widgets)
qt5_use_modules(Ziggurat_static Core Widgets)

if (WIN32)
	set(EXTRA_LD ${EXTRA_LD} -lws2_32)
endif()

target_link_libraries(Ziggurat ${EXTRA_LD})
