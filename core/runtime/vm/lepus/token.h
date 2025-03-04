// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_TOKEN_H_
#define CORE_RUNTIME_VM_LEPUS_TOKEN_H_

#include <iostream>
#include <utility>

#include "base/include/value/base_string.h"

namespace lynx {
namespace lepus {
enum TokenType {
  Token_And = 256,
  Token_Break = 257,
  Token_Do = 258,
  Token_Else = 259,
  Token_Elseif = 260,
  Token_End = 261,
  Token_False = 262,
  Token_For = 263,
  Token_Function = 264,
  Token_If = 265,
  Token_In = 266,
  Token_Var = 267,
  Token_Nil = 268,
  Token_Not = 269,
  Token_Or = 270,
  Token_Return = 271,
  Token_Switch = 272,
  Token_Case = 273,
  Token_Default = 274,
  Token_True = 275,
  Token_Until = 276,
  Token_While = 277,
  Token_Id = 278,
  Token_String = 279,
  Token_Number = 280,
  Token_Equal = 281,
  Token_NotEqual = 282,
  Token_LessEqual = 283,
  Token_GreaterEqual = 284,
  Token_INC = 285,             // ++
  Token_DEC = 286,             // --
  Token_ASSIGN_BIT_OR = 287,   // |=
  Token_ASSIGN_BIT_XOR = 288,  // ^=
  Token_ASSIGN_BIT_AND = 289,  // &=
  Token_ASSIGN_SHL = 290,      // <<=
  Token_ASSIGN_SAR = 291,      // >>>=
  Token_ASSIGN_SHR = 292,      // >>>=
  Token_ASSIGN_ADD = 293,      // +=
  Token_ASSIGN_SUB = 294,      // -=
  Token_ASSIGN_MUL = 295,      // *=
  Token_ASSIGN_DIV = 296,      // /=
  Token_ASSIGN_MOD = 297,      // %=
  Token_EOF = 298,
  Token_Continue = 299,
  Token_Try = 300,
  Token_Catch = 301,
  Token_Finally = 302,
  Token_Throw = 303,
  Token_ASSIGN_Pow = 304,  // **=
  Token_Pow,               //**
  Token_Typeof,
  Token_RegExp,
  Token_AbsNotEqual,
  Token_AbsEqual,
  Token_Undefined,
  Token_Import,
  Token_Export,
  Token_From,
  Token_Optional_Chaining,  // ?.
  Token_Nullish_Coalescing  // ??
};

struct Token {
  double number_;
  base::String str_;
  base::String pattern_;
  base::String flags_;

  //        base::String* module_;
  int line_;
  int column_;
  int token_;  // id & some character

  Token() : line_(0), column_(0), token_(Token_EOF) {}

  Token(int line, int column, int token)
      : line_(line), column_(column), token_(token) {}

  Token(int line, int column, int token, double number)
      : number_(number), line_(line), column_(column), token_(token) {}

  Token(int line, int column, int token, base::String str)
      : str_(std::move(str)), line_(line), column_(column), token_(token) {}

  Token(int line, int column, int token, base::String pattern,
        base::String flags)
      : pattern_(std::move(pattern)),
        flags_(std::move(flags)),
        line_(line),
        column_(column),
        token_(token) {}

  Token(const Token& token) { Copy(token); }

  ~Token() {}

  Token& operator=(const Token& token) {
    Copy(token);
    return *this;
  }

  bool inline IsString() const {
    return token_ == Token_String || token_ == Token_Id;
  }
  static bool inline IsObjectKey(int token) {
    return token == Token_Id || token == Token_String || token == Token_Break ||
           token == Token_Do || token == Token_If || token == Token_Else ||
           token == Token_Elseif || token == Token_False ||
           token == Token_True || token == Token_Function ||
           token == Token_For || token == Token_Var || token == Token_Nil ||
           token == Token_While || token == Token_Switch ||
           token == Token_Undefined || token == Token_Case ||
           token == Token_Default || token == Token_Return ||
           token == Token_Continue || token == Token_Finally ||
           token == Token_Try || token == Token_Throw || token == Token_Catch ||
           token == Token_Until || token == Token_Number;
  }

 private:
  void Copy(const Token& token) {
    str_ = base::String();

    if (token.token_ == Token_Number) {
      number_ = token.number_;
    } else if (IsObjectKey(token.token_)) {
      str_ = token.str_;
    }
    token_ = token.token_;
    line_ = token.line_;
    column_ = token.column_;
    if (token.token_ == Token_RegExp) {
      pattern_ = token.pattern_;
      flags_ = token.flags_;
    }
  }
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_TOKEN_H_
