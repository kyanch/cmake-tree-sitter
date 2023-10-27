if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(${EXTRA_LIB_FOR_RUST} bcrypt.dll ntdll.dll WS2_32.dll advapi32.dll userenv.dll)
endif()
add_executable(tsh tsh.c)
target_link_libraries(tsh PRIVATE tree-sitter tree-sitter-c tree-sitter-cxx tree-sitter-highlight ${EXTRA_LIB_FOR_RUST})

enable_testing()
add_test(
    NAME highlight
    COMMAND tsh
)