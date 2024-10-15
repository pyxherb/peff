phul_add_component(phul_base)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(phul_base PRIVATE ${HEADERS} ${SRC})
target_compile_definitions(phul_base PRIVATE IS_PHUL_BASE_BUILDING=1)
