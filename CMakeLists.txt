cmake_minimum_required(VERSION 3.16)
project(Nova LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set output directory for binaries
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# Set custom build types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Configs" FORCE)

# SDL3
set(SDL_DIR ${CMAKE_SOURCE_DIR}/vendors/SDL)
add_subdirectory(${SDL_DIR})
include_directories(${SDL_DIR}/include)

# --- Compiler-specific flags ---
if (MSVC)
    # MSVC uses cl.exe
    message(STATUS "Configuring for MSVC")

    # Debug
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /W4")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

    # Release
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /W4")
    # (linker default)

    # Distribution: maxi optim + LTO (link-time code generation)
    set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_DIST} /Ox /GL /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_DIST} /LTCG")

    set(LIBRARIES SDL3::SDL3 opengl32)
else()
    # GCC / Clang
    message(STATUS "Configuring for GCC/Clang")

    # Debug
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall")

    # Release
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++20")

    # Distribution
    set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_DIST} -std=c++20 -O3 -march=x86-64-v2 -fomit-frame-pointer -DNDEBUG")

    if(UNIX) 
        set(LIBRARIES SDL3::SDL3;GL)
    else()
        set(LIBRARIES SDL3::SDL3;opengl32)
    endif()
endif()

# Source files
file(GLOB SOURCES src/**.cpp src/**.h)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Linking vendors
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

# Define build-specific macros
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:NOVA_DEBUG>
    $<$<CONFIG:Release>:NOVA_RELEASE>
    $<$<CONFIG:Dist>:NOVA_DIST>
)

# Set output directory for each build type
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_SOURCE_DIR}/build/${OUTPUTCONFIG}/bin
    )
endforeach()