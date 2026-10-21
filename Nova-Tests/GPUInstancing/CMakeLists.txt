## NOVA-TESTS / GPUInstancing ##

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS *.h *.hpp)

add_executable(Nova-Tests-GPUInstancing ${SOURCES} ${HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS})

target_link_libraries(Nova-Tests-GPUInstancing PRIVATE Nova-Core)

target_include_directories(Nova-Tests-GPUInstancing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

nova_set_runtime_output(Nova-Tests-GPUInstancing)
nova_configure_target(Nova-Tests-GPUInstancing)

add_test(NAME GPUInstancing COMMAND Nova-Tests-GPUInstancing)