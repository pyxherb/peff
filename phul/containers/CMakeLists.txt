phul_add_component(phul_containers)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(phul_containers PRIVATE ${HEADERS} ${SRC})
target_compile_definitions(phul_containers PRIVATE IS_PHUL_CONTAINERS_BUILDING=1)

target_link_libraries(phul_containers PUBLIC phul_base phul_utils)
