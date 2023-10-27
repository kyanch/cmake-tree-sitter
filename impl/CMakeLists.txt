# query转成头文件
function(query_to_header)
    set(oneValueArgs OUTPUT_DIR OUTPUT_NAME)
    cmake_parse_arguments(QUERY "" "${oneValueArgs}" "FILES" ${ARGN})
    # 判断 OUTPUT_NAME是否有定义
    if(NOT DEFINED QUERY_OUTPUT_NAME)
        set(QUERY_OUTPUT_NAME query.h)
    endif()
    message(STATUS "Query output:\t${QUERY_OUTPUT_DIR}/${QUERY_OUTPUT_NAME}")
    SET(TOTAL_QUERY_CONTENT "#pragma once\n")
    foreach(file IN LISTS QUERY_FILES)
        message(STATUS "Query process:\t${file}")
        get_filename_component(file_name ${file} NAME_WE)
        # message(STATUS "file name: ${file_name}")
        file(READ ${file} file_content)
        string(REPLACE "\"" "\\\"" file_content "${file_content}")
        string(REPLACE "\n" "\\n" file_content "${file_content}")
        set(TOTAL_QUERY_CONTENT "${TOTAL_QUERY_CONTENT}const char query_${file_name}[]={\"${file_content}\"};\n")
    endforeach(file in LISTS QUERY_FILES)
    # message(STATUS "${TOTAL_QUERY_CONTENT}")
    file(WRITE "${QUERY_OUTPUT_DIR}/${QUERY_OUTPUT_NAME}" "${TOTAL_QUERY_CONTENT}")
endfunction(query_to_header)

# 生成query到c_query.h
file(GLOB _QUERY_FILES c/queries/*.scm OUTPUT_DIR)
query_to_header(FILES ${_QUERY_FILES}
 OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/c/src/tree_sitter
 OUTPUT_NAME c_query.h)
# tree-sitter c implement
set(_TSC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/c/src)
file(GLOB _TSC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c)
add_library(tree-sitter-c STATIC ${_TSC_SOURCES})
target_include_directories(tree-sitter-c PUBLIC ${_TSC_HEADER})


# 生成query到c_query.h
file(GLOB _QUERY_FILES cpp/queries/*.scm OUTPUT_DIR)
query_to_header(FILES ${_QUERY_FILES}
 OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/tree_sitter
 OUTPUT_NAME cxx_query.h)
# tree-sitter cxx implement
set(_TSCXX_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src)
file(GLOB _TSCXX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/*.c)
add_library(tree-sitter-cxx STATIC ${_TSCXX_SOURCES})
target_include_directories(tree-sitter-cxx PUBLIC ${_TSCXX_HEADER})