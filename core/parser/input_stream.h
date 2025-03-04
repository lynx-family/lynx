// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PARSER_INPUT_STREAM_H_
#define CORE_PARSER_INPUT_STREAM_H_

#include <string>

namespace lynx {
namespace parser {
class InputStream {
 public:
  InputStream();
  ~InputStream();
  void Write(const std::string& source);
  bool HasNext();
  char Next();
  void Back();
  void Back(int step);
  std::string GetPartStr(int32_t& line, int32_t& col);

 private:
  int cursor_;
  std::string source_;
};
}  // namespace parser
}  // namespace lynx

#endif  // CORE_PARSER_INPUT_STREAM_H_
