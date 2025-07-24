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

# Dear ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendors/imgui)
include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

# glm
add_subdirectory(${CMAKE_SOURCE_DIR}/vendors/glm/glm)
include_directories(${CMAKE_SOURCE_DIR}/vendors/glm)

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

    set(GLEW_ROOT_DIR "${PROJECT_SOURCE_DIR}/vendors/GLEW")
    set(GLEW_INCLUDE_DIR "${GLEW_ROOT_DIR}/include")
    include_directories(${GLEW_INCLUDE_DIR})

    set(GLEW_LIB_DIR     "${GLEW_ROOT_DIR}/lib/Release/x64")
    set(GLEW_DLL_DIR     "${GLEW_ROOT_DIR}/Release/x64")

    set(LIBRARIES SDL3::SDL3 glm::glm "${GLEW_LIB_DIR}/glew32.lib" opengl32)
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
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
        set(LIBRARIES SDL3::SDL3;GL;GLEW::GLEW)
    else()
        set(GLEW_ROOT_DIR "${PROJECT_SOURCE_DIR}/vendors/GLEW")
        set(GLEW_INCLUDE_DIR "${GLEW_ROOT_DIR}/include")
        include_directories(${GLEW_INCLUDE_DIR})

        set(GLEW_LIB_DIR     "${GLEW_ROOT_DIR}/lib/Release/x64")
        set(GLEW_DLL_DIR     "${GLEW_ROOT_DIR}/Release/x64")

        set(LIBRARIES SDL3::SDL3 glm::glm "${GLEW_LIB_DIR}/glew32.lib" opengl32)
    endif()
endif()

# Source files
file(GLOB_RECURSE SOURCES src/**.cpp src/**.h)
file(GLOB_RECURSE HEADERS include/**.hpp include/**.h)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS}
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

if(WIN32)

    get_target_property(_out_dir ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
    if (NOT _out_dir)
        set(_out_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy 
        $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       "${GLEW_DLL_DIR}/glew32.dll"
       $<TARGET_FILE_DIR:${PROJECT_NAME}>
   )

endif()

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

file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}/assets/shaders" SHADER_DIR)
target_compile_definitions(${PROJECT_NAME} PRIVATE SHADER_DIR="${SHADER_DIR}")