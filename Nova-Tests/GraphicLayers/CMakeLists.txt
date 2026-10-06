## NOVA-TESTS / GraphicLayers ##

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS *.h *.hpp)

add_executable(Nova-Tests-GraphicLayers ${SOURCES} ${HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS})

target_link_libraries(Nova-Tests-GraphicLayers PRIVATE Nova-Core)

target_include_directories(Nova-Tests-GraphicLayers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(Nova-Tests-GraphicLayers PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${NOVA_BIN_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/"
)

target_compile_definitions(Nova-Tests-GraphicLayers
    PRIVATE
        $<$<CONFIG:Debug>:NOVA_DEBUG>
        $<$<CONFIG:Release>:NOVA_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:NOVA_RELWITHDEBINFO>
        $<$<CONFIG:MinSizeRel>:NOVA_MINSIZEREL>
)

add_test(NAME GraphicLayers COMMAND Nova-Tests-GraphicLayers)