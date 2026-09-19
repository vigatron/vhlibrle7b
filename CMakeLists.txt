cmake_minimum_required(VERSION 3.16)

#
project(vhlibrle7b VERSION 0.1.0 LANGUAGES CXX)

#
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version_vhlibrle7b.h"
)

#
add_library(vhlib_rle7b INTERFACE)

#
target_compile_features(vhlib_rle7b INTERFACE cxx_std_17)

#
target_sources(vhlib_rle7b INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhlibrle7binc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhlibrle7berrs.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhlibrle7bcrc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhlibrle7bmreg.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhlibrle7bmem.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhlibrle7bstrm.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vhlibrle7b.hpp
)

#
target_include_directories(vhlib_rle7b INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
