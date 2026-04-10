add_subdirectory("base")
add_subdirectory("utils")
add_subdirectory("containers")
add_subdirectory("advutils")

if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(_PEFF_ENABLE_ASSERTION TRUE)
    else()
        set(_PEFF_ENABLE_ASSERTION FALSE)
    endif()
else()
    set(_PEFF_ENABLE_ASSERTION FALSE)
endif()

set(PEFF_ENABLE_ASSERTION ${_PEFF_ENABLE_ASSERTION} CACHE STRING "Control if to enable debugging for reference-counting objects.")
set(PEFF_ENABLE_RCOBJ_DEBUGGING FALSE CACHE STRING "Control if to enable debugging for reference-counting objects.")

configure_file(
    config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/peff/generated)
