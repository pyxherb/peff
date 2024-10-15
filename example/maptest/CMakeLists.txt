file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)

add_executable(maptest ${HEADERS} ${SRC})
target_link_libraries(maptest PRIVATE phul_base phul_containers)
set_target_properties(maptest PROPERTIES CXX_STANDARD 17)
