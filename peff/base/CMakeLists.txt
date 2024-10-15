peff_add_component(peff_base)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(peff_base PRIVATE ${HEADERS} ${SRC})
target_compile_definitions(peff_base PRIVATE IS_PEFF_BASE_BUILDING=1)
