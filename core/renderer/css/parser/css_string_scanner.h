// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_PARSER_CSS_STRING_SCANNER_H_
#define CORE_RENDERER_CSS_PARSER_CSS_STRING_SCANNER_H_

#include <map>
#include <string>

#include "core/renderer/css/css_keywords.h"

namespace lynx {
namespace tasm {

struct Token final {
  TokenType type = TokenType::TOKEN_EOF;
  TokenType unit = TokenType::UNKNOWN;
  const char* start = nullptr;
  uint32_t length = 0;

  bool IsZero() {
    return type == TokenType::NUMBER && length == 1 && start[0] == '0';
  }

  bool IsIdent() {
    return type == TokenType::IDENTIFIER || type >= TokenType::UNKNOWN;
  }

  Token() = default;
  Token(TokenType type, const char* start, uint32_t length)
      : Token(type, TokenType::UNKNOWN, start, length) {}

  Token(TokenType type, TokenType unit, const char* start, uint32_t length)
      : type(type), unit(unit), start(start), length(length) {}

  Token& operator=(const Token&) = default;
  ~Token() = default;
};

class Scanner final {
 public:
  Scanner(const char* content, uint32_t content_length)
      : content_(content), content_length_(content_length) {}

  ~Scanner() = default;

  Token ScanToken();

  char Advance();
  bool IsAtEnd() const;
  Token MakeToken(TokenType type) const;
  Token ErrorToken() const;
  Token String(char boundary);

  Token Numeric(bool begin_with_dot = false);
  Token Hex();
  Token Whitespace();
  bool Match(char expected);
  char Peek() const;
  char PeekNext() const;
  char PeekNextNext() const;
  void SkipWhiteSpace();
  const char* content() const { return content_; }

 private:
  static bool IsWhitespace(char c);
  static bool IsDigit(char c);
  static bool IsAlpha(char c);
  static bool IsNamed(char c);
  static bool ToLower(const char* src, unsigned length, char* dst);
  Token IdentLikeToken();
  Token FunctionExpression(TokenType type);

  const char* content_;
  uint32_t content_length_;
  uint32_t start_{0};
  uint32_t current_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_PARSER_CSS_STRING_SCANNER_H_
