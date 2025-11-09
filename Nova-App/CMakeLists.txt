## NOVA-APP ##

file(GLOB_RECURSE SOURCES src/**.cpp)
file(GLOB_RECURSE HEADERS include/**.h include/**.hpp)

add_executable(Nova-App ${SOURCES} ${HEADERS})

target_sources(Nova-App PRIVATE ${SOURCES} ${HEADERS})

# Match VS filters to directory structure on disk
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS})

target_link_libraries(Nova-App Nova-Core)

target_include_directories(Nova-App PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set working directory for VS debugging
set_target_properties(Nova-App PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Nova-App")