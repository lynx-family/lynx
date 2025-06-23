// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_UTILS_UNICODE_DECODE_UTILS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_UTILS_UNICODE_DECODE_UTILS_H_
#include <cstdint>
#include <string>
#include <string_view>

namespace lynx::tasm::harmony {
enum class UnicodeDecodeProperty : uint8_t {
  kDefault,
  kInsertZeroWidthChar,
  kCjkInsertWordJoiner,
};

class UnicodeDecodeUtils {
 public:
  static std::string Decode(
      std::string_view input_string,
      UnicodeDecodeProperty property = UnicodeDecodeProperty::kDefault);

 private:
  // buffer size >= 5, return length
  static int ConvertUnicode(uint32_t unicode, char* buffer);

  static const char* DecodeEntity(std::string_view entity);

  static bool IsCJK(uint32_t character);

  // find the start byte of current unicode char
  static const char* ReverseFindUnicodeCharFirstByte(const char* base,
                                                     int length);

  // convert current utf-8 char to unicode codepoint, *end = nullptr if invalid
  static uint32_t ConvertU8ToU32Char(const char* char_start, int length,
                                     const char** end);

  // is start byte of a unicode char: 0xxxxxxx, 110xxxxx, 1110xxxx, 11110xxx
  static bool IsCharStartByte(char c);

  static void ProcessInsertChar(std::string* output_ptr,
                                UnicodeDecodeProperty property);
};
}  // namespace lynx::tasm::harmony
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_UTILS_UNICODE_DECODE_UTILS_H_
