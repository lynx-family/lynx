// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_SCANNER_H_
#define CORE_RUNTIME_VM_LEPUS_SCANNER_H_

#include <string>

#include "base/include/value/base_string.h"
#include "core/parser/input_stream.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/token.h"

namespace lynx {
namespace lepus {
class Scanner {
 public:
  explicit Scanner(parser::InputStream* input);
  void NextToken(Token& token, const Token& current_token);
  void SetSdkVersion(const std::string& sdk_version);

  int line() { return line_; }
  int column() { return column_; }
  std::string GetPartStr(Token& token) {
    return input_stream_->GetPartStr(token.line_, token.column_);
  }
  std::string GetPartStr(int32_t& line, int32_t& column) {
    return input_stream_->GetPartStr(line, column);
  }

 private:
  void ParseNewLine();
  void ParseSingleLineComment();
  void ParseMultiLineComment();
  void ParseNumber(Token& token);
  void ParseEqual(Token& token, int equal);
  void ParseTokenCharacter(Token& token, int token_character);
  void ParseString(Token& token);
  bool ParseRegExp(Token& token);
  void ParseId(Token& token);

  int EscapeConvert(char c);
  bool IsRegExpFlags(int current_character);

  int NextCharacter() {
    int character = input_stream_->Next();
    if (character != 0) {
      ++column_;
    } else {
      character = EOF;
    }
    return character;
  }

  void CharacterBack(int k) {
    if (k <= column_) {
      input_stream_->Back(k);
      column_ -= k;
    }
  }

  parser::InputStream* input_stream_;
  int current_character_ = EOF;
  Token current_token_;
  int line_ = 1;
  int column_ = 0;
  std::string sdk_version_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_SCANNER_H_
