## NOVA ##

cmake_minimum_required(VERSION 3.10)

project(Nova)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(Nova-Core)
add_subdirectory(Nova-App)

# Set Nova-App as the startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Nova-App)