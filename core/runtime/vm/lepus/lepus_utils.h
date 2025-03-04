// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_UTILS_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_UTILS_H_

#include <map>
#include <string>

#include "core/runtime/vm/lepus/token.h"

namespace lynx {
namespace lepus {
static inline bool IsWhitespace(int c) {
  return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

static inline bool IsNewLine(int c) { return c == '\r' || c == '\n'; }

static inline bool IsDigit(int c) { return c >= '0' && c <= '9'; }

static inline bool IsHex(int c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

static inline bool IsOtherToken(int c) {
  return c == '#' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' ||
         c == ']' || c == ']' || c == ';' || c == ':' || c == ',' || c == '.' ||
         c == '?';
}

const static std::map<std::string, int> kKeyWords = {
    {"break", Token_Break},     {"do", Token_Do},
    {"if", Token_If},           {"else", Token_Else},
    {"elseif", Token_Elseif},   {"false", Token_False},
    {"true", Token_True},       {"function", Token_Function},
    {"for", Token_For},         {"let", Token_Var},
    {"null", Token_Nil},        {"undefined", Token_Undefined},
    {"while", Token_While},     {"switch", Token_Switch},
    {"case", Token_Case},       {"default", Token_Default},
    {"return", Token_Return},   {"continue", Token_Continue},
    {"try", Token_Try},         {"catch", Token_Catch},
    {"finally", Token_Finally}, {"throw", Token_Throw},
    {"typeof", Token_Typeof},   {"import", Token_Import},
    {"export", Token_Export},   {"var", Token_Var}};
static inline bool IsKeyWord(const std::string& word, int& token) {
  auto iter = kKeyWords.find(word);
  if (iter != kKeyWords.end()) {
    token = iter->second;
    return true;
  }
  return false;
}

static inline bool IsPrimaryExpr(int token) {
  return token == Token_Nil || token == Token_False || token == Token_True ||
         token == Token_Number || token == Token_String ||
         token == Token_Function || token == Token_Id || token == '(' ||
         token == '{' || token == Token_DEC || token == Token_INC ||
         token == '[' || token == Token_RegExp || token == Token_Undefined;
}
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_LEPUS_UTILS_H_
