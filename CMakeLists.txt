cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

message(STATUS "${CMAKE_MODULE_PATH}")
project(tree-sitter VERSION 0.20.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_language(Rust)
include(CMakeCargo)

set(_TS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/lib/src ${CMAKE_CURRENT_SOURCE_DIR}/lib/include)
add_library(tree-sitter STATIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/src/lib.c)
target_include_directories(tree-sitter PUBLIC ${_TS_HEADER})

# tree-sitter highlight lib
# cargo_build( NAME tree-sitter-highlight 
#     SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/highlight
#     INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/highlight/include)

add_subdirectory(impl)