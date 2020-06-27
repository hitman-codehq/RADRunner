
cmake_minimum_required(VERSION 3.7.1)

project(RADRunner)

# TODO: CAW - Add -Wall etc.
if(MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(_DEBUG MUNGWALL_NO_LINE_TRACKING)
endif()

set(sourceFiles "RADRunner.cpp")
set(sourceFiles ${sourceFiles} "ClientCommands.cpp")
set(sourceFiles ${sourceFiles} "ServerCommands.cpp")
set(sourceFiles ${sourceFiles} "DirWrapper.cpp")
set(sourceFiles ${sourceFiles} "StdSocket.cpp")

add_executable(RADRunner ${sourceFiles})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:CONSOLE")
	set_target_properties(RADRunner PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
endif()

target_include_directories(RADRunner PRIVATE "../StdFuncs")

find_library(StdFuncs StdFuncs PATHS ../StdFuncs PATH_SUFFIXES build)
target_link_libraries(RADRunner ${StdFuncs})

if(WIN32)
	target_link_libraries(RADRunner "libwsock32.a")
endif()

if(USE_CLANG_TIDY)
	find_program(ClangTidy NAMES "clang-tidy")

	if(ClangTidy)
		message(STATUS "Found clang-tidy as ${ClangTidy}")
		set(RunClangTidy ${ClangTidy})
		set_target_properties(RADRunner PROPERTIES C_CLANG_TIDY "${RunClangTidy}" CXX_CLANG_TIDY "${RunClangTidy}")
	endif()
endif()
