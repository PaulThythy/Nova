## NOVA-TESTS / BuildTypes ##

add_executable(Nova-Tests-BuildTypes main.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES main.cpp)

target_link_libraries(Nova-Tests-BuildTypes PRIVATE Nova-Core)

target_include_directories(Nova-Tests-BuildTypes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

nova_set_runtime_output(Nova-Tests-BuildTypes)
nova_configure_target(Nova-Tests-BuildTypes)

add_test(NAME BuildTypes COMMAND Nova-Tests-BuildTypes)
