// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/string/string_utils.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstdarg>
#include <cstring>
#include <sstream>
#include <string>

#include "base/include/fml/macros.h"

// TODO(zhengsenyao): Uncomment LOG code when LOG available
// #include "base/include/log/logging.h"

namespace lynx {
namespace base {
const char kWhitespaceASCII[] = {0x09,  // CHARACTER TABULATION
                                 0x0A,  // LINE FEED (LF)
                                 0x0B,  // LINE TABULATION
                                 0x0C,  // FORM FEED (FF)
                                 0x0D,  // CARRIAGE RETURN (CR)
                                 0x20,  // SPACE
                                 0};
// NOLINT. This is replacement glyph which would be displayed as '�'.
const char16_t kReplacementU16 = u'\xFFFD';

const char* SplitString(
    std::string_view target, char separator, bool trim,
    const std::function<bool(const char*, size_t, int)>& callback) {
  if (target.empty()) {
    return target.data();
  }

  auto is_trimmed = [](char c) -> bool { return (c == ' ' || c == '\t'); };

  int index = 0;
  const char* s = target.data();
  const char* end = &target.back() + 1;
  if (trim) {
    while (s != end && is_trimmed(*s)) {
      ++s;
    }
  }
  const char* e = s;
  while (e != end) {
    e = s;
    while (e != end && *e != separator) {
      ++e;
    }
    if (e - s > 0) {
      long sc = 0;
      if (trim) {
        const char* b = e - 1;
        while (b >= s && is_trimmed(*b)) {
          --b;
          sc++;
        }
      }
      if (e - s - sc > 0) {
        if (!callback(s, e - s - sc, index++)) {
          return e;
        }
      }
    }

    if (e == end) {
      break;
    }

    s = e + 1;
    if (trim) {
      while (s != end && is_trimmed(*s)) {
        ++s;
      }
    }
  }
  return e;
}

bool SplitString(std::string_view target, char separator,
                 std::vector<std::string>& result) {
  size_t i = 0, len = target.length(), start = i;
  while (i < len) {
    bool is_last = i == len - 1;
    char current_char = target[i];
    if (current_char != separator) {
      if (is_last) {
        result.push_back(std::string(target.substr(start)));
      }
    } else {
      if (i == start) {
        start++;
      } else {
        result.push_back(std::string(target.substr(start, i - start)));
        start = i + 1;
      }
    }
    i++;
  }
  return !result.empty();
}

namespace {
template <class VECTOR_LIKE>
bool SplitStringBySpaceOutOfBrackets(std::string_view target,
                                     VECTOR_LIKE& result) {
  size_t i = 0, len = target.length(), start = i, bracket = 0;
  while (i < len) {
    bool is_last = i == len - 1;
    char current_char = target[i];
    if (current_char == '(') {
      ++bracket;
    } else if (current_char == ')') {
      --bracket;
    }
    if (bracket > 0 || !isspace(current_char)) {
      if (is_last) {
        result.push_back(std::string(target.substr(start)));
      }
    } else {
      if (i == start) {
        start++;
      } else {
        result.push_back(std::string(target.substr(start, i - start)));
        start = i + 1;
      }
    }
    i++;
  }
  return !result.empty();
}
}  // namespace

bool SplitStringBySpaceOutOfBrackets(std::string_view target,
                                     std::vector<std::string>& result) {
  return SplitStringBySpaceOutOfBrackets<decltype(result)>(target, result);
}

bool SplitStringBySpaceOutOfBrackets(
    std::string_view target, base::InlineVector<std::string, 4>& result) {
  return SplitStringBySpaceOutOfBrackets<decltype(result)>(target, result);
}

std::vector<std::string_view> SplitToStringViews(std::string_view str,
                                                 const std::string& delims) {
  std::vector<std::string_view> output;

  for (auto first = str.data(), second = str.data(), last = first + str.size();
       second != last && first != last; first = second + 1) {
    second =
        std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

    if (first != second) {
      output.emplace_back(first, second - first);
    }
  }

  return output;
}

std::string Join(const std::vector<std::string>& vec, const char* delim) {
  std::stringstream res;
  for (size_t i = 0; i < vec.size(); ++i) {
    res << vec[i];
    if (i < vec.size() - 1) {
      res << delim;
    }
  }
  return res.str();
}

std::string JoinString(const std::vector<std::string>& pieces) {
  std::string joined;
  for (const auto& piece : pieces) {
    joined += piece + " ";
  }
  return joined;
}

bool EndsWithIgnoreSourceCase(std::string_view s, std::string_view ending) {
  return EndsWith(StringToLowerASCII(s), ending);
}

// flexDirection => flex-direction
// backgroundColor => background-color
// width => width
// line-height => line-height
std::string CamelCaseToDashCase(std::string_view camel_case_property) {
  std::string dash_case_property;
  // "A" -> "-a" 2
  // The upper bound of length of responding dash_case_property
  // is 2 times the original one
  dash_case_property.reserve(camel_case_property.length() * 2);

  for (const auto& c : camel_case_property) {
    if (c >= 'A' && c <= 'Z') {
      dash_case_property.push_back('-');
      dash_case_property.push_back(std::tolower(c));
    } else {
      dash_case_property.push_back(c);
    }
  }

  return dash_case_property;
}

void TrimWhitespaceASCII(std::string_view input, int position,
                         std::string* output) {
  size_t first_good_char = std::string::npos;
  for (char c : kWhitespaceASCII) {
    size_t pos = input.find_first_not_of(c, position);
    if (pos == std::string::npos) continue;
    if (first_good_char == std::string::npos) first_good_char = pos;
    first_good_char = first_good_char < pos ? pos : first_good_char;
  }

  size_t last_good_char = input.size();
  for (char c : kWhitespaceASCII) {
    size_t pos = input.find_last_not_of(c, position);
    if (pos == std::string::npos) continue;
    last_good_char = last_good_char > pos ? last_good_char : pos;
  }

  if (input.empty() || first_good_char == std::string::npos ||
      last_good_char == std::string::npos) {
    output->clear();
    return;
  }

  *output = input.substr(first_good_char, last_good_char - first_good_char + 1);
  return;
}

std::string StringToLowerASCII(std::string_view input) {
  std::string output;
  output.reserve(input.size());
  for (char i : input) {
    if (i >= 'A' && i <= 'Z') {
      output.push_back(i - ('A' - 'a'));
    } else {
      output.push_back(i);
    }
  }
  return output;
}

//
// Uses for trim blank around string off.
//    " aa "     =>   "aa"
//    " a  a "   =>   "a  a"
//
std::string TrimString(std::string_view str) {
  if (str.empty()) return "";
  size_t length = str.size();
  uint32_t front_space_count = 0;
  uint32_t back_space_count = 0;
  uint32_t total_space_count = 0;
  while (front_space_count < length && str[front_space_count] == ' ') {
    front_space_count++;
    break;
  }
  while (front_space_count + back_space_count < length &&
         str[length - back_space_count - 1] == ' ') {
    back_space_count++;
    break;
  }
  total_space_count = front_space_count + back_space_count;
  return std::string(str.substr(front_space_count, length - total_space_count));
}

std::string_view TrimToStringView(std::string_view to_trim) {
  auto left = to_trim.begin();
  for (;; ++left) {
    if (left == to_trim.end()) {
      return std::string_view();
    }
    if (!std::isspace(*left)) {
      break;
    }
  }
  auto right = to_trim.end() - 1;
  for (; right > left && isspace(*right); --right) {
  }
  return std::string_view(to_trim.data() + std::distance(to_trim.begin(), left),
                          std::distance(left, right) + 1);
}

template <typename T, typename CharT = typename T::value_type>
T TrimStringT(T input, T trim_chars, TrimPositions positions) {
  size_t begin =
      (positions & TRIM_LEADING) ? input.find_first_not_of(trim_chars) : 0;
  size_t end = (positions & TRIM_TRAILING)
                   ? input.find_last_not_of(trim_chars) + 1
                   : input.size();
  return input.substr(std::min(begin, input.size()), end - begin);
}

std::string TrimString(std::string input, std::string trim_chars,
                       TrimPositions positions) {
  return TrimStringT(input, trim_chars, positions);
}
std::string_view TrimString(std::string_view input, std::string_view trim_chars,
                            TrimPositions positions) {
  return TrimStringT(input, trim_chars, positions);
}

//
// Splits string by pattern in the char vector and following the order in
// vector. Won't spilt wrap by '', () or "" as string.
//
// "color: white; font-size: 100" => {"color", " white", " font-size", " 100"}
// "color:white; :font-size:100" => {"color", " white"}
// "color:white;:;width:100" => {"color", "white", "", "", "width","100"}
// "width: 200px; height: 200px;background-image: url('https://xxxx.jpg');"
// "width: 200px; height: 200px;background-image: url(https://xxxx.jpg);"
base::InlineVector<std::string, 32> SplitStringByCharsOrderly(
    std::string_view str, char cs[], size_t char_count) {
  const char* byte_array = str.data();
  const size_t size = str.size();
  base::InlineVector<std::string, 32> result;
  if (!size || char_count == 0) {
    result.push_back(std::string(str));
    return result;
  }
  char characters[256];
  memset(&characters[0], 0, 256);
  for (size_t i = 0; i < char_count; i++) {
    char c = cs[i];
    switch (c) {
      case '{':
      case '}':
      case '(':
      case ')':
      case '\"':
      case '\'': {
        return result;  // Guarantee NRVO. Do not return '{}'.
      }
      default:
        break;
    }
    characters[static_cast<int>(c)] = 1;
  }

  std::string value;
  int word_start = -1;
  uint32_t word_count = 0;
  bool word_produced = false;
  int order = 0;
  base::InlineVector<std::string, 8> grouper;
  bool is_variable = false;
  bool is_string = false;
  int end_char = -1;
  for (int i = 0; static_cast<size_t>(i) < size; ++i) {
    char c = byte_array[i];
    if (!is_variable && !is_string && cs[order % char_count] == c) {
      word_produced = true;
      order++;
    } else if (!is_variable && !is_string && characters[static_cast<int>(c)]) {
      // restart
      order = 0;
      word_start = -1;
      word_count = 0;
      grouper.clear();
    } else {
      if (c == '{') {
        is_variable = true;
      } else if (is_variable && c == '}') {
        is_variable = false;
      }
      if ((c == '\'' || c == '\"' || c == '(') && !is_string) {
        is_string = true;
        if (c == '(') {
          end_char = ')';
        } else {
          end_char = c;
        }
      } else if (is_string && c == end_char) {
        is_string = false;
        end_char = -1;
      }
      word_start = word_start == -1 ? i : word_start;
      word_count++;
    }
    if (word_produced || (static_cast<size_t>(i) == size - 1 && word_count)) {
      // consumed
      if (word_count && word_start >= 0 &&
          static_cast<size_t>(word_start) + static_cast<size_t>(word_count) <=
              size) {
        grouper.emplace_back(byte_array, static_cast<size_t>(word_start),
                             static_cast<size_t>(word_count));
      } else {
        grouper.emplace_back();
      }
      word_start = -1;
      word_count = 0;

      if (grouper.size() == char_count) {
        // output
        for (auto& s : grouper) {
          result.emplace_back(std::move(s));
        }
      }

      if (order % char_count == 0) {
        grouper.clear();
      }

      word_produced = false;
    }
  }
  return result;
}

void ReplaceAll(std::string& subject, std::string_view search,
                std::string_view replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
}

std::string SafeStringConvert(const char* str) {
  return str == nullptr ? std::string() : str;
}

std::string PtrToStr(void* ptr) {
  // TODO(heshan):use std::to_chars instead when support c++17
  char temp[20]{0};
  // for hexadecimal, begin with 0x
#ifndef __EMSCRIPTEN__
  std::snprintf(temp, sizeof(temp), "0x%" PRIxPTR,
                reinterpret_cast<std::uintptr_t>(ptr));
#else
  // emcc not qualify cpp standard, use uint32 for PRIxPTR in 64bit...
  // so use zx instead...
  std::snprintf(temp, sizeof(temp), "0x%zx",
                reinterpret_cast<std::uintptr_t>(ptr));
#endif
  return temp;
}

// (1,2, 3,4) ==> vector:{1,2,3,4}
bool ConvertParenthesesStringToVector(std::string& origin,
                                      std::vector<std::string>& ret,
                                      char separator) {
  origin.erase(remove_if(origin.begin(), origin.end(), isspace), origin.end());
  auto start = origin.find("(");
  auto end = origin.find(")");
  if (start >= end) {
    return false;
  }
  origin = origin.substr(start + 1, end - start - 1);
  return base::SplitString(origin, separator, ret);
}

std::vector<std::string> SplitStringIgnoreBracket(std::string str,
                                                  char delimiter) {
  int start = 0;
  std::vector<std::string> result;
  bool has_bracket = false;
  for (int i = 0; static_cast<size_t>(i) < str.size(); i++) {
    if (str[i] == delimiter) {
      if (has_bracket) {
        continue;
      } else {
        if (i > start) {
          result.push_back(TrimString(str.substr(start, i - start)));
        }
        start = i + 1;
      }
    } else if (str[i] == '(') {
      has_bracket = true;
    } else if (str[i] == ')') {
      has_bracket = false;
    }
  }
  if (static_cast<size_t>(start) < str.size()) {
    result.push_back(TrimString(str.substr(start, str.size() - start)));
  }
  return result;
}

bool BothAreSpaces(char lhs, char rhs) {
  return (lhs == rhs) && (isspace(lhs));
}

std::string RemoveSpaces(std::string_view str) {
  std::stringstream ss;
  for (size_t i = 0; i < str.length(); ++i) {
    if (!std::isspace(str[i])) {
      ss << str[i];
    }
  }
  return ss.str();
}

// "a b    c  d   " => "a b c d "
void ReplaceMultiSpaceWithOne(std::string& str) {
  std::string::iterator new_end =
      std::unique(str.begin(), str.end(), &BothAreSpaces);
  str.erase(new_end, str.end());
}

// if \n, \r, \t in \"\", exec the following replace actions
// '\n' => "\n"
// '\r' => "\r"
// '\t' => "\t"
// "\"a\"" => "\"a\""
//  "\"a\nb\"" => "\"a\\nb\""
// "( x? \"a\" : \"b\")" => "( x? \"a\" : \"b\")"
// "( x ? \n \"a\" : \n\"b\")" => "( x ? \n \"a\" : \n\"b\")"
// "( x ? \n\"a \nc\": \n\"b\"" => "( x ? \n\"a \\nc\": \n\"b\""
// "( x ? \n a : \n b)" => "( x ? \n a : \n b)"
void ReplaceEscapeCharacterWithLiteralString(std::string& input) {
  int newline_count = 0;
  int double_quotes_count = 0;
  bool pre_is_escape = false;
  for (auto c : input) {
    if (c == '\\') {
      pre_is_escape = true;
      continue;
    }
    if (!pre_is_escape && c == '\"') {
      ++double_quotes_count;
    }
    if (c == '\r' || c == '\n' || c == '\t') {
      if (double_quotes_count % 2 == 1) {
        ++newline_count;
      }
    }
    pre_is_escape = false;
  }
  size_t len = input.size();
  input.resize(len + newline_count);
  size_t left = len - 1;
  size_t right = input.size() - 1;
  auto insert_back_slash = [](std::string& str, size_t& index) {
    --index;
    str[index] = '\\';
  };

  double_quotes_count = 0;
  while (left < right) {
    if (input[left] == '\"') {
      if (left == 0 || input[left - 1] != '\\') {
        ++double_quotes_count;
      }
    }
    if (double_quotes_count % 2 == 0) {
      input[right] = input[left];
      --right;
      --left;
      continue;
    }
    if (input[left] == '\n') {
      input[right] = 'n';
      insert_back_slash(input, right);
    } else if (input[left] == '\r') {
      input[right] = 'r';
      insert_back_slash(input, right);
    } else if (input[left] == '\t') {
      input[right] = 't';
      insert_back_slash(input, right);
    } else {
      input[right] = input[left];
    }
    --right;
    --left;
  }
}

std::u16string U8StringToU16(std::string_view u8_string) {
  std::u32string u32str = U8StringToU32(u8_string);
  return U32StringToU16(u32str);
}

std::string U16StringToU8(std::u16string_view u16_string) {
  std::u32string u32_string = U16StringToU32(u16_string);
  return U32StringToU8(u32_string);
}

std::u32string U8StringToU32(std::string_view u8_string) {
  size_t length = u8_string.length();

  std::u32string u32str;
  uint32_t i = 0;

  while (i < length) {
    uint32_t utf32 = static_cast<unsigned char>(u8_string[i]);
    int additional_bytes = 0;

    if ((utf32 & 0x80u) == 0) {
      additional_bytes = 0;
    } else if ((utf32 & 0xE0u) == 0xC0) {
      utf32 &= 0x1Fu;
      additional_bytes = 1;
    } else if ((utf32 & 0xF0u) == 0xE0) {
      utf32 &= 0x0Fu;
      additional_bytes = 2;
    } else if ((utf32 & 0xF8u) == 0xF0) {
      utf32 &= 0x07u;
      additional_bytes = 3;
    } else {
      // TODO(zhengsenyao): Uncomment LOG code when LOG available
      // LOGE("Invalid UTF-8 encoding");
      return U"";
    }

    for (int j = 0; j < additional_bytes; ++j) {
      if (++i >= length) {
        // TODO(zhengsenyao): Uncomment LOG code when LOG available
        // LOGE("Invalid UTF-8 encoding");
        return U"";
      }

      auto byte = static_cast<unsigned char>(u8_string[i]);
      if ((byte & 0xC0u) != 0x80) {
        // TODO(zhengsenyao): Uncomment LOG code when LOG available
        // LOGE("Invalid UTF-8 encoding");
        return U"";
      }

      utf32 = (utf32 << 6u) | (byte & 0x3Fu);
    }

    u32str.push_back(utf32);
    ++i;
  }

  return u32str;
}

std::string U32StringToU8(std::u32string_view u32_string) {
  size_t length = u32_string.length();

  std::string utf8;

  for (uint32_t i = 0; i < length; ++i) {
    auto utf32 = u32_string[i];

    if (utf32 <= 0x7F) {
      utf8.push_back(utf32);
    } else if (utf32 <= 0x7FF) {
      utf8.push_back(0xC0 | ((utf32 >> 6u) & 0x1Fu));
      utf8.push_back(0x80 | (utf32 & 0x3Fu));
    } else if (utf32 <= 0xFFFF) {
      utf8.push_back(0xE0 | ((utf32 >> 12u) & 0x0Fu));
      utf8.push_back(0x80 | ((utf32 >> 6u) & 0x3Fu));
      utf8.push_back(0x80 | (utf32 & 0x3Fu));
    } else if (utf32 <= 0x10FFFF) {
      utf8.push_back(0xF0 | ((utf32 >> 18u) & 0x07u));
      utf8.push_back(0x80 | ((utf32 >> 12u) & 0x3Fu));
      utf8.push_back(0x80 | ((utf32 >> 6u) & 0x3Fu));
      utf8.push_back(0x80 | (utf32 & 0x3Fu));
    } else {
      // TODO(zhengsenyao): Uncomment LOG code when LOG available
      // LOGE("U32StringToU8 convert error");
      return "";
    }
  }

  return utf8;
}

std::u32string U16StringToU32(std::u16string_view u16_string) {
  size_t length = u16_string.length();

  std::u32string u32str;

  for (uint32_t i = 0; i < length;) {
    uint32_t utf32 = u16_string[i];

    if (utf32 >= 0xD800 && utf32 <= 0xDBFF) {
      if (i + 1 < length) {
        uint32_t low_surrogate = u16_string[i + 1];
        if (low_surrogate >= 0xDC00 && low_surrogate <= 0xDFFF) {
          utf32 = ((utf32 - 0xD800) << 10) + (low_surrogate - 0xDC00) + 0x10000;
          i += 2;
        } else {
          // TODO(zhengsenyao): Uncomment LOG code when LOG available
          // LOGE("Invalid UTF-16 encoding");
        }
      } else {
        // TODO(zhengsenyao): Uncomment LOG code when LOG available
        // LOGE("Invalid UTF-16 encoding");
      }
    } else {
      i++;
    }

    u32str.push_back(utf32);
  }

  return u32str;
}

std::u16string U32StringToU16(std::u32string_view u32_string) {
  size_t length = u32_string.length();

  std::u16string u16str;

  for (uint32_t i = 0; i < length; ++i) {
    uint32_t utf32 = u32_string[i];

    if (utf32 <= 0xFFFF) {
      u16str.push_back(static_cast<char16_t>(utf32));
    } else if (utf32 >= 0x10000 && utf32 <= 0x10FFFF) {
      utf32 -= 0x10000;
      u16str.push_back(static_cast<char16_t>(0xD800 | ((utf32 >> 10) & 0x3FF)));
      u16str.push_back(static_cast<char16_t>(0xDC00 | (utf32 & 0x3FF)));
    } else {
      // TODO(zhengsenyao): Uncomment LOG code when LOG available
      // LOGE("U32StringToU16 Invalid UTF-32 encoding");
      return u"";
    }
  }

  return u16str;
}

bool IsValidUtf8Bytes(const unsigned char* p, int count) {
  for (int i = 0; i < count; ++i) {
    uint8_t c = p[++i];
    if ((c & 0xC0) != 0x80) {
      return false;
    }
  }
  return true;
}

std::string Utf16LeToUtf8(const char16_t* u16str, size_t length) {
  if (length <= 0) {
    return "";
  }
  const char16_t* p = u16str;
  std::u16string::size_type len = length;
  if (p[0] == 0xFEFF) {
    p += 1;
    len -= 1;
  }

  std::string u8str;
  u8str.reserve(len * 3);

  char16_t u16char;
  for (std::u16string::size_type i = 0; i < len; ++i) {
    // Assume little-endian
    u16char = p[i];

    // 1 utf8 code units
    if (u16char < 0x0080) {
      // u16char <= 0x007f
      // U- 0000 0000 ~ 0000 07ff : 0xxx xxxx
      u8str.push_back(static_cast<char>(u16char & 0x00FF));
      continue;
    }
    // 2 utf8 code units
    if (u16char >= 0x0080 && u16char <= 0x07FF) {
      // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
      u8str.push_back(static_cast<char>(((u16char >> 6) & 0x1F) | 0xC0));
      u8str.push_back(static_cast<char>((u16char & 0x3F) | 0x80));
      continue;
    }
    // Surrogate pair
    if (u16char >= 0xD800 && u16char <= 0xDBFF) {
      if (i + 1 >= len) {
        // TODO(yulitao): Need to add an unknown glyph?
        // FML_LOG(ERROR) << "incomplete utf16 encoding.";
        continue;
      }
      // * U-00010000 - U-001FFFFF: 1111 0xxx 10xxxxxx 10xxxxxx 10xxxxxx
      uint32_t highSur = u16char;
      uint32_t lowSur = p[++i];
      // From surrogate pair to unicode codepoint
      // 1. Deal with high surrogate(minus 0xD800, with effective 10bit left)
      // 2. Deal with low surrogate(minus 0xDC00, with effective 10bit left)
      // 3. Plus 0x10000, results in unicode codepoint.
      uint32_t codePoint = highSur - 0xD800;
      codePoint <<= 10;
      codePoint |= lowSur - 0xDC00;
      codePoint += 0x10000;
      // Encode to utf8 with 4 bytes.
      u8str.push_back(static_cast<char>((codePoint >> 18) | 0xF0));
      u8str.push_back(static_cast<char>(((codePoint >> 12) & 0x3F) | 0x80));
      u8str.push_back(static_cast<char>(((codePoint >> 06) & 0x3F) | 0x80));
      u8str.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
      continue;
    }
    // 3 utf8 code units
    {
      // * U-0000E000 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
      u8str.push_back(static_cast<char>(((u16char >> 12) & 0x0F) | 0xE0));
      u8str.push_back(static_cast<char>(((u16char >> 6) & 0x3F) | 0x80));
      u8str.push_back(static_cast<char>((u16char & 0x3F) | 0x80));
      continue;
    }
  }

  return u8str;
}

std::string Utf16ToUtf8(const char16_t* u16str, size_t length) {
  if (length <= 0) {
    return "";
  }
  // Byte Order Mark
  char16_t bom = u16str[0];
  switch (bom) {
    case 0xFFFE:  // Big Endian
      LYNX_BASE_DCHECK(false);
      // << "UTF16 with big endian is not supported!";
      return "";
    case 0xFEFF:  // Little Endian
    default:
      // Default to little endian
      return Utf16LeToUtf8(u16str, length);
  }
}

std::u16string Utf8ToUtf16(const char* u8str, size_t length, bool addbom,
                           bool* ok) {
  std::u16string u16str;
  u16str.reserve(length);
  if (addbom) {
    u16str.push_back(0xFEFF);  // bom (FF FE)
  }
  std::string::size_type len = length;

  const unsigned char* p = (const unsigned char*)(u8str);
  if (len > 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
    p += 3;
    len -= 3;
  }

  bool is_ok = true;
  for (std::string::size_type i = 0; i < len; ++i) {
    uint32_t ch = p[i];
    if ((ch & 0x80) == 0) {
      // 1 byte encoding
      u16str.push_back(static_cast<char16_t>(ch));
      continue;
    }

    if ((ch & 0xF8) == 0xF0) {  // 4 bytes encoding
      if (i + 3 >= length || !IsValidUtf8Bytes(p + i, 3)) {
        u16str.push_back(kReplacementU16);
        continue;
      }
      uint32_t c2 = p[++i];
      uint32_t c3 = p[++i];
      uint32_t c4 = p[++i];
      // Take lower 3 bits of first byte and lower 6 bits for remaining bytes
      // to compose the unicode.
      uint32_t codePoint = ((ch & 0x07U) << 18) | ((c2 & 0x3FU) << 12) |
                           ((c3 & 0x3FU) << 6) | (c4 & 0x3FU);
      if (codePoint >= 0x10000) {
        // In UTF-16, codepoint U+10000 to U+10FFFF should be represented by 2
        // code units, which is called surrogate pair.
        // 1、Decrease codepoint by 0x10000 (with 20bits remaining)
        // 2、Increase the higher 10bits by 0xD800 to retrieve high surrogate.
        // 3、Increase the lower 10bits by 0xDC00 to retrieve low surrogate.
        codePoint -= 0x10000;
        u16str.push_back(static_cast<char16_t>((codePoint >> 10) | 0xD800U));
        u16str.push_back(
            static_cast<char16_t>((codePoint & 0x03FFU) | 0xDC00U));
      } else {
        // [U+0000, U+D7FF] / [U+E000, U+FFFF] in utf16 is same with unicode.
        // [U+D800, U+DFFF] is nonexists.
        u16str.push_back(static_cast<char16_t>(codePoint));
      }
    } else if ((ch & 0xF0) == 0xE0) {  // 3 bytes encoding, 0x800 to 0xFFFF.
      if (i + 2 >= length || !IsValidUtf8Bytes(p + i, 2)) {
        u16str.push_back(kReplacementU16);
        continue;
      }
      uint32_t c2 = p[++i];
      uint32_t c3 = p[++i];
      // Compose unicode codepoint by retrieve lower 4bits of first byte and
      // lower 6 bits of remaining bytes.
      uint32_t codePoint =
          ((ch & 0x0FU) << 12) | ((c2 & 0x3FU) << 6) | (c3 & 0x3FU);
      if (!IsInUtf16BMP(codePoint)) {
        u16str.push_back(kReplacementU16);
        continue;
      }
      u16str.push_back(static_cast<char16_t>(codePoint));
    } else if ((ch & 0xE0) == 0xC0) {
      if (i + 1 >= length || !IsValidUtf8Bytes(p + i, 1)) {
        u16str.push_back(kReplacementU16);
        continue;
      }
      uint32_t c2 = p[++i];
      // Compose unicode codepoint by retrieve lower 5bits of first byte and
      // lower 6 bits of remaining bytes.
      uint32_t codePoint = ((ch & 0x1F) << 6u) | (c2 & 0x3F);
      u16str.push_back(static_cast<char16_t>(codePoint));
    } else {
      u16str.push_back(kReplacementU16);
      is_ok = false;
    }
  }

  if (ok != NULL) {
    *ok = is_ok;
  }

  return u16str;
}

std::string FormatStringWithVaList(const char* format, va_list args) {
  int length, size = 100;
  char* mes = nullptr;
  if ((mes = static_cast<char*>(malloc(size * sizeof(char)))) == nullptr) {
    return "";
  }
  while (true) {
    va_list copy_args;
    va_copy(copy_args, args);
    length = vsnprintf(mes, size, format, copy_args);
    va_end(copy_args);
    if (length > -1 && length < size) break;
    size *= 2;
    char* clone = static_cast<char*>(realloc(mes, size * sizeof(char)));
    if (clone == nullptr) {
      break;
    } else {
      mes = clone;
      clone = nullptr;
    }
  }
  std::string message = mes;
  free(mes);
  mes = nullptr;
  return message;
}

std::string FormatString(const char* format, ...) {
  std::string error_msg;
  va_list args;
  va_start(args, format);
  error_msg = FormatStringWithVaList(format, args);
  va_end(args);
  return error_msg;
}

bool EqualsIgnoreCase(std::string_view left, std::string_view right) {
  auto left_lower = StringToLowerASCII(left);
  auto right_lower = StringToLowerASCII(right);

  return left_lower == right_lower;
}

}  // namespace base
}  // namespace lynx
