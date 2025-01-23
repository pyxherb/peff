file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)

add_library(peff_containers SHARED)
target_compile_definitions(peff_containers PRIVATE PEFF_DYNAMIC_LINK=1 IS_PEFF_CONTAINERS_BUILDING=1)
set_target_properties(peff_containers PROPERTIES CXX_STANDARD 17)
target_sources(peff_containers PRIVATE ${HEADERS} ${SRC})
target_link_libraries(peff_containers PUBLIC peff_base peff_utils)
target_include_directories(peff_containers PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(peff_containers PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    ARCHIVE FALSE
)

add_library(peff_containers_static STATIC)
target_compile_definitions(peff_containers_static PRIVATE PEFF_STATIC_LINK=1 IS_PEFF_CONTAINERS_BUILDING=1)
set_target_properties(peff_containers_static PROPERTIES CXX_STANDARD 17)
target_sources(peff_containers_static PRIVATE ${HEADERS} ${SRC})
target_link_libraries(peff_containers_static PUBLIC peff_base_static peff_utils_static)
target_include_directories(peff_containers_static PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(peff_containers_static PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
)

install(TARGETS peff_containers peff_containers_static
    EXPORT peffContainersTargets
    LIBRARY DESTINATION lib/
    ARCHIVE DESTINATION lib/
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/peff/containers/
)
