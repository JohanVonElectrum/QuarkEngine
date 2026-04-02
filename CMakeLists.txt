cmake_minimum_required(VERSION 4.1)
project(
    QuarkEngine
    DESCRIPTION "Quark Engine is a 3D voxel engine written in C."
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(quark)
add_subdirectory(testbed)
