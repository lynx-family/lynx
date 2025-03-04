// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/parser/input_stream.h"
#define TOKEN_NUM_PRINT_EACH_SIDE 8

namespace lynx {
namespace parser {
InputStream::InputStream() : cursor_(0), source_() {}

InputStream::~InputStream() {}

void InputStream::Write(const std::string& source) { source_.append(source); }

bool InputStream::HasNext() { return source_[cursor_] != 0; }

char InputStream::Next() { return HasNext() ? source_[cursor_++] : 0; }

void InputStream::Back() {
  if (cursor_ > 0) --cursor_;
}

void InputStream::Back(int step) {
  if (cursor_ > 0) {
    cursor_ -= step;
  }
}

// return source code around error position, use ' ' as seperator
std::string InputStream::GetPartStr(int32_t& line, int32_t& col) {
  int32_t line_index = 1, column_index = 0;
  size_t length = source_.length();

  while (line_index < line) {
    while (static_cast<size_t>(column_index) < length &&
           source_[column_index++] != '\n')
      ;
    line_index++;
  }
  // start: start position of the error line
  int32_t start = column_index;
  // error_column_index: position of the error
  int32_t error_column_index = column_index + col;
  while (static_cast<size_t>(column_index) < length &&
         source_[column_index++] != '\n')
    ;
  // end: end position of the error line
  int32_t end = column_index;

  int32_t token_number = 0;
  int32_t index = -1;
  // find ahead and after TOKEN_NUM_PRINT_EACH_SIDE token around error position
  for (index = error_column_index; index < end; index++) {
    if (source_[index] == ' ') {
      token_number++;
      if (token_number == TOKEN_NUM_PRINT_EACH_SIDE) {
        break;
      }
    }
  }
  int32_t end_pos = index;

  token_number = 0;
  for (index = error_column_index; index > start; index--) {
    if (source_[index] == ' ') {
      token_number++;
      if (token_number == TOKEN_NUM_PRINT_EACH_SIDE) {
        break;
      }
    }
  }
  int32_t start_pos = index;

  return source_.substr(start_pos, end_pos - start_pos);
}
}  // namespace parser
}  // namespace lynx
