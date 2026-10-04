## NOVA ##

cmake_minimum_required(VERSION 3.22)

project(Nova)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# keep compile artifacts in Build/ ; Nova-App executable goes in Bin/
set(NOVA_BIN_DIR "${PROJECT_SOURCE_DIR}/Bin")

add_subdirectory(Nova-Core)
add_subdirectory(Nova-App)

set_target_properties(Nova-App PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${NOVA_BIN_DIR}"
)

# Set Nova-App as the startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Nova-App)