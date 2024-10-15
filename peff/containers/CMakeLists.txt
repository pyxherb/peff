peff_add_component(peff_containers)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(peff_containers PRIVATE ${HEADERS} ${SRC})
target_compile_definitions(peff_containers PRIVATE IS_PEFF_CONTAINERS_BUILDING=1)

target_link_libraries(peff_containers PUBLIC peff_base peff_utils)
