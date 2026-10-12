## NOVA-TESTS / GraphicLayers ##

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS *.h *.hpp)

add_executable(Nova-Tests-GraphicLayers ${SOURCES} ${HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS})

target_link_libraries(Nova-Tests-GraphicLayers PRIVATE Nova-Core)

target_include_directories(Nova-Tests-GraphicLayers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

nova_set_runtime_output(Nova-Tests-GraphicLayers)
nova_configure_target(Nova-Tests-GraphicLayers)

add_test(NAME GraphicLayers COMMAND Nova-Tests-GraphicLayers)
