cmake_minimum_required(VERSION 3.5)
project(blocksci)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(BlockSci_VERSION 0.7.0)

include(GNUInstallDirs)

# Docker builds need portable CPU instructions, while local Release builds keep
# the historical host-tuned defaults for performance.
option(BLOCKSCI_PORTABLE_BUILD "Disable host-specific CPU optimizations" OFF)

add_subdirectory(external)

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(benchmark)
add_subdirectory(test)
add_subdirectory(example)
