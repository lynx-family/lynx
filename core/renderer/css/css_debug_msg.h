// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_DEBUG_MSG_H_
#define CORE_RENDERER_CSS_CSS_DEBUG_MSG_H_

namespace lynx {
namespace tasm {
// css attr and style type
constexpr char ANIMATION_PROPERTY[] = "animation-property";
constexpr char BOOL_TYPE[] = "bool-type";
constexpr char CUBIC_BEZIER[] = "cubic-bezier";
constexpr char FLOAT_TYPE[] = "float-type";
constexpr char SQUARE_BEZIER[] = "square-bezier";
constexpr char STEP_VALUE[] = "step-value";
constexpr char TIMING_FUNCTION[] = "timing-function";
constexpr char TIME_VALUE[] = "time-value";

// css warning info
constexpr char TYPE_UNSUPPORTED[] = "%s don't support type:%s";
constexpr char TYPE_MUST_BE[] = "%s must be %s";
constexpr char STRING_TYPE[] = "a string!";
constexpr char NUMBER_TYPE[] = "a number!";
constexpr char ARRAY_TYPE[] = "an array!";
constexpr char ARRAY_OR_MAP_TYPE[] = "an array or a map!";
constexpr char ARRAY_OR_NUMBER_TYPE[] = "an array or a number!";
constexpr char ENUM_TYPE[] = "an enum!";
constexpr char INT_TYPE[] = "an int!";
constexpr char STRING_OR_NUMBER_TYPE[] = "a string or number!";
constexpr char STRING_OR_BOOL_TYPE[] = "a string or bool!";
constexpr char FORMAT_ERROR[] = "%s format error:%s";
constexpr char EMPTY_ERROR[] = "%s is empty!";
constexpr char SIZE_ERROR[] = "%s size error:%d";
constexpr char TYPE_ERROR[] = "%s type error";
constexpr char CANNOT_REACH_METHOD[] = "method unreachable.";
constexpr char SET_PROPERTY_ERROR[] = "set %s error.";
}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_DEBUG_MSG_H_
