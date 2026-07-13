file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)

add_executable(opttest ${HEADERS} ${SRC})
target_link_libraries(opttest PRIVATE peff_base_static peff_utils_static peff_containers_static)
set_target_properties(opttest PROPERTIES CXX_STANDARD 20)
