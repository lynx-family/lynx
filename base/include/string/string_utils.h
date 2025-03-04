// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_STRING_STRING_UTILS_H_
#define BASE_INCLUDE_STRING_STRING_UTILS_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/vector.h"

namespace lynx {
namespace base {

using UChar = char16_t;
using LChar = char;
using UChar32 = int32_t;

enum TrimPositions {
  TRIM_NONE = 0,
  TRIM_LEADING = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL = TRIM_LEADING | TRIM_TRAILING,
};

inline bool BeginsWith(std::string_view s, std::string_view begin) {
  if (s.length() >= begin.length()) {
    return (0 == s.compare(0, begin.length(), begin));
  } else {
    return false;
  }
}

// Callback returns pointer, length and index of each splitted string.
// If separator is not space and trim is true, the returned string will be
// automatically trimmed.
// Return pointer to last character processed.
// Return false of callback to terminate processing.
const char* SplitString(
    std::string_view target, char separator, bool trim,
    const std::function<bool(const char*, size_t, int)>& callback);

bool SplitString(std::string_view target, char separator,
                 std::vector<std::string>& result);

bool SplitStringBySpaceOutOfBrackets(std::string_view target,
                                     std::vector<std::string>& result);

// For CSS style handlers with maximum 4 components.
bool SplitStringBySpaceOutOfBrackets(
    std::string_view target, base::InlineVector<std::string, 4>& result);

template <typename OutputStringType, typename T>
std::vector<OutputStringType> SplitString(T str, T delims,
                                          bool want_all = false) {
  std::vector<OutputStringType> output;

  for (auto first = str.data(), second = str.data(), last = first + str.size();
       second != last && first != last; first = second + 1) {
    second =
        std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

    if (want_all || first != second) {
      output.emplace_back(first, second);
    }
  }

  return output;
}

std::vector<std::string_view> SplitToStringViews(
    std::string_view str, const std::string& delims = " ");

// Returns a string joined by the given delimiter.
std::string Join(const std::vector<std::string>& vec, const char* delimiter);

std::string JoinString(const std::vector<std::string>& pieces);

std::string CamelCaseToDashCase(std::string_view);

inline bool EndsWith(std::string_view s, std::string_view ending) {
  if (s.length() >= ending.length()) {
    return (0 ==
            s.compare(s.length() - ending.length(), ending.length(), ending));
  } else {
    return false;
  }
}

bool EndsWithIgnoreSourceCase(std::string_view s, std::string_view ending);

void TrimWhitespaceASCII(std::string_view input, int position,
                         std::string* output);
std::string StringToLowerASCII(std::string_view input);

namespace internal {
template <class T>
void AppendStringImpl(std::stringstream& ss, const T& head) {
  ss << head;
}

template <class T, class... Args>
void AppendStringImpl(std::stringstream& ss, const T& head,
                      const Args&... args) {
  ss << head;
  AppendStringImpl(ss, args...);
}
}  // namespace internal

template <class... Args>
std::string AppendString(const Args&... args) {
  if constexpr (sizeof...(Args) > 0) {
    std::stringstream ss;
    internal::AppendStringImpl(ss, args...);
    return ss.str();
  }
  return {};
}

// String utils
std::string TrimString(std::string_view str);
std::string_view TrimToStringView(std::string_view to_trim);
std::string TrimString(std::string input, std::string trim_chars,
                       TrimPositions positions);
std::string_view TrimString(std::string_view input, std::string_view trim_chars,
                            TrimPositions positions);

base::InlineVector<std::string, 32> SplitStringByCharsOrderly(
    std::string_view str, char cs[], size_t char_count);

// Use like this `SplitStringByCharsOrderly<':', ';'>(input);`
template <char... chars>
auto SplitStringByCharsOrderly(std::string_view str) {
  char cs[] = {chars...};
  return SplitStringByCharsOrderly(str, cs, sizeof(cs));
}

void ReplaceAll(std::string& str, std::string_view from, std::string_view to);

inline std::string_view TruncateToStringView(const std::string& input,
                                             size_t max_length) {
  return std::string_view(input.c_str(), std::max(input.length(), max_length));
}

std::string SafeStringConvert(const char* str);

std::string PtrToStr(void* ptr);

// (1,2, 3,4) ==> vector:{1,2,3,4}
bool ConvertParenthesesStringToVector(std::string& origin,
                                      std::vector<std::string>& ret,
                                      char separator = ',');
// delimiter=",": "a,b,(1,2,3),d" =>[a,b,(1,2,3),d]
std::vector<std::string> SplitStringIgnoreBracket(std::string str,
                                                  char delimiter);

// Remove all spaces (https://www.cplusplus.com/reference/cctype/isspace/) from
// `str`.
std::string RemoveSpaces(std::string_view str);
// this method will modify input str.
// "a b    c  d   " => "a b c d "
void ReplaceMultiSpaceWithOne(std::string& str);

// The purpose of this function is to replace \n, \r, and \t in \"\" with \\n,
// \\r, and \\t, respectively, to avoid lepusNG generating code cache failure.
// Now, this function is only used in the encoder.
void ReplaceEscapeCharacterWithLiteralString(std::string& input);

inline bool IsInUtf16BMP(char16_t c) { return (c & 0xF800) != 0xD800; }

inline bool IsLeadingSurrogate(char16_t c) { return c >= 0xD800 && c < 0xDC00; }

inline bool IsTrailingSurrogate(char16_t c) {
  return c >= 0xDC00 && c <= 0xDFFF;
}

inline bool IsUtf8Start(char c) {
  // Start with "11" or "0X", which all consists UTF8 encode.
  return (c & 0xc0) != 0x80;
}

std::u16string U8StringToU16(std::string_view u8_string);
std::string U16StringToU8(std::u16string_view u16_string);
std::u32string U8StringToU32(std::string_view u8_string);
std::string U32StringToU8(std::u32string_view u32_string);
std::u32string U16StringToU32(std::u16string_view u16_string);
std::u16string U32StringToU16(std::u32string_view u32_string);

// The following functions implementation is different from the above
// implementation. It considers the LE/BE and has some special character
// replacement. Auto judge LE or BE. Default to LE.
std::string Utf16ToUtf8(const char16_t* u16str, size_t length);
inline std::string Utf16ToUtf8(const std::u16string& u16str) {
  return Utf16ToUtf8(u16str.c_str(), u16str.length());
}
// Convert to utf16-le
std::u16string Utf8ToUtf16(const char* u8str, size_t length, bool addbom,
                           bool* ok);

inline std::u16string Utf8ToUtf16(const std::string& str) {
  return Utf8ToUtf16(str.c_str(), str.length(), false, nullptr);
}

std::string FormatStringWithVaList(const char* format, va_list args);
std::string FormatString(const char* format, ...);

inline bool StringEqual(const char* a, const char* b) {
  assert(a != nullptr && b != nullptr);
  auto la = strlen(a);
  if (la != strlen(b)) return false;
  return strncmp(a, b, la) == 0;
}

bool EqualsIgnoreCase(std::string_view left, std::string_view right);

// Refer from
// https://chromium.googlesource.com/v8/v8/+/master/src/inspector/v8-string-conversions.cc

constexpr bool IsASCII(UChar c) { return !(c & ~0x7F); }

constexpr bool IsASCIINumber(UChar c) { return c >= '0' && c <= '9'; }

constexpr bool IsASCIIHexNumber(UChar c) {
  return ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') || IsASCIINumber(c);
}

constexpr int ToASCIIHexValue(UChar c) {
  return c < 'A' ? c - '0' : (c - 'A' + 10) & 0xF;
}

constexpr bool IsASCIIAlphaCaselessEqual(UChar css_character, char character) {
  // This function compares a (preferably) constant ASCII
  // lowercase letter to any input character.
  LYNX_BASE_DCHECK(character >= 'a');
  LYNX_BASE_DCHECK(character <= 'z');
  return (css_character | 0x20) == character;
}

constexpr bool IsASCIISpace(UChar c) {
  return c <= ' ' && (c == ' ' || (c <= 0xD && c >= 0x9));
}

template <typename CharType>
constexpr bool IsHTMLSpace(CharType character) {
  return character <= ' ' &&
         (character == ' ' || character == '\n' || character == '\t' ||
          character == '\r' || character == '\f');
}

constexpr bool IsSpaceOrNewline(UChar c) {
  return IsASCII(c) && c <= ' ' && (c == ' ' || (c <= 0xD && c >= 0x9));
}

inline size_t InlineUTF8SequenceLengthNonASCII(char b0) {
  if ((b0 & 0xC0) != 0xC0) return 0;
  if ((b0 & 0xE0) == 0xC0) return 2;
  if ((b0 & 0xF0) == 0xE0) return 3;
  if ((b0 & 0xF8) == 0xF0) return 4;
  return 0;
}

inline size_t InlineUTF8SequenceLength(char b0) {
  return IsASCII(b0) ? 1 : InlineUTF8SequenceLengthNonASCII(b0);
}

inline size_t InlineUTF8SequenceLength(const char* utf8, size_t c_length,
                                       size_t utf8_index) {
  size_t cur_utf8_index = 0;
  size_t cur_index = 0;
  while (cur_utf8_index != utf8_index && cur_index < c_length) {
    cur_index += InlineUTF8SequenceLength(utf8[cur_index]);
    cur_utf8_index++;
  }
  return cur_index;
}

inline size_t UTF8IndexToCIndex(const char* utf8, size_t c_length,
                                size_t utf8_index) {
  size_t cur_utf8_index = 0;
  size_t cur_index = 0;
  while (cur_utf8_index != utf8_index && cur_index < c_length) {
    cur_index += InlineUTF8SequenceLength(utf8[cur_index]);
    cur_utf8_index++;
  }
  return cur_index;
}

inline size_t Utf8IndexToCIndexForUtf16(const char* utf8, size_t c_length,
                                        size_t utf16_index) {
  size_t cur_utf16_index = 0, cur_c_index = 0, cur_char_size = 0;

  while (cur_utf16_index < utf16_index && cur_c_index < c_length) {
    cur_char_size = InlineUTF8SequenceLength(utf8[cur_c_index]);
    cur_c_index += cur_char_size;
    // if cur_char_size == 4, the char's length is 2 in utf16.
    cur_utf16_index += (cur_char_size == 4 ? 2 : 1);
  }

  if (cur_utf16_index > utf16_index) {
    return cur_c_index - cur_char_size;
  }
  return cur_c_index;
}

inline size_t CIndexToUTF8Index(const char* utf8, size_t c_length,
                                size_t c_index) {
  size_t cur_c_index = 0;
  size_t cur_utf8_index = 0;
  while (cur_c_index < c_index && cur_c_index < c_length) {
    cur_c_index += InlineUTF8SequenceLength(utf8[cur_c_index]);
    cur_utf8_index++;
  }
  return cur_utf8_index;
}

inline size_t SizeOfUtf8(const char* utf8, size_t c_length) {
  size_t size = 0;
  size_t cur_index = 0;
  while (cur_index < c_length) {
    cur_index += InlineUTF8SequenceLength(utf8[cur_index]);
    size++;
  }
  return size;
}

inline size_t SizeOfUtf16(const std::string& src_u8) {
  std::u16string u16_conv = lynx::base::U8StringToU16(src_u8);
  return u16_conv.size();
}

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_STRING_STRING_UTILS_H_
