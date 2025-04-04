cmake_minimum_required(VERSION 3.13)
project(peff VERSION 0.1.0)

include_directories(BEFORE SYSTEM ${PROJECT_SOURCE_DIR})

add_subdirectory("peff")
add_subdirectory("example")

#
# Check for AddressSanitizer if user has chosen to enable it for debugging.
#
if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(ENABLE_ADDRESS_SANITIZER TRUE CACHE BOOL "With AddressSanitizer")

        if(${ENABLE_ADDRESS_SANITIZER})
            message(CHECK_START "Finding AddressSanitizer")

            if(CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))
                # GCC/Clang
                message(CHECK_PASS "AddressSanitizer mode: GCC/Clang")
                set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address")
            elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
                message(CHECK_FAIL "You should enable AddressSanitizer manually in CMakeSettings.json")
            else()
                # Incompatible compiler
                message(CHECK_FAIL "No suitable AddressSanitizer mode for the compiler: \"${CMAKE_CXX_COMPILER_ID}\"")
            endif()
        endif()
    endif()
endif()

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
install(EXPORT peffAdvutilsTargets
    FILE peffAdvutilsTargets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
