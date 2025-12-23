## NOVA ##

cmake_minimum_required(VERSION 3.10)

project(Nova)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(Nova-Core)
add_subdirectory(Nova-App)

# Set Nova-App as the startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Nova-App)