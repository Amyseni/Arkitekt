cmake_minimum_required(VERSION 4.2.3 FATAL_ERROR)
set(SHELL "/usr/bin/zsh")
set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message("Project version: ${PROJECT_VERSION}")

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_VS_PLATFORM_NAME)
	set(CMAKE_VS_PLATFORM_NAME "x86")
endif()
message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86"))
	message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()


################################################################################
# Global configuration types
################################################################################
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CONFIGURATION_TYPES
	"Release"
	CACHE STRING "" FORCE
)

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################
project(${PROJ_NAME} CXX)



################################################################################
# Source groups
################################################################################
include_directories("LibArkitekt")
include_directories("include")
include_directories("src")


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
add_library(${PROJ_NAME} SHARED 
	"src/SigScan.h"
	"src/SigScan.cpp"
	"include/Arkitekt/Assembler.win32.hpp"
	"src/hde32/include/hde32.h"
	"src/hde32/include/table32.h"
	"src/hde32/src/hde32.cpp"
	"src/Arkitekt.cpp"
	"src/Arkitekt.h"
	"src/detours.h"
	"src/hde.h"
	"src/Logging.h"
	"src/Logging.cpp"
	"src/PALMemoryProtection.h"
	"include/dynasm/dasm_proto.h"
	"include/dynasm/dasm_x86.cpp"
)

set(CMAKE_SHARED_LIBRARY_PREFIX_CXX "")

target_compile_features(${PROJ_NAME} PRIVATE cxx_std_23)


find_path(DIRECTX_INCLUDE_DIR d3d11.h
	PATHS "/nvme2/organik/MSVC_Hackaround/msvc-downl/kits/10/Lib/10.0.26100.0/um" # Example path, adjust SDK version
	NO_DEFAULT_PATH
)
find_library(DIRECTX_LIB d3d11.lib
	PATHS "/nvme2/organik/MSVC_Hackaround/msvc-downl/kits/10/Lib/10.0.26100.0/um/x86" # Example path, adjust SDK version and architecture
	NO_DEFAULT_PATH
)
if(DIRECTX_INCLUDE_DIR)
	include_directories(${DIRECTX_INCLUDE_DIR})
endif()


################################################################################
# Target
################################################################################

use_props(${PROJ_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE Src)

set_target_properties(${PROJ_NAME} PROPERTIES
	VS_GLOBAL_KEYWORD "Win32Proj"
)


set_target_properties(${PROJ_NAME} PROPERTIES
	INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)

################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJ_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"

		MultiThreadedDLL
	
)

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJ_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJ_NAME} PRIVATE
	"NDEBUG;"
	"_CRT_SECURE_NO_WARNINGS;"
	"_CRT_NONSTDC_NO_DEPRECATE;"
	"LIBARK_EXPORTS;"
	"SECURITY_WIN32;"
	"__i386__;"
	"_WIN32;"
	"WIN32;"
	"_CONSOLE;"
	"UNICODE;"
	"_UNICODE;"
	"LOGLEVEL=4;"
)

target_compile_definitions(${PROJ_NAME} PRIVATE _REENTRANT)
target_compile_definitions(${PROJ_NAME} PRIVATE LIBARK_EXPORTS)
################################################################################
# Compile and link options`
################################################################################


target_compile_options(${PROJ_NAME} PRIVATE
	#! if you change ANY of these (but especially the #! marked flags), you are doing an undefined behavior unless you explicitly know what you are changing and why.
	#! DEFINE the broken behavior and the reason a flag you are changing defines the behavior, otherwise LEAVE THESE ALONE
    
    /permissive-;           #! prevent non-conformance to the C++ standard
    /Zc:rvalueCast;         #* msvc docs: causes the compiler to follow section 5.4 of the C++11 standard and treats only cast expressions that 
							#* result in non-reference types or rvalue references to non-function types as rvalue types.
    /Zc:referenceBinding;   #! doesn't allow expressions that bind a user-defined type temporary to a non-const lvalue reference.
							#! BASICALLY: never move a temporary variable, unless it is const
	/O2;					#! optimize speed with virtually no safeguards
	/W3;                    #* warning level 3
    /Zc:wchar_t;            #* enable wchar_t as a built-in type (default, listed for the sake of completeness)
	/Zc:preprocessor;
	/Ox;					#* MORE SPEED (implied by /O2)
    /arch:SSE2;             #! use the x86-SSE2 instruction set. (includes floating point opcodes)
    /Gz;                    #! use __cdecl for all functions except member functions
    /Oi;                    #! enable intrinsic functions because THEY FAST
	/Ob2;                   #! allow maximum inlining (function level)
							#! this is okay only because #pragma statements are used to disable inlining of speicfic functions.
    /MD;                    #* idk something about building a DLL
	/Zc:inline-;			#! do not remove unreferenced data or functions (well named flag alert.)
    /Ot;                    #* EVEN MORE SPEED (implied by /O2)
	/Zc:nrvo-;				#! When returning a call to an initializer (e.g. return RValue(5);), always compile explicit move/copy
    /Zc:tlsGuards;			#! enable checks for initialization of thread-local variables for non-main threads in DLLs.
)
target_link_options(${PROJ_NAME} PRIVATE
	/OPT:NOREF;
	/OPT:NOICF;
	/SUBSYSTEM:CONSOLE;
)

