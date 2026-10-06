## NOVA-TESTS ##

add_executable(Nova-Tests main.cpp)

target_link_libraries(Nova-Tests PRIVATE Nova-Core)

target_include_directories(Nova-Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(Nova-Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${NOVA_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${NOVA_BIN_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/"
)

target_compile_definitions(Nova-Tests
    PRIVATE
        $<$<CONFIG:Debug>:NOVA_DEBUG>
        $<$<CONFIG:Release>:NOVA_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:NOVA_RELWITHDEBINFO>
        $<$<CONFIG:MinSizeRel>:NOVA_MINSIZEREL>
)