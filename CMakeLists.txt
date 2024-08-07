cmake_minimum_required(VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(fluid-c VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json

add_executable(${CMAKE_PROJECT_NAME} 
	"src/main.c"
	"src/common.h"
	"src/misc.c"
	"src/misc.h"
	"src/perf.c"
	"src/perf.h"
	"src/pixel.h"
	"src/mat.c" 
	"src/mat.h" 
	"src/sim.c"
	"src/sim.h"
	"src/vis.c" 
	"src/vis.h" 
	"src/app.c" 
	"src/app.h")

# https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#genex:IF
# https://stackoverflow.com/a/72330784
if(MSVC)
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4 $<$<NOT:$<CONFIG:Debug>>:/Ox>)
else()
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic $<$<NOT:$<CONFIG:Debug>>:-O3>)
endif()

# https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")