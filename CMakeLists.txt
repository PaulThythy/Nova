## NOVA ##

cmake_minimum_required(VERSION 3.22)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Dist" CACHE STRING "Build configurations" FORCE)
set_property(CACHE CMAKE_CONFIGURATION_TYPES PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel;Dist")

project(Nova)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel" "Dist")
endif()

# Dist mirrors Release flags (optimizations + NDEBUG). Must be set after project().
foreach(_nova_lang C CXX)
    if(NOT CMAKE_${_nova_lang}_FLAGS_DIST)
        set(CMAKE_${_nova_lang}_FLAGS_DIST "${CMAKE_${_nova_lang}_FLAGS_RELEASE}"
            CACHE STRING "Flags used by the ${_nova_lang} compiler during Dist builds." FORCE)
    endif()
endforeach()
foreach(_nova_linker EXE SHARED STATIC MODULE)
    if(NOT CMAKE_${_nova_linker}_LINKER_FLAGS_DIST)
        set(CMAKE_${_nova_linker}_LINKER_FLAGS_DIST "${CMAKE_${_nova_linker}_LINKER_FLAGS_RELEASE}"
            CACHE STRING "Flags used by the linker during Dist ${_nova_linker} builds." FORCE)
    endif()
endforeach()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NOVA_BUILD_TESTS "Build Nova unit/integration tests" OFF)

# Compile artifacts: Build/ (or Build/<Config>/ with single-config presets)
# Executables: Bin/<Config>/  e.g. Bin/Debug/Nova-App
set(NOVA_BIN_DIR "${PROJECT_SOURCE_DIR}/Bin")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/NovaCompileOptions.cmake")

add_subdirectory(Nova-Core)
add_subdirectory(Nova-App)

nova_configure_target(Nova-Core)
nova_configure_target(Nova-App)
nova_set_runtime_output(Nova-App)

if(NOVA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Nova-Tests)
endif()

# Set Nova-App as the startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Nova-App)