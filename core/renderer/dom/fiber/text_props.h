// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_TEXT_PROPS_H_
#define CORE_RENDERER_DOM_FIBER_TEXT_PROPS_H_
#include <optional>

#include "base/include/value/base_string.h"

namespace lynx {
namespace tasm {
using starlight::VerticalAlignType;
constexpr const char* kInlinePlaceHolder = "I";
enum TextPropertyKeyID {
  kPropInlineStart = 0,
  kPropInlineEnd = 1,
  kPropTextString = 2,

  // styles
  kTextPropFontSize = 3,
  kTextPropColor = 4,
  kTextPropWhiteSpace = 5,
  kTextPropTextOverflow = 6,
  kTextPropFontWeight = 7,
  kTextPropFontStyle = 8,
  kTextPropFontFamily = 9,
  kTextPropLineHeight = 10,
  kTextPropLetterSpacing = 11,
  kTextPropLineSpacing = 12,
  kTextPropTextShadow = 13,
  kTextPropTextDecoration = 14,
  kTextPropTextAlign = 15,
  kTextPropVerticalAlign = 16,

  // attributes
  kTextPropTextMaxLine = 99,
  kTextPropBackGroundColor = 100,
  kPropImageSrc = 101,  // image
  kPropInlineViewSign = 102,
  kPropRectSize = 103,
  kPropMargin = 104,
  kPropBorderRadius = 105,
  kPropColorLinearGradient = 106,
  kPropColorRadialGradient = 107,
  kPropColorConicGradient = 108,

  kTextPropEnd = 0xFF,
};
struct TextProps {
  enum class WhiteSpace { NO_WRAP = 0, NORMAL = 1 };

  enum class TextOverflow { CLIP = 0, ELLIPSIS = 1 };

  enum class Typeface { NORMAL = 0, BOLD = 1, ITALIC = 2 };

  enum class TextAlign { LEFT = 0, CENTER = 1, RIGHT = 2 };

  // attributes
  std::optional<int> text_max_line;        //=-1
  std::optional<base::String> image_mode;  //""
};

// A lookup table to determine the number of UTF-16 code units contributed
// by each possible UTF-8 byte value (0-255).
// This table is the core of a highly efficient algorithm for calculating the
// UTF-16 length of a UTF-8 string, mirroring Java's `String.length()`.
//
// The values in the table mean:
//   - 1: This byte is the start of a character that will become a single
//        UTF-16 code unit. This applies to:
//        - ASCII characters (0x00 - 0x7F).
//        - The start byte of a 2-byte sequence (0xC2 - 0xDF).
//        - The start byte of a 3-byte sequence (0xE0 - 0xEF).
//
//   - 2: This byte is the start of a character that will become a surrogate
//   pair
//        (two UTF-16 code units). This applies to:
//        - The start byte of a 4-byte sequence (0xF0 - 0xF4), representing
//          characters in Unicode's supplementary planes (e.g., many emojis).
//
//   - 0: This byte does not contribute to the length count. This applies to:
//        - Continuation bytes (0x80 - 0xBF), as their length is already
//        accounted
//          for by the corresponding start byte.
//        - Invalid or overlong UTF-8 start bytes (e.g., 0xC0, 0xC1, 0xF5-0xFF),
//          which are treated as errors and do not contribute to the valid
//          length.

static const uint8_t kUtf8ToUtf16Units[256] = {
    // 1-byte sequences (ASCII), contribute 1 UTF-16 unit
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x00-0x0F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x10-0x1F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x20-0x2F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x30-0x3F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x40-0x4F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x50-0x5F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x60-0x6F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x70-0x7F

    // Continuation bytes, contribute 0 UTF-16 units
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80-0x8F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90-0x9F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xA0-0xAF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xB0-0xBF

    // Invalid/overlong start bytes (C0, C1), contribute 0
    0, 0,
    // 2-byte sequences, contribute 1 UTF-16 unit
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,        // 0xC2-0xCF
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0xD0-0xDF

    // 3-byte sequences, contribute 1 UTF-16 unit
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0xE0-0xEF

    // 4-byte sequences, contribute 2 UTF-16 units (surrogate pair)
    2, 2, 2, 2, 2,
    // Invalid start bytes (F5-FF), contribute 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // 0xF5-0xFF
};

#define FOREACH_TEXT_MEASURER_PROPERTY(V) \
  V(FontSize, 1)                          \
  V(Color, 1)                             \
  V(WhiteSpace, 1)                        \
  V(TextOverflow, 1)                      \
  V(FontWeight, 1)                        \
  V(FontStyle, 1)                         \
  V(FontFamily, 1)                        \
  V(LineHeight, 1)                        \
  V(LetterSpacing, 1)                     \
  V(TextAlign, 1)                         \
  V(VerticalAlign, 1)                     \
  V(TextDecoration, 1)                    \
  V(Overflow, 1)                          \
  V(OverflowX, 1)                         \
  V(OverflowY, 1)

inline int IsTextMeasurerWanted(CSSPropertyID id) {
  static const auto& kWantedProperty = []() -> const int(&)[kPropertyEnd] {
    static int arr[kPropertyEnd];
    std::fill(std::begin(arr), std::end(arr), 0);

#define DECLARE_WANTED_PROPERTY(name, type) arr[kPropertyID##name] = type;
    FOREACH_TEXT_MEASURER_PROPERTY(DECLARE_WANTED_PROPERTY)
#undef DECLARE_WANTED_PROPERTY

    return arr;
  }();

  return kWantedProperty[id];
}

// TODO(zhengsenyao): it's better to move this method to
// base::String.length_utf16()
inline size_t GetUtf16SizeFromUtf8(const char* u8str, size_t length) {
  size_t utf16_size = 0;
  const unsigned char* p = reinterpret_cast<const unsigned char*>(u8str);
  const unsigned char* end = p + length;
  while (p < end) {
    utf16_size += kUtf8ToUtf16Units[*p++];
  }
  return utf16_size;
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_TEXT_PROPS_H_
