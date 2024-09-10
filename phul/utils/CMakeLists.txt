phul_add_component(phul_utils)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(phul_utils_targets PUBLIC ${HEADERS} ${SRC})
target_compile_definitions(phul_utils_targets PRIVATE IS_PHUL_UTILS_BUILDING=1)

phul_component_link_component(phul_utils phul_base)
