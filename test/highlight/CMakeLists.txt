add_executable(tsh tsh.c)
target_link_libraries(tsh private tree-sitter tree-sitter-c tree-sitter-cpp tree-sitter-highlight)

enable_testing()
add_test(
    NAME highlight
    COMMAND tsh
)