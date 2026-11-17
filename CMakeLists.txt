cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
cmake_policy(SET CMP0115 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)

if(CMAKE_HOST_UNIX OR UNIX)
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x86-linux" CACHE STRING "")
    endif()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
elseif(CMAKE_HOST_WIN32 OR WIN32)
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x86-windows" CACHE STRING "")
    endif()
endif()
#==============================================================================#
set(PLUGIN_NAME "ServerConfigManager")
set(PLUGIN_VERSION_MAJOR 0)
set(PLUGIN_VERSION_MINOR 0)
set(PLUGIN_VERSION_PATCH 1)
set(PLUGIN_VERSION_BUILD 1)
#==============================================================================#
project(${PLUGIN_NAME} LANGUAGES C CXX VERSION ${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}.${PLUGIN_VERSION_PATCH}.${PLUGIN_VERSION_BUILD})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(DEFINED CMAKE_C_COMPILER_ID)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        message(FATAL_ERROR "Unsupported CMAKE_C_COMPILER_ID: '${CMAKE_C_COMPILER_ID}'...")
    endif()
elseif(DEFINED CMAKE_CXX_COMPILER_ID)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(FATAL_ERROR "Unsupported CMAKE_CXX_COMPILER_ID: '${CMAKE_CXX_COMPILER_ID}'...")
    endif()
endif()
if(DEFINED CMAKE_GENERATOR)
    if(NOT CMAKE_GENERATOR STREQUAL "Unix Makefiles" AND
       NOT CMAKE_GENERATOR STREQUAL "Ninja" AND
       NOT CMAKE_GENERATOR MATCHES "(Visual Studio)[A-Za-z0-9 ]+")
        message(FATAL_ERROR "Unsupported CMAKE_GENERATOR: '${CMAKE_GENERATOR}'...")
    endif()
endif()
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(FATAL_ERROR "CMAKE_CXX_STANDARD has been set to '${CMAKE_CXX_STANDARD}' which is lower than the minimum required standard (c++20).")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "component MUST be compiled as 32-bit (x86).")
endif()

string(TOLOWER ${PLUGIN_NAME} PLUGIN_NAME_LOWERCASE)

### .........
function(ListFilesInSubmodule directory_path pattern)
    set(_maxDepth -1)
    if(ARGC GREATER 2)
        set(_maxDepth ${ARGV2})
        if(NOT _maxDepth MATCHES "^[0-9]+$")
            message(FATAL_ERROR "ListFilesInSubmodule: max_depth must be a non-negative integer, got '${_maxDepth}'.")
        endif()
    endif()

    # Get absolute normalized path
    get_filename_component(_baseDir "${directory_path}" ABSOLUTE)

    # Use native CMake globbing (no PowerShell / Bash required)
    file(GLOB_RECURSE _matchedFiles CONFIGURE_DEPENDS RELATIVE "${_baseDir}" "${_baseDir}/${pattern}")

    unset(_Temp)
    foreach(_relFile IN LISTS _matchedFiles)
        # Calculate depth by counting path separators
        string(REGEX MATCHALL "/" _slashes "${_relFile}")
        list(LENGTH _slashes _slashCount)
        math(EXPR _fileDepth "${_slashCount} + 1")

        if(_maxDepth EQUAL -1 OR _fileDepth LESS_EQUAL _maxDepth)
            list(APPEND _Temp "${_baseDir}/${_relFile}")
        endif()
    endforeach()

    if(NOT _Temp)
        set(_maxDepthLog "")
        if(NOT _maxDepth EQUAL -1)
            set(_maxDepthLog " (max_depth=${_maxDepth})")
        endif()
        message(STATUS "No files found matching pattern '${pattern}' in '${directory_path}'${_maxDepthLog}.")
        return()
    endif()

    # Push accumulated files to parent scope
    set(PLUGIN_STRUCTURES ${PLUGIN_STRUCTURES} ${_Temp} PARENT_SCOPE)
endfunction()
### .........
ListFilesInSubmodule("${CMAKE_CURRENT_SOURCE_DIR}/src" "*.cpp" 1)
ListFilesInSubmodule("${CMAKE_CURRENT_SOURCE_DIR}/include" "*.hpp" 1)
ListFilesInSubmodule("${CMAKE_CURRENT_SOURCE_DIR}/include" "*.h" 1)

foreach(PROJECT_FILE IN ITEMS ${PLUGIN_STRUCTURES})
    get_filename_component(FileExt ${PROJECT_FILE} EXT)
    if(FileExt STREQUAL ".hpp" OR FileExt STREQUAL ".h")
        set(PLUGIN_HDR
            ${PLUGIN_HDR}
            ${PROJECT_FILE}
        )
    elseif(FileExt STREQUAL ".cpp" OR FileExt STREQUAL ".c" OR FileExt STREQUAL ".cc")
        set(PLUGIN_SRC
            ${PLUGIN_SRC}
            ${PROJECT_FILE}
        )
    endif()
endforeach()

add_library(${PLUGIN_NAME_LOWERCASE} SHARED)
target_sources(${PLUGIN_NAME_LOWERCASE} PUBLIC ${PLUGIN_SRC} ${PLUGIN_HDR})
set_target_properties(${PLUGIN_NAME_LOWERCASE} PROPERTIES PREFIX "")

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/omp-sdk)
target_include_directories(${PLUGIN_NAME_LOWERCASE} SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/deps/omp-sdk/include)

file(GLOB PawnNatives_HDR "${CMAKE_SOURCE_DIR}/deps/pawn-natives/*.hpp" "${CMAKE_SOURCE_DIR}/deps/pawn-natives/Internal/*.hpp")
add_library(PawnNatives OBJECT ${PawnNatives_HDR})
set_target_properties(PawnNatives PROPERTIES LINKER_LANGUAGE CXX)
if(DEFINED UNIX)
    target_compile_options(PawnNatives PRIVATE -m32 -fPIC)
    target_link_options(PawnNatives PRIVATE -m32)
endif()
target_include_directories(PawnNatives
    SYSTEM PUBLIC
        ${CMAKE_SOURCE_DIR}/deps/pawn-natives
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps>
        $<INSTALL_INTERFACE:>
)
target_link_libraries(PawnNatives PRIVATE OMP-SDK)

target_include_directories(${PLUGIN_NAME_LOWERCASE} PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_library(AMX INTERFACE)
target_include_directories(AMX
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps/amx>
        $<INSTALL_INTERFACE:include>
)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PLUGIN_NAME_LOWERCASE} PRIVATE PawnNatives OMP-SDK AMX nlohmann_json::nlohmann_json)

set_target_properties(${PLUGIN_NAME_LOWERCASE} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ServerConfigManager"
)

if(MSVC)
    target_link_options(${PLUGIN_NAME_LOWERCASE} PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)
endif()

if(DEFINED UNIX)
    add_definitions(-DLINUX -DENABLE_BINRELOC -D_GLIBCXX_USE_CXX11_ABI=0)

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(x64)|(X64)|(amd64)|(AMD64)" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_property(TARGET ${PLUGIN_NAME_LOWERCASE} PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
    endif()
    target_compile_options(${PLUGIN_NAME_LOWERCASE} PRIVATE
        -m32
        -Wall
        -Wextra
        -Werror
        -Wnull-dereference
        -Wpointer-arith
        -Wcast-align
        -Wwrite-strings
        -Wduplicated-cond
        -Wshift-overflow=2
        -Wformat=2
        -Wtrampolines
        -Wvector-operation-performance
        -Wno-old-style-cast
        -Wno-unused-parameter
        -Wno-attributes
        -Wno-ignored-qualifiers
        -Wno-sign-compare
        -Wno-volatile
        -Wno-type-limits
        -Wno-format-zero-length
        -Wno-array-bounds
        -Wno-stringop-overread
        -Wno-stringop-overflow
        -Wno-stringop-truncation
    )
    target_link_options(${PLUGIN_NAME_LOWERCASE} PRIVATE
        -m32
    )
    option(ENABLE_ASAN "Enable AddressSanitizer (requires LD_PRELOAD on uninstrumented hosts)" OFF)
    if(ENABLE_ASAN)
        target_compile_options(${PLUGIN_NAME_LOWERCASE} PRIVATE -fsanitize=address,leak)
        target_link_options(${PLUGIN_NAME_LOWERCASE} PRIVATE -fsanitize=address,leak -static-libasan)
    endif()
    if(EXISTS "/usr/lib/i386-linux-gnu")
        target_link_directories(${PLUGIN_NAME_LOWERCASE} PRIVATE /usr/lib/i386-linux-gnu)
    endif()
	# link runtime statically
    if(STATIC_BUILD)
        add_link_options(
            "-static-libgcc"
            "-static-libstdc++"
        )
    endif()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
elseif(DEFINED WIN32)
	if(NOT CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
		message(FATAL_ERROR "Building for Windows requires -A Win32 flag.")
    endif()

    if(MSVC)
        add_compile_options(/wd4820 /wd4514 /wd4365 /wd4242 /wd4625 /wd4626 /wd5026 /wd5027 /wd5204)
    endif()

    set_property(TARGET ${PLUGIN_NAME_LOWERCASE} PROPERTY POSITION_INDEPENDENT_CODE Off)
	add_definitions(
		-D_CRT_SECURE_NO_WARNINGS
        -DHAVE_STDINT_H
        -DNOGDI # disables ERROR define (conflicts with log level)
		-D_WINSOCK_DEPRECATED_NO_WARNINGS
		-DWin32_LEAN_AND_MEAN
        -DWIN32_LEAN_AND_MEAN
		-D_CRT_DECLARE_NONSTDC_NAMES
		-D_CRT_NONSTDC_NO_WARNINGS
        -DWin32
        -D_Win32
        -D__STDC__
        -DWINDOWS_IGNORE_PACKING_MISMATCH
	)
    if(DEFINED CMAKE_CXX_COMPILER_ID)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall /MP /EHsc /utf-8 /wd4820 /wd4514 /wd4710 /wd4711 /wd5045 /wd4625 /wd4626 /wd5026 /wd5027 /wd5204")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR BUILD_TESTING)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        endif()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MTd")
    elseif(DEFINED CMAKE_C_COMPILER_ID)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zp4 /Wall /MP /EHsc /utf-8")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR BUILD_TESTING)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
        endif()
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MTd")
        set(CMAKE_SHARED_LINKER_FLAGS "/NOLOGO /MACHINE:X86 /INCREMENTAL:NO /NODEFAULTLIB:Win32RT")
    endif()
	# link runtime statically
    if(STATIC_BUILD)
        set(CMAKE_Win32_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

add_definitions(-DSUBHOOK_STATIC -DSUBHOOK_IMPLEMENTATION)
add_definitions(-DHAVE_STDINT_H=1 -DHAVE_I64=1 -DPAWN_CELL_SIZE=64)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
endif()

configure_file("${CMAKE_SOURCE_DIR}/pluginconfig.h.in" "${CMAKE_SOURCE_DIR}/pluginconfig.h")
