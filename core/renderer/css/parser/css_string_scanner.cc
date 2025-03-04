// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/css_string_scanner.h"

#include <cstring>

#include "base/include/vector.h"

namespace lynx {
namespace tasm {

Token Scanner::ScanToken() {
  start_ = current_;

  if (IsAtEnd()) {
    return MakeToken(TokenType::TOKEN_EOF);
  }

  char c = Advance();

  if (IsWhitespace(c)) {
    return Whitespace();
  }

  if (IsAlpha(c)) {
    return IdentLikeToken();
  }
  // [<Number>] <dot> <Number>
  if (IsDigit(c) || (c == '.' && IsDigit(Peek()))) {
    return Numeric(c == '.');
  }

  // negative number
  if (c == '-' || c == '+') {
    if (IsDigit(Peek()) || (Peek() == '.' && IsDigit(PeekNext()))) {
      return Numeric();
    }
  }

  // hex number
  if (c == '#') {
    return Hex();
  }

  switch (c) {
    case '(':
      return MakeToken(TokenType::LEFT_PAREN);
    case ')':
      return MakeToken(TokenType::RIGHT_PAREN);
    case ',':
      return MakeToken(TokenType::COMMA);
    case ':':
      return MakeToken(TokenType::COLON);
    case '.':
      return MakeToken(TokenType::DOT);
    case '-':
      return MakeToken(TokenType::MINUS);
    case '+':
      return MakeToken(TokenType::PLUS);
    case ';':
      return MakeToken(TokenType::SEMICOLON);
    case '/':
      return MakeToken(TokenType::SLASH);
      // fixme(renzhongyue): all '#' are parsed to hex number now. This path is
      // unreachable.
    case '#':
      return MakeToken(TokenType::SHARP);
    case '%':
      return MakeToken(TokenType::PERCENTAGE);
    case '\'':
      return String('\'');
    case '\"':
      return String('\"');
    default:
      return MakeToken(TokenType::UNKNOWN);
  }
}

char Scanner::Advance() {
  current_++;
  return content_[current_ - 1];
}

bool Scanner::Match(char expected) {
  if (IsAtEnd()) {
    return false;
  }

  if (content_[current_] != expected) {
    return false;
  }

  current_++;
  return true;
}

bool Scanner::IsWhitespace(char c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f';
}

bool Scanner::IsDigit(char c) { return c >= '0' && c <= '9'; }

bool Scanner::IsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Scanner::IsNamed(char c) { return IsAlpha(c) || IsDigit(c) || c == '-'; }

Token Scanner::String(char boundary) {
  // skip first `'`
  Advance();
  while (Peek() != boundary && !IsAtEnd()) {
    Advance();
  }

  if (IsAtEnd()) {
    // Unterminated string without `'` at end , like 'xxxxx
    return ErrorToken();
  }
  // skip last `'`
  Advance();

  return MakeToken(TokenType::STRING);
}

Token Scanner::FunctionExpression(TokenType type) {
  char left_paren = '(';
  char right_paren = ')';

  // Save the `current_` value. If the token is not a function name who followed
  // by parenthesis, restore the `current_` to this value.
  uint32_t previous_current = current_;
  SkipWhiteSpace();
  if (Peek() != left_paren /* begin with ( */ || IsAtEnd()) {
    // The token is keyword but not a function. Should not skip the whitespaces.
    current_ = previous_current;
    return MakeToken(type);
  }
  base::InlineStack<TokenType, 16> brackets;
  brackets.push(TokenType::LEFT_PAREN);
  uint32_t args_start = current_;
  while (!IsAtEnd() && !brackets.empty()) {
    Advance();
    if (Peek() == left_paren) {
      brackets.push(TokenType::LEFT_PAREN);
    }
    if (Peek() == right_paren) {
      brackets.pop();
    }
  }
  if (!brackets.empty() || IsAtEnd()) {
    return Token{TokenType::ERROR, content_ + start_, current_ - start_};
  }
  Advance();
  // TODO(renzhongyue): all function type token should begin with the char next
  // to '('
  if (TokenType::BLUR <= type && type <= TokenType::GRAYSCALE) {
    return Token{type, content_ + args_start + 1, current_ - args_start - 2};
  }
  return MakeToken(type);
}

Token Scanner::Numeric(bool begin_with_dot) {
  // in case is negative number
  if ((Peek() == '-' || Peek() == '+') &&
      (IsDigit(PeekNext() || PeekNext() == '.'))) {
    Advance();
  }
  while (IsDigit(Peek())) {
    Advance();
  }
  if (begin_with_dot && Peek() == '.') {
    return MakeToken(TokenType::NUMBER);
  }
  // support float number
  if (Peek() == '.' && IsDigit(PeekNext())) {
    // consume the dot
    Advance();
  }

  while (IsDigit(Peek())) {
    Advance();
  }

  // <number> ,e , -, <number> like '3e-5'
  if (Peek() == 'e' && PeekNext() == '-' && IsDigit(PeekNextNext())) {
    Advance();  // e
    Advance();  // -
  }
  while (IsDigit(Peek())) {
    Advance();
  }

  Token number = MakeToken(TokenType::NUMBER);
  char p = Peek();
  // <percentage-token>, or <dimension-token>
  if (IsAlpha(p) || p == '%') {
    Token unit_token = ScanToken();
    return Token(TokenType::DIMENSION, unit_token.type, number.start,
                 number.length);
  }
  // <number-token>
  return number;
}

Token Scanner::Hex() {
  // skip #
  start_ = current_;
  while (IsAlpha(Peek()) || IsDigit(Peek())) {
    Advance();
  }

  return MakeToken(TokenType::HEX);
}

Token Scanner::Whitespace() {
  while (IsWhitespace(Peek())) {
    Advance();
  }

  return MakeToken(TokenType::WHITESPACE);
}

Token Scanner::IdentLikeToken() {
  while (IsNamed(Peek())) {
    Advance();
  }
  if (start_ > content_length_ || current_ > content_length_) {
    return MakeToken(TokenType::ERROR);
  }
  unsigned len = static_cast<unsigned>(current_ - start_);
  char s[len + 1];
  ToLower(content_ + start_, len, s);
  auto* it = GetTokenValue(s, len);
  if (it != nullptr /* got token keywords */) {
    if (TokenType::CALC <= it->type && it->type <= TokenType::GRAYSCALE) {
      // maybe function
      return FunctionExpression(it->type);
    }
    return MakeToken(it->type);
  }
  // not a keyword
  return MakeToken(TokenType::IDENTIFIER);
}

bool Scanner::IsAtEnd() const {
  return content_[current_] == '\0' || current_ >= content_length_;
}

Token Scanner::MakeToken(TokenType type) const {
  return Token{type, content_ + start_ + (type == TokenType::STRING ? 1 : 0),
               current_ - start_ - (type == TokenType::STRING ? 2 : 0)};
}

Token Scanner::ErrorToken() const {
  return Token(TokenType::ERROR, nullptr, 0);
}

char Scanner::Peek() const {
  if (current_ > content_length_) {
    return '\0';
  }
  return content_[current_];
}

char Scanner::PeekNext() const {
  if (IsAtEnd()) {
    return '\0';
  }

  return content_[current_ + 1];
}

char Scanner::PeekNextNext() const {
  if (PeekNext() == '\0' || current_ + 2 > content_length_) {
    return '\0';
  }

  return content_[current_ + 2];
}

void Scanner::SkipWhiteSpace() {
  for (;;) {
    char c = Peek();
    if (IsWhitespace(c)) {
      Advance();
    } else {
      return;
    }
  }
}
// Use uint32_t to improve efficiency, we can convert 4 chars together. Notice:
// '@' , '[' , '\' , ']' , '^' , '_' will lead to fault with this method.
bool Scanner::ToLower(const char* src, unsigned length, char* dst) {
  unsigned i;
  for (i = 0; i < (length & ~3); i += 4) {
    uint32_t x;
    memcpy(&x, src + i, sizeof(x));
    x |= (x & 0x40404040) >> 1;
    memcpy(dst + i, &x, sizeof(x));
  }
  for (; i < length; ++i) {
    char c = src[i];
    dst[i] = c | ((c & 0x40) >> 1);
  }
  dst[length] = '\0';
  return true;
}

}  // namespace tasm
}  // namespace lynx
