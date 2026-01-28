cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)
set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${PROJ_NAME} CXX)

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
set(PROJ_NAME ${PROJ_NAME})



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
    "src/Arkitekt.h"
    "src/Logging.cpp"
    "src/Arkitekt.cpp"
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

use_props(${PROJ_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE src)
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
	"SECURITY_WIN32;"
	"__i386__;"
	"_WIN32;"
	"WIN32;"
	"_CONSOLE;"
	"UNICODE;"
	"_UNICODE;"
)

target_compile_definitions(${PROJ_NAME} PRIVATE _REENTRANT)

################################################################################
# Compile and link options`
################################################################################


target_compile_options(${PROJ_NAME} PRIVATE
	#! if you change ANY of these (but especially the #! marked flags), you are doing an undefined behavior unless you explicitly know what you are changing and why.
	#! DEFINE the broken behavior and the reason a flag you are changing defines the behavior, otherwise LEAVE THESE ALONE
    
    /permissive-;           #! prevent non-conformance to the C++ standard
    /Zc:rvalueCast;         #* msvc docs: the compiler follows section 5.4 of the C++11 standard and treats only cast expressions that result in non-reference types 
							#* and cast expressions that result in rvalue references to non-function types as rvalue types.
							#* by default: the compiler is non-conforming, and treats all cast expressions that result in rvalue references as rvalues.
    /Zc:referenceBinding;   #! doesn't allow expressions that bind a user-defined type temporary to a non-const lvalue reference. 
							#! By default, or if /Zc:referenceBinding- is specified, the compiler allows such expressions as a Microsoft extension
	/W3;                    #* warning level 3
    /Zc:wchar_t;            #* enable wchar_t as a built-in type (default, no other options disable this, listed for the sake of completeness of implied flag usage)
	#/Z7;					#? generate full debug PDB info
    #/Gm-;                   #! disable minimal rebuild (this is a performance optimization that can cause issues with certain code patterns, so we disable it)
	/Ox;					#* tend to optimize for speed
    /arch:SSE2;             #! use the x86-SSE2 instruction set. (includes floating point opcodes)
    /Gz;                    #! use __stdcall for all functions except member functions
	/Oy-;					#! don't omit frame pointers
    /MD;                    #* use the multithreaded DLL version of the C runtime library (msvcrt.dll)
    /std:c++23preview;      #* use latest (preview) C++ std 23
	/Zc:preprocessor;
    /FC;                    
	/Zc:inline-;			#! do not remove unreferenced data or functions
    /Ot;                    #* optimize for speed
	/Zc:nrvo-;				#! disable optional elision (skipping) of copy / move operations on returning newly instantiated variables from functions.
    /Zc:tlsGuards;			#! enables checks for initialization of thread-local variables in DLLs. Previously, thread-local variables in DLLs weren't correctly initialized. 
							#! Other than on the thread that loaded the DLL, they weren't initialized before first use on threads that existed before the DLL was loaded.
)

target_link_options(${PROJ_NAME} PRIVATE
	/NDEBUG;				#! implies certain things to the linker about symbol removal / combining things it thinks are identical. these flags you should basically never touch
							#! if you do mess with them, the linker will mess with you and you don't win that fight
	/OPT:NOREF;				#! leave unreferenced data and functions alone (but the linker this time) (implicit by /DEBUG)
	/OPT:NOICF;				#! do not iteratively fold identical COMDATs (redundant COMDATs are removed by the linker if true) (also implicit by /DEBUG)
	/LTCG:OFF;				#! NO link-time code generation optimizations. if you aren't sure why this matters DO NOT TOUCH IT
	#/ASSEMBLYDEBUG; 		#? emits the DebuggableAttribute attribute with debug information tracking and 
							#! disables JIT optimizations
	/SUBSYSTEM:CONSOLE;
)