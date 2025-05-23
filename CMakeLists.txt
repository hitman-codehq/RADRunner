
cmake_minimum_required(VERSION 3.16)

project(RADRunner)

if(MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_options(/EHsc /W4)
else()
	add_compile_options(-fno-asynchronous-unwind-tables -Wall -Wextra)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(_DEBUG MUNGWALL_NO_LINE_TRACKING)
endif()

set(sourceFiles "RADRunner.cpp")
set(sourceFiles ${sourceFiles} "ClientCommands.cpp")
set(sourceFiles ${sourceFiles} "Execute.cpp")
set(sourceFiles ${sourceFiles} "ServerCommands.cpp")

find_library(StdFuncs StdFuncs PATHS ../StdFuncs PATH_SUFFIXES build)
find_package(Qt6 COMPONENTS Core Gui Widgets)

if(${Qt6_FOUND} EQUAL 1)
	message(STATUS "Configuring using Qt6 directory ${Qt6_DIR}")

	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	set(CMAKE_AUTOUIC ON)

	add_executable(RADRunner ${sourceFiles})
	add_compile_definitions(QT_GUI_LIB)
	target_link_libraries(RADRunner ${StdFuncs})
	target_link_libraries(RADRunner Qt6::Core Qt6::Gui Qt6::Widgets)
else()
	message(STATUS "Qt6 not found, configuring basic build")

	add_executable(RADRunner ${sourceFiles})
	target_link_libraries(RADRunner ${StdFuncs})
endif()

set_property(TARGET RADRunner PROPERTY CXX_STANDARD 14)
set_property(TARGET RADRunner PROPERTY CXX_STANDARD_REQUIRED ON)

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:CONSOLE")
	set_target_properties(RADRunner PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
endif()

target_include_directories(RADRunner PRIVATE "../StdFuncs")

if(WIN32)
	target_link_libraries(RADRunner "ws2_32")
endif()

if(USE_CLANG_TIDY)
	find_program(ClangTidy NAMES "clang-tidy")

	if(ClangTidy)
		message(STATUS "Found clang-tidy as ${ClangTidy}")
		set(RunClangTidy ${ClangTidy})
		set_target_properties(RADRunner PROPERTIES C_CLANG_TIDY "${RunClangTidy}" CXX_CLANG_TIDY "${RunClangTidy}")
	else()
		message(STATUS "clang-tidy not found, so will not be used")
	endif()
endif()
