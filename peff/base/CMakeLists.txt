file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)

add_library(peff_base SHARED)
target_compile_definitions(peff_base PRIVATE PEFF_DYNAMIC_LINK=1 IS_PEFF_BASE_BUILDING=1)
set_target_properties(peff_base PROPERTIES CXX_STANDARD 17)
target_sources(peff_base PRIVATE ${HEADERS} ${SRC})
target_include_directories(peff_base PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(peff_base PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    ARCHIVE FALSE
)

add_library(peff_base_static STATIC)
target_compile_definitions(peff_base_static PRIVATE PEFF_STATIC_LINK=1 IS_PEFF_BASE_BUILDING=1)
set_target_properties(peff_base_static PROPERTIES CXX_STANDARD 17)
target_sources(peff_base_static PRIVATE ${HEADERS} ${SRC})
target_include_directories(peff_base_static PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(peff_base_static PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
)

install(TARGETS peff_base peff_base_static
    EXPORT peffBaseTargets
    LIBRARY DESTINATION lib/
    ARCHIVE DESTINATION lib/
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/peff/base/
)
