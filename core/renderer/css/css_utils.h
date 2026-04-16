// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_UTILS_H_
#define CORE_RENDERER_CSS_CSS_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

using RadialGradientSizeType = starlight::RadialGradientSizeType;
using RadialGradientShapeType = starlight::RadialGradientShapeType;
using DeclarationListConsumeFunction =
    base::MoveOnlyClosure<void, const char* /*key_start*/,
                          uint32_t /* key_length*/, const char* /*value_start*/,
                          uint32_t /*value_length*/, bool /*important*/>;

// Strips trailing "!important" (CSS case-insensitive, optional whitespace).
// Returns true if "!important" was found and removed.
// out_value_start/out_value_length point to the trimmed value without
// !important.
bool StripImportant(const char* value, uint32_t value_length,
                    const char** out_value_start, uint32_t* out_value_length);

inline std::string MaybeStripImportant(const std::string& value) {
  const char* start;
  uint32_t len;
  if (StripImportant(value.c_str(), static_cast<uint32_t>(value.length()),
                     &start, &len)) {
    return std::string(start, len);
  }
  return value;
}

inline std::string_view MaybeStripImportantAsView(const std::string& value) {
  const char* start;
  uint32_t len;
  if (StripImportant(value.c_str(), static_cast<uint32_t>(value.length()),
                     &start, &len)) {
    return std::string_view(start, len);
  }
  return std::string_view(value);
}

std::pair<float, float> GetRadialGradientRadius(
    RadialGradientShapeType shape, RadialGradientSizeType shape_size, float cx,
    float cy, float sx, float sy);

bool ParseStyleDeclarationList(const char* content, uint32_t content_length,
                               DeclarationListConsumeFunction consume_func);

ClassList SplitClasses(const char* content, size_t length);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_UTILS_H_
