## NOVA ##

cmake_minimum_required(VERSION 3.22)

project(Nova)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

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