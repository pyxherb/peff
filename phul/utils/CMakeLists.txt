phul_add_component(phul_utils)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(phul_utils PRIVATE ${HEADERS} ${SRC})
target_compile_definitions(phul_utils PRIVATE IS_PHUL_UTILS_BUILDING=1)

target_link_libraries(phul_utils PUBLIC phul_base)
