// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_CSS_NG_UTILS_H_
#define CORE_RENDERER_CSS_NG_CSS_NG_UTILS_H_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"

namespace lynx {
namespace css {
using UChar = base::UChar;
using LChar = base::LChar;
using UChar32 = base::UChar32;

#define U16_LEAD(supplementary) (UChar)(((supplementary) >> 10) + 0xd7c0)
#define U16_TRAIL(supplementary) (UChar)(((supplementary) & 0x3ff) | 0xdc00)

namespace ustring_helper {

inline void append(std::u16string& str, const UChar32 c) {
  if (((uint32_t)(c) <= 0xffff)) {  // U_IS_BMP
    str.push_back(static_cast<UChar>(c));
    return;
  }
  str.push_back(U16_LEAD(c));
  str.push_back(U16_TRAIL(c));
}

inline std::u16string from_string(const std::string& str) {
  return base::U8StringToU16(str);
}

inline std::u16string from_string(const char* str, size_t len) {
  return base::U8StringToU16(std::string(str, len));
}

inline std::string to_string(const std::u16string& str) {
  return base::U16StringToU8(str);
}

}  // namespace ustring_helper

constexpr LChar kEndOfFileMarker = 0;

// https://html.spec.whatwg.org/C/#parse-error-unexpected-null-character
constexpr UChar kReplacementCharacter = 0xFFFD;
const std::string& CSSGlobalEmptyString();
const std::u16string& CSSGlobalEmptyU16String();
const std::string& CSSGlobalStarString();
const std::u16string& CSSGlobalStarU16String();

template <class T>
inline T ToLower(const T& input) {
  T result;
  result.resize(input.size());
  std::transform(input.begin(), input.end(), result.begin(), ::tolower);
  return result;
}

inline std::string ToLowerASCII(const std::u16string& input) {
  std::u16string result;
  result.resize(input.size());
  std::transform(input.begin(), input.end(), result.begin(), ::tolower);
  return lynx::css::ustring_helper::to_string(result);
}

inline bool EqualIgnoringASCIICase(const std::u16string& lfs,
                                   const std::u16string& rhs) {
  return ToLower(lfs) == rhs;
}

inline bool EqualIgnoringASCIICase(const std::string& lfs,
                                   const std::string& rhs) {
  return ToLower(lfs) == rhs;
}

}  // namespace css
}  // namespace lynx
#endif  // CORE_RENDERER_CSS_NG_CSS_NG_UTILS_H_
