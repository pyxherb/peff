file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)

add_library(peff_utils SHARED)
target_compile_definitions(peff_utils PRIVATE PEFF_DYNAMIC_LINK=1 IS_PEFF_UTILS_BUILDING=1)
target_compile_definitions(peff_utils PUBLIC PEFF_DYNAMIC_LINK=1)
set_target_properties(peff_utils PROPERTIES CXX_STANDARD 17)
target_sources(peff_utils PRIVATE ${HEADERS} ${SRC})
target_link_libraries(peff_utils PUBLIC peff_base)
target_include_directories(peff_utils PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(peff_utils PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    ARCHIVE FALSE
)

add_library(peff_utils_static STATIC)
target_compile_definitions(peff_utils_static PRIVATE PEFF_STATIC_LINK=0 IS_PEFF_UTILS_BUILDING=1)
set_target_properties(peff_utils_static PROPERTIES CXX_STANDARD 17)
target_sources(peff_utils_static PRIVATE ${HEADERS} ${SRC})
target_link_libraries(peff_utils_static PUBLIC peff_base_static)
target_include_directories(peff_utils_static PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(peff_utils_static PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
)

install(TARGETS peff_utils peff_utils_static
    EXPORT peffUtilsTargets
    LIBRARY DESTINATION lib/
    ARCHIVE DESTINATION lib/
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/peff/utils/
)
