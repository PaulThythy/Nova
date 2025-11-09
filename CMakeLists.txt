## NOVA ##

cmake_minimum_required(VERSION 3.10)

project(Nova)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(Nova-Core)
add_subdirectory(Nova-App)