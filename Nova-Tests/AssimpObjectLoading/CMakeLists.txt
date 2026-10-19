## NOVA-TESTS / AssimpObjectLoading ##

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS *.h *.hpp)

add_executable(Nova-Tests-AssimpObjectLoading ${SOURCES} ${HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS})

target_link_libraries(Nova-Tests-AssimpObjectLoading PRIVATE Nova-Core)

target_include_directories(Nova-Tests-AssimpObjectLoading PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

nova_set_runtime_output(Nova-Tests-AssimpObjectLoading)
nova_configure_target(Nova-Tests-AssimpObjectLoading)
nova_set_app_subsystem(Nova-Tests-AssimpObjectLoading)

add_test(NAME AssimpObjectLoading COMMAND Nova-Tests-AssimpObjectLoading)