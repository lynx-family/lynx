// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#ifndef CLAY_LYNX_ADAPTOR_BASE_DEF_H_
#define CLAY_LYNX_ADAPTOR_BASE_DEF_H_

namespace lynx {
// Keep same as "core/base/js_constants.h"
static constexpr const char* const BIG_INT_VAL = "__lynx_val__";
static constexpr const int64_t kMaxJavaScriptNumber = 9007199254740991;
static constexpr const int64_t kMinJavaScriptNumber = -9007199254740991;

// Keep same as "core/renderer/css/css_property.h"
using PseudoState = uint32_t;
static constexpr PseudoState kPseudoStateNone = 0;
static constexpr PseudoState kPseudoStateHover = 1;
static constexpr PseudoState kPseudoStateHoverTransition = 1 << 1;
static constexpr PseudoState kPseudoStateActive = 1 << 3;
static constexpr PseudoState kPseudoStateActiveTransition = 1 << 4;
static constexpr PseudoState kPseudoStateFocus = 1 << 6;
static constexpr PseudoState kPseudoStateFocusTransition = 1 << 7;
static constexpr PseudoState kPseudoStatePlaceHolder = 1 << 8;
static constexpr PseudoState kPseudoStateBefore = 1 << 9;
static constexpr PseudoState kPseudoStateAfter = 1 << 10;
static constexpr PseudoState kPseudoStateSelection = 1 << 11;

// Keep same as "core/style/transform_raw_data.h"
struct TransformRawData {
  static constexpr int INDEX_FUNC = 0;
  static constexpr int INDEX_TRANSLATE_0 = 1;
  static constexpr int INDEX_TRANSLATE_0_UNIT = 2;
  static constexpr int INDEX_TRANSLATE_1 = 3;
  static constexpr int INDEX_TRANSLATE_1_UNIT = 4;
  static constexpr int INDEX_TRANSLATE_2 = 5;
  static constexpr int INDEX_TRANSLATE_2_UNIT = 6;
};
}  // namespace lynx
#endif  // CLAY_LYNX_ADAPTOR_BASE_DEF_H_
