cmake_minimum_required(VERSION 3.13)
project(phul VERSION 0.1.0)

include_directories("${PROJECT_SOURCE_DIR}")

include(${PROJECT_SOURCE_DIR}/cmake/PhulBuild.cmake)

add_subdirectory("phul")
add_subdirectory("example")
