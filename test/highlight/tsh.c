#include <stdio.h>
#include <tree_sitter/api.h>
#include <tree_sitter/c_query.h>
#include <tree_sitter/highlight.h>
#include <tree_sitter/parser.h>

#pragma once
TSLanguage *tree_sitter_c();
TSLanguage *tree_sitter_cpp();

int main() {
  FILE *f = NULL;
  char source_code[2048] = {0};
  f = fopen(__FILE__, "r");
  fread(source_code, 2048, 1, f);
  fclose(f);
  printf("%s", source_code);
  ts_highlighter_new(const char **highlight_names,
                     const char **attribute_strings, uint32_t highlight_count)

      return 0;
}