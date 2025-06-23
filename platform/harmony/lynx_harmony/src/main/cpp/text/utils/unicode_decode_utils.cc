// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/unicode_decode_utils.h"

#include <utility>

#include "base/include/string/string_number_convert.h"

namespace lynx::tasm::harmony {

std::string UnicodeDecodeUtils::Decode(const std::string_view input_string,
                                       const UnicodeDecodeProperty property) {
  std::string output;
  const auto length = input_string.length();
  output.reserve(length);
  for (int i = 0; i < length; i++) {
    if (const auto c = input_string[i]; c == '&') {
      if (const auto end_pos = input_string.find(';', i);
          end_pos != std::string_view::npos) {
        if (i + 1 < length && input_string[i + 1] == '#') {
          int32_t unicode = -1;
          bool success = false;
          if (i + 2 < length && input_string[i + 2] == 'x' &&
              end_pos < i + 3 + 6) {
            // hex, max length = 6
            std::string hex_string(input_string.data() + i + 3,
                                   end_pos - i - 3);
            success = base::StringToInt(hex_string, &unicode, 16);
          } else if (end_pos < i + 2 + 7) {
            // decimal, max length = 7
            std::string decimal_string(input_string.data() + i + 2,
                                       end_pos - i - 2);
            success = base::StringToInt(decimal_string, &unicode, 10);
          }
          if (char unicode_buffer[5];
              success && unicode <= 0x10ffff && unicode >= 0 &&
              ConvertUnicode(unicode, unicode_buffer) > 0) {
            output.append(unicode_buffer);
            i = end_pos;
          } else {
            output.push_back(input_string[i]);
          }
        } else {
          // entity
          const auto entity_string =
              input_string.substr(i + 1, end_pos - i - 1);
          if (const auto* entity = DecodeEntity(entity_string);
              entity != nullptr) {
            output.append(entity);
            i = end_pos;
          } else {
            output.push_back(input_string[i]);
          }
        }
      }
    } else {
      output.push_back(input_string[i]);
    }
    if (i + 1 < length && IsCharStartByte(input_string[i + 1])) {
      // char end
      ProcessInsertChar(&output, property);
    }
  }
  return output;
}

int UnicodeDecodeUtils::ConvertUnicode(const uint32_t unicode, char* buffer) {
  int length = 0;
  if (unicode <= 0x7F) {
    *(buffer++) = unicode;
    length = 1;
  } else if (unicode <= 0x7FF) {
    *(buffer++) = (0xC0 | ((unicode >> 6u) & 0x1Fu));
    *(buffer++) = (0x80 | (unicode & 0x3Fu));
    length = 2;
  } else if (unicode <= 0xFFFF) {
    *(buffer++) = (0xE0 | ((unicode >> 12u) & 0x0Fu));
    *(buffer++) = (0x80 | ((unicode >> 6u) & 0x3Fu));
    *(buffer++) = (0x80 | (unicode & 0x3Fu));
    length = 3;
  } else if (unicode <= 0x10FFFF) {
    *(buffer++) = (0xF0 | ((unicode >> 18u) & 0x07u));
    *(buffer++) = (0x80 | ((unicode >> 12u) & 0x3Fu));
    *(buffer++) = (0x80 | ((unicode >> 6u) & 0x3Fu));
    *(buffer++) = (0x80 | (unicode & 0x3Fu));
    length = 4;
  }
  *buffer = 0;
  return length;
}

const char* UnicodeDecodeUtils::DecodeEntity(const std::string_view entity) {
  static constexpr std::pair<std::string_view, const char*> entity_map[] = {
      {"amp", "\u0026"},    {"nbsp", "\u00a0"},   {"ensp", "\u2002"},
      {"emsp", "\u2003"},   {"thinsp", "\u2009"}, {"zwnj", "\u200c"},
      {"zwj", "\u200d"},    {"lrm", "\u200e"},    {"rlm", "\u200f"},
      {"ndash", "\u2013"},  {"lsquo", "\u2018"},  {"rsquo", "\u2019"},
      {"sbquo", "\u201a"},  {"ldquo", "\u201c"},  {"rdquo", "\u201d"},
      {"bdquo", "\u201e"},  {"dagger", "\u2020"}, {"Dagger", "\u2021"},
      {"bull", "\u2022"},   {"hellip", "\u2026"}, {"permil", "\u2030"},
      {"prime", "\u2032"},  {"Prime", "\u2033"},  {"lsaquo", "\u2039"},
      {"rsquo", "\u203a"},  {"oline", "\u203e"},  {"frasl", "\u2044"},
      {"lt", "\u003c"},     {"gt", "\u003e"},     {"middot", "\u00b7"},
      {"mldr", "\u2026"},   {"cacute", "\u0107"}, {"quot", "\u0022"},
      {"amacr", "\u0101"},  {"caron", "\u02c7"},  {"emacr", "\u0113"},
      {"mdash", "\u2014"},  {"copy", "\u00a9"},   {"times", "\u00d7"},
      {"darr", "\u2193"},   {"imacr", "\u0121"},  {"iacute", "\u00ed"},
      {"igrave", "\u00ec"}, {"agrave", "\u00e0"}, {"ge", "\u2265"},
      {"le", "\u2264"},
  };
  for (const auto& [entity_string, unicode] : entity_map) {
    if (entity == entity_string) {
      return unicode;
    }
  }
  return nullptr;
}

bool UnicodeDecodeUtils::IsCJK(const uint32_t character) {
  return (character >= 0x4e00 &&
          character <= 0x9fff) || /* CJK Unified Ideographs */
         (character >= 0x3400 &&
          character <= 0x4dbf) || /* CJK Unified Ideographs Extention A */
         (character >= 0x20000 &&
          character <= 0x2a6df) || /* CJK Unified Ideographs Extention B */
         (character >= 0x2e80 &&
          character <= 0x2eff) || /* CJK Radicals Supplement */
         (character >= 0xf900 &&
          character <= 0xfaff) || /* CJK Compatibility Ideographs */
         (character >= 0x3040 &&
          character <= 0x30ff) || /* Hiragana and Katakana*/
         (character >= 0x31f0 &&
          character <= 0x31ff) || /* Katakana Phonetic Extensions */
         (character >= 0x1100 && character <= 0x11ff) || /* Hangul */
         (character > +0xac00 && character <= 0xd7a3) /* Hangul Syllables */;
}

bool UnicodeDecodeUtils::IsCharStartByte(const char c) {
  // 0xxxxxxx, 110xxxxx, 1110xxxx, 11110xxx
  return ((c & 0b1000'0000) == 0) || ((c & 0b1110'0000) == 0b1100'0000) ||
         ((c & 0b1111'0000) == 0b1110'0000) ||
         ((c & 0b1111'1000) == 0b1111'0000);
}

uint32_t UnicodeDecodeUtils::ConvertU8ToU32Char(const char* char_start,
                                                const int length,
                                                const char** end) {
  *end = nullptr;
  int char_len = 0;
  uint32_t cp;
  uint8_t nxt = char_start[char_len];
  if ((nxt & 0x80u) == 0) {
    cp = nxt & 0x7Fu;
    char_len = 1;
  } else if ((nxt & 0xE0u) == 0xC0) {
    cp = (nxt & 0x1Fu);
    char_len = 2;
  } else if ((nxt & 0xF0u) == 0xE0) {
    cp = nxt & 0x0Fu;
    char_len = 3;
  } else if ((nxt & 0xF8u) == 0xF0) {
    cp = nxt & 0x07u;
    char_len = 4;
  } else {
    return -1;
  }
  if (char_len > length) {
    return -1;
  }
  for (uint32_t j = 1; j < char_len; ++j) {
    nxt = char_start[j];
    if ((nxt & 0xC0u) != 0x80) {
      return -1;
    }
    cp <<= 6u;
    cp |= nxt & 0x3fu;
  }
  *end = char_start + char_len;
  return cp;
}

const char* UnicodeDecodeUtils::ReverseFindUnicodeCharFirstByte(
    const char* base, const int length) {
  for (int i = 0; i < length && i <= 4; i++) {
    const char c = base[-i];
    if (IsCharStartByte(c)) {
      return base - i;
    }
    if ((c & 0xC0u) != 0x80) {
      return nullptr;
    }
  }
  return nullptr;
}

void UnicodeDecodeUtils::ProcessInsertChar(
    std::string* output_ptr, const UnicodeDecodeProperty property) {
  auto& output = *output_ptr;
  if (property == UnicodeDecodeProperty::kInsertZeroWidthChar) {
    // append u200b after all char
    output.append("\u200b");
  } else if (property == UnicodeDecodeProperty::kCjkInsertWordJoiner) {
    // append u2060 between cjk char
    if (const auto* char_start_current = ReverseFindUnicodeCharFirstByte(
            output.c_str() + output.length() - 1, output.length());
        char_start_current != nullptr) {
      const int current_char_byte_length =
          output.c_str() + output.length() - char_start_current;
      const char* char_end_current;
      if (const auto unicode_current = ConvertU8ToU32Char(
              char_start_current, current_char_byte_length, &char_end_current);
          char_end_current != nullptr && IsCJK(unicode_current)) {
        if (const auto* char_start_before = ReverseFindUnicodeCharFirstByte(
                char_start_current - 1,
                char_start_current - output.c_str() - 1);
            char_start_before != nullptr) {
          const char* char_end_before;
          if (const auto unicode_before = ConvertU8ToU32Char(
                  char_start_before, char_start_current - char_start_before,
                  &char_end_before);
              char_end_before != nullptr && IsCJK(unicode_before)) {
            char buffer[5];
            memcpy(buffer, char_start_current, current_char_byte_length);
            buffer[current_char_byte_length] = '\0';
            output.erase(output.length() - current_char_byte_length,
                         current_char_byte_length);
            output.append("\u2060");
            output.append(buffer);
          }
        }
      }
    }
  }
}

}  // namespace lynx::tasm::harmony
