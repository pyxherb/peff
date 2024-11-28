cmake_minimum_required(VERSION 3.13)
project(peff VERSION 0.1.0)

add_subdirectory("peff")
add_subdirectory("example")

# Generate the version file for the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/peffConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMinorVersion
)

# Create config file
configure_package_config_file(
    cmake/peffConfig.cmake.in peffConfig.cmake
    INSTALL_DESTINATION lib/cmake/peff
)

# Install config files
install(
    FILES   
       ${CMAKE_CURRENT_BINARY_DIR}/peffConfig.cmake
       ${CMAKE_CURRENT_BINARY_DIR}/peffConfigVersion.cmake
    DESTINATION
        lib/cmake/${PROJECT_NAME}
)

# Exporting Targets from the Build Tree
install(EXPORT peffBaseTargets
    FILE peffBaseTargets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
install(EXPORT peffUtilsTargets
    FILE peffUtilsTargets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
install(EXPORT peffContainersTargets
    FILE peffContainersTargets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
