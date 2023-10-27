#include <stdio.h>
#include <string.h>
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
  TSHighlighter *hl =
      ts_highlighter_new(default_highlight_names, default_highlight_names, 18);
  ts_highlighter_add_language(hl, "C", "C", NULL, tree_sitter_c(),
                              query_highlights, NULL, NULL,
                              strlen(query_highlights), 0, 0, false);
  TSHighlightBuffer *buffer = ts_highlight_buffer_new();
  ts_highlighter_highlight(hl, "C", source_code, strlen(source_code), buffer,
                           false);
  int len = ts_highlight_buffer_len(buffer);
  int line_count = ts_highlight_buffer_line_count(buffer);
  char *data = ts_highlight_buffer_content(buffer);
  char highlight_source_code[4096] = {0};
  memcpy(highlight_source_code, data, len);
  highlight_source_code[len] = 0;
  printf("%s", highlight_source_code);
  return 0;
}