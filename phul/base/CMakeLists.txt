phul_add_component(phul_base)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(phul_base_targets PUBLIC ${HEADERS} ${SRC})
target_compile_definitions(phul_base_targets PRIVATE IS_PHUL_BASE_BUILDING=1)
