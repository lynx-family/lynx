// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_DEBUG_MSG_H_
#define CORE_RENDERER_CSS_CSS_DEBUG_MSG_H_

namespace lynx {
namespace tasm {
// css attr and style type
static const char *ANIMATION_PROPERTY = "animation-property";
static const char *BOOL_TYPE = "bool-type";
static const char *CUBIC_BEZIER = "cubic-bezier";
static const char *FLOAT_TYPE = "float-type";
static const char *SQUARE_BEZIER = "square-bezier";
static const char *STEP_VALUE = "step-value";
static const char *TIMING_FUNCTION = "timing-function";
static const char *TIME_VALUE = "time-value";

// css warning info
static const char *TYPE_UNSUPPORTED = "%s don't support type:%s";
static const char *TYPE_MUST_BE = "%s must be %s";
static const char *STRING_TYPE = "a string!";
static const char *NUMBER_TYPE = "a number!";
static const char *ARRAY_TYPE = "an array!";
static const char *ARRAY_OR_MAP_TYPE = "an array or a map!";
static const char *ARRAY_OR_NUMBER_TYPE = "an array or a number!";
static const char *ENUM_TYPE = "an enum!";
static const char *INT_TYPE = "an int!";
static const char *STRING_OR_NUMBER_TYPE = "a string or number!";
static const char *STRING_OR_BOOL_TYPE = "a string or bool!";
static const char *FORMAT_ERROR = "%s format error:%s";
static const char *EMPTY_ERROR = "%s is empty!";
static const char *SIZE_ERROR = "%s size error:%d";
static const char *TYPE_ERROR = "%s type error";
static const char *CANNOT_REACH_METHOD = "method unreachable.";
static const char *SET_PROPERTY_ERROR = "set %s error.";
}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_DEBUG_MSG_H_
