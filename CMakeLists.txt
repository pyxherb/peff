cmake_minimum_required(VERSION 3.13)
project(peff VERSION 0.1.0)

include_directories("${PROJECT_SOURCE_DIR}")

include(${PROJECT_SOURCE_DIR}/cmake/PeffBuild.cmake)

add_subdirectory("peff")
add_subdirectory("example")
