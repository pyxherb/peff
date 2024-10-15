peff_add_component(peff_utils)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)
target_sources(peff_utils PRIVATE ${HEADERS} ${SRC})
target_compile_definitions(peff_utils PRIVATE IS_PEFF_UTILS_BUILDING=1)

target_link_libraries(peff_utils PUBLIC peff_base)
