add_subdirectory("base")
add_subdirectory("utils")
add_subdirectory("containers")
add_subdirectory("advutils")

set(PEFF_ENABLE_RCOBJ_DEBUGGING FALSE CACHE STRING "Control if to enable debugging for reference-counting objects.")

configure_file(
    config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/peff/generated)
