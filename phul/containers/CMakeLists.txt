phul_add_component(phul_containers)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(phul_containers_targets PUBLIC ${HEADERS} ${SRC})
target_compile_definitions(phul_containers_targets PRIVATE IS_PHUL_CONTAINERS_BUILDING=1)

phul_component_link_component(phul_containers phul_base)
phul_component_link_component(phul_containers phul_utils)
