// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/scanner.h"

#include <map>
#include <utility>

#include "base/include/value/base_string.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/lepus_utils.h"

namespace lynx {
namespace lepus {
Scanner::Scanner(parser::InputStream* input)
    : input_stream_(input), sdk_version_("null") {}

void Scanner::ParseNewLine() {
  int next = NextCharacter();
  if (IsNewLine(next) && next != current_character_) {
    current_character_ = NextCharacter();
  } else {
    current_character_ = next;
  }
  ++line_;
  column_ = 0;
}

void Scanner::ParseSingleLineComment() {
  while (!IsNewLine(current_character_) && current_character_ != EOF) {
    current_character_ = NextCharacter();
  }
}

void Scanner::ParseMultiLineComment() {
  while (true) {
    current_character_ = NextCharacter();
    if (current_character_ == '*') {
      int next = NextCharacter();
      if (next == '/') {
        current_character_ = NextCharacter();
        break;
      } else {
        current_character_ = next;
        if (IsNewLine(current_character_)) {
          ++line_;
          column_ = 0;
        }
      }
    } else if (IsNewLine(current_character_)) {
      ++line_;
      column_ = 0;
    } else if (current_character_ == EOF) {
      // TODO Error
      break;
    }
  }
}

void Scanner::ParseNumber(Token& token) {
  std::string buffer;
  bool has_dot = false;
  while (true) {
    buffer.push_back(current_character_);
    current_character_ = NextCharacter();
    if (!IsDigit(current_character_) && !IsHex(current_character_) &&
        current_character_ != '.') {
      if (buffer.back() == 'e' &&
          (current_character_ == '+' || current_character_ == '-'))
        continue;
      break;
    } else if (current_character_ == '.') {
      if (!has_dot) {
        // for 123.123
        has_dot = true;
      } else {
        // for 123.123.toFixed
        break;
      }
    }
  }
  char* ptr;
  double ret;
  ret = strtod(buffer.c_str(), &ptr);
  if (ptr && *ptr != '\0') {
    throw CompileException("Invalid or unexpected token",
                           Token(line_, column_, Token_Number),
                           GetPartStr(line_, column_));
  } else {
    token = Token(line_, column_, Token_Number, ret);
  }
}

void Scanner::ParseEqual(Token& token, int equal) {
  int next = NextCharacter();
  if (next != '=') {
    token = Token(line_, column_, current_character_);
    current_character_ = next;
  } else {
    if (equal == Token_NotEqual || equal == Token_Equal) {
      int next_next = NextCharacter();
      if (next_next != '=') {
        token = Token(line_, column_, equal);
        current_character_ = next_next;
        return;
      }
    }

    if (equal == Token_NotEqual || equal == Token_Equal) {
      if (equal == Token_NotEqual) {
        equal = Token_AbsNotEqual;
      } else {
        equal = Token_AbsEqual;
      }
      token = Token(line_, column_, equal);
      current_character_ = NextCharacter();
    } else {
      token = Token(line_, column_, equal);
      current_character_ = NextCharacter();
    }
  }
}

void Scanner::ParseTokenCharacter(Token& token, int token_character) {
  if (token_character < Token_And) {
    token = Token(line_, column_, current_character_);
    current_character_ = token_character;
  } else {
    token = Token(line_, column_, token_character);
    current_character_ = NextCharacter();
  }
}

void Scanner::ParseString(Token& token) {
  int quote = current_character_;
  std::string buffer;
  while (true) {
    current_character_ = NextCharacter();
    if (current_character_ == '\\') {
      current_character_ = NextCharacter();
      if (current_character_ == '\n') continue;
      current_character_ = EscapeConvert(current_character_);
    } else if (current_character_ == quote || current_character_ == EOF)
      break;
    buffer.push_back(current_character_);
  }
  if (current_character_ != EOF) current_character_ = NextCharacter();
  token = Token(line_, column_, Token_String, std::move(buffer));
}

bool Scanner::IsRegExpFlags(int current_character) {
  int current_character_tmp = current_character;
  bool result = false;
  if (current_character_tmp == 'g' || current_character_tmp == 'i' ||
      current_character_tmp == 'm' || current_character_tmp == 's' ||
      current_character_tmp == 'u' || current_character_tmp == 'y') {
    result = true;
  }
  return result;
}

bool Scanner::ParseRegExp(Token& token) {
  CharacterBack(1);
  int quote = current_character_;  // '/'
  std::string pattern = "";
  std::string flags = "";
  bool in_class = false;
  bool handleEscape = false;

  // pattern
  while (true) {
    current_character_ = NextCharacter();

    if (current_character_ == '\n' || current_character_ == '\r') {
      throw CompileException("Wrong RegExp", current_token_,
                             GetPartStr(current_token_));
    }
    int next = NextCharacter();
    if (current_character_ == '\\' && next == '/') {
      handleEscape = true;
    }
    CharacterBack(1);
    if (current_character_ == '/') {
      if (!in_class && !handleEscape) {
        break;
      }
      if (handleEscape) {
        handleEscape = false;
      }
    } else if (current_character_ == '[') {
      in_class = true;
    } else if (current_character_ == ']') {
      in_class = false;
    } else if (current_character_ == '\\') {
    } else if (current_character_ == EOF) {
      break;
    }

    pattern.push_back(current_character_);
  }

  // flag
  if (current_character_ == quote) {
    while (true) {
      current_character_ = NextCharacter();
      if (IsRegExpFlags(current_character_)) {
        flags.push_back(current_character_);
      } else {
        break;
      }
    }
  }

  token =
      Token(line_, column_, Token_RegExp, std::move(pattern), std::move(flags));
  return true;
}

void Scanner::ParseId(Token& token) {
  if (!isalpha(current_character_) && current_character_ != '_' &&
      current_character_ != '$') {
    throw CompileException("invalid name", Token(line_, column_, Token_Id),
                           GetPartStr(line_, column_));
  }
  std::string buffer;
  buffer.push_back(current_character_);
  while ((current_character_ = NextCharacter()) &&
         (isalnum(current_character_) || current_character_ == '_' ||
          current_character_ == '$')) {
    buffer.push_back(current_character_);
  }

  int token_type = Token_EOF;
  if (!IsKeyWord(buffer, token_type)) {
    token = Token(line_, column_, Token_Id, std::move(buffer));
    return;
  }
  if (lynx::tasm::Config::IsHigherOrEqual(sdk_version_,
                                          FEATURE_CONTROL_VERSION_2) == false) {
    if (token_type == Token_Try || token_type == Token_Throw ||
        token_type == Token_Finally || token_type == Token_Catch) {
      token = Token(line_, column_, token_type, std::move(buffer));
      throw CompileException("The current Sdk Version is less than 1.4,",
                             "try catch not supported.", token,
                             GetPartStr(token));
    }
  }
  token = Token(line_, column_, token_type, std::move(buffer));
}

bool IsRegexpAllowed(Token token) {
  switch (token.token_) {
    case Token_Number:
    case Token_String:
    case Token_RegExp:
    case Token_DEC:
    case Token_INC:
    case Token_Nil:
    case Token_Undefined:
    case Token_False:
    case Token_True:
    case ')':
    case ']':
    case '}':
    case Token_Id:
      return false;
    default:
      return true;
  }
}

void Scanner::NextToken(Token& token, const Token& current_token) {
  if (current_character_ == EOF) {
    current_character_ = NextCharacter();
  }

  while (current_character_ != EOF) {
    if (IsWhitespace(current_character_)) {
      current_character_ = NextCharacter();

    } else if (IsNewLine(current_character_)) {
      ParseNewLine();
    } else if (IsDigit(current_character_)) {
      return ParseNumber(token);
    } else if (current_character_ == '/') {
      int next = NextCharacter();
      if (next == '/') {
        ParseSingleLineComment();
      } else if (next == '*') {
        ParseMultiLineComment();
      } else if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_DIV);
      } else {
        if (IsRegexpAllowed(current_token)) {
          ParseRegExp(token);
          return;
        } else {
          return ParseTokenCharacter(token, next);
        }
      }
    } else if (current_character_ == '+') {
      int next = NextCharacter();
      if (next == '+') {
        return ParseTokenCharacter(token, Token_INC);
      } else if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_ADD);
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '-') {
      int next = NextCharacter();
      if (next == '-') {
        return ParseTokenCharacter(token, Token_DEC);
      } else if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_SUB);
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '*') {
      int next = NextCharacter();
      if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_MUL);
      } else if (next == '*') {
        next = NextCharacter();
        if (next == '=') {
          return ParseTokenCharacter(token, Token_ASSIGN_Pow);
        } else {
          ParseTokenCharacter(token, Token_Pow);
          CharacterBack(1);
          current_character_ = next;
          return;
        }
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '%') {
      int next = NextCharacter();
      if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_MOD);
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '^') {
      int next = NextCharacter();
      if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_BIT_XOR);
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '|') {
      int next = NextCharacter();
      if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_BIT_OR);
      } else if (next == '|') {
        return ParseTokenCharacter(token, Token_Or);
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '&') {
      int next = NextCharacter();
      if (next == '=') {
        return ParseTokenCharacter(token, Token_ASSIGN_BIT_AND);
      } else if (next == '&') {
        return ParseTokenCharacter(token, Token_And);
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '?') {
      int next = NextCharacter();
      if (next == '.') {
        return ParseTokenCharacter(token, Token_Optional_Chaining);
      } else if (next == '?') {
        return ParseTokenCharacter(token, Token_Nullish_Coalescing);
      } else {
        return ParseTokenCharacter(token, next);
      }
    } else if (current_character_ == '~') {
      int next = NextCharacter();
      return ParseTokenCharacter(token, next);
    } else if (IsOtherToken(current_character_)) {
      return ParseTokenCharacter(token, NextCharacter());
    } else if (current_character_ == '!') {
      return ParseEqual(token, Token_NotEqual);
    } else if (current_character_ == '=') {
      return ParseEqual(token, Token_Equal);
    } else if (current_character_ == '>') {
      return ParseEqual(token, Token_GreaterEqual);
    } else if (current_character_ == '<') {
      return ParseEqual(token, Token_LessEqual);
    } else if (current_character_ == '"' || current_character_ == '\'') {
      return ParseString(token);
    } else {
      return ParseId(token);
    }
  }
  token = Token(line_, column_, Token_EOF);
}

int Scanner::EscapeConvert(char c) {
  std::map<char, int> cToAscII = {{'a', 7},   {'b', 8},   {'f', 12}, {'n', 10},
                                  {'r', 13},  {'t', 9},   {'v', 11}, {'\\', 92},
                                  {'\'', 39}, {'\"', 34}, {'?', 63}, {'0', 0}};
  std::map<char, int>::iterator it;
  it = cToAscII.find(c);
  if (it != cToAscII.end())
    return it->second;
  else
    return c;
}

void Scanner::SetSdkVersion(const std::string& sdk_version) {
  sdk_version_ = sdk_version;
}

}  // namespace lepus
}  // namespace lynx
