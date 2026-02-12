## NOVA-APP ##

file(GLOB_RECURSE SOURCES src/**.cpp)
file(GLOB_RECURSE HEADERS include/**.h include/**.hpp)

add_executable(Nova-App ${SOURCES} ${HEADERS})

target_sources(Nova-App PRIVATE ${SOURCES} ${HEADERS})

# Match VS filters to directory structure on disk
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS})

target_link_libraries(Nova-App Nova-Core)

target_include_directories(Nova-App PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Set working directory for VS debugging
set_target_properties(Nova-App PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")

target_compile_definitions(Nova-App PRIVATE
    NOVA_APP_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
)

target_compile_definitions(Nova-App
    PRIVATE
        NOVA_APP_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
        $<$<CONFIG:Debug>:NOVA_DEBUG>
        $<$<CONFIG:Release>:NOVA_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:NOVA_RELWITHDEBINFO>
        $<$<CONFIG:MinSizeRel>:NOVA_MINSIZEREL>
)