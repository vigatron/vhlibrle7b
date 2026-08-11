cmake_minimum_required(VERSION 3.16)
project(vhlibrle7b VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version_vhlibrle7b.h"
)

add_library(vhlib_rle7b INTERFACE)

target_sources(vhlib_rle7b INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhrle7b.hpp
)

target_include_directories(vhlib_rle7b INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
