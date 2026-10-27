// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLAY_UI_COMPONENT_EDITABLE_TEXT_UTILS_H_
#define CLAY_UI_COMPONENT_EDITABLE_TEXT_UTILS_H_

#include <algorithm>
#include <optional>
#include <string>

#include "clay/gfx/geometry/float_point.h"

namespace clay {

class TextUtils {
 public:
  /// Returns true iff the given value is a valid UTF-16 high (first) surrogate.
  /// The value must be a UTF-16 code unit, meaning it must be in the range
  /// 0x0000-0xFFFF.
  ///
  /// See also:
  ///   * https://en.wikipedia.org/wiki/UTF-16#Code_points_from_U+010000_to_U+10FFFF
  ///   * [isLowSurrogate], which checks the same thing for low (second)
  /// surrogates.
  static bool IsHighSurrogate(int value) { return (value & 0xFC00) == 0xD800; }

  /// Returns true iff the given value is a valid UTF-16 low (second) surrogate.
  /// The value must be a UTF-16 code unit, meaning it must be in the range
  /// 0x0000-0xFFFF.
  ///
  /// See also:
  ///   * https://en.wikipedia.org/wiki/UTF-16#Code_points_from_U+010000_to_U+10FFFF
  ///   * [isHighSurrogate], which checks the same thing for high (first)
  /// surrogates.
  static bool IsLowSurrogate(int value) { return (value & 0xFC00) == 0xDC00; }

  // Returns true iff the given value is a valid UTF-16 surrogate. The value
  // must be a UTF-16 code unit, meaning it must be in the range 0x0000-0xFFFF.
  //
  // See also:
  //   * https://en.wikipedia.org/wiki/UTF-16#Code_points_from_U+010000_to_U+10FFFF
  static bool IsUtf16Surrogate(uint32_t value) {
    return (value & 0xF800) == 0xD800;
  }

  // Returns true if the given position falls in the center of a surrogate pair.
  static bool BreaksSurrogatePair(const std::u16string& text, int position) {
    int length = text.length();
    return IsHighSurrogate(text.at(position - 1 >= length
                                       ? position - length - 1
                                       : std::max(0, position - 1))) &&
           IsLowSurrogate(text.at(position >= length ? position - length
                                                     : std::max(0, position)));
  }

  // Checks if the glyph is either [Unicode.RLM] or [Unicode.LRM]. These values
  // take up zero space and do not have valid bounding boxes around them.
  //
  // We do not directly use the [Unicode] constants since they are strings.
  static bool IsUnicodeDirectionality(uint32_t value) {
    return value == 0x200F || value == 0x200E;
  }

  static bool JudgeIfZWJCharacter(const std::u16string& text, int position,
                                  bool prev) {
    int length = text.length();
    if (text.empty() || position > length || position < 0) {
      return false;
    }
    if (prev && position > 0) {
      size_t prev_code_unit = text.at(std::max(0, position - 1));
      if (IsHighSurrogate(prev_code_unit) || IsLowSurrogate(prev_code_unit) ||
          text.at(std::min(std::max(0, position), length - 1)) ==
              TextUtils::kZWJUtf16 ||
          IsUnicodeDirectionality(prev_code_unit)) {
        return true;
      }
    } else if (position < length) {
      size_t next_code_unit = text.at(position);
      if (IsHighSurrogate(next_code_unit) || IsLowSurrogate(next_code_unit) ||
          text.at(std::min(std::max(0, position), length - 1)) ==
              TextUtils::kZWJUtf16 ||
          IsUnicodeDirectionality(next_code_unit)) {
        return true;
      }
    }
    return false;
  }

  static bool IsNewLine(size_t code_point) {
    // 0x000A Line Feed | 0x0085 New Line |0x000B Form Feed | 0x000C Vertical
    // Feed | 0x2028 Line Separator
    if (code_point == 0x000A || code_point == 0x0085 || code_point == 0x000B ||
        code_point == 0x000C || code_point == 0x2028 || code_point == 0x2029) {
      return true;
    } else {
      return false;
    }
  }

  // Unicode value for a zero width joiner character.
  static constexpr uint32_t kZWJUtf16 = 0x200d;
  static const char16_t kZeroWidthJoinerCharacter = u'\u200D';
  static const char16_t kZeroWidthSpaceCharacter = u'\u200B';
  static const char16_t kZeroWidthWordJoinerCharacter = u'\u2060';
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_TEXT_UTILS_H_
