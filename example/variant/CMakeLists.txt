file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)

add_executable(variant ${HEADERS} ${SRC})
target_link_libraries(variant PRIVATE peff_base_static peff_utils_static peff_containers_static peff_advutils_static)
set_target_properties(variant PROPERTIES CXX_STANDARD 20)
