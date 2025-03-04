// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_JS_CONSTANTS_H_
#define CORE_BASE_JS_CONSTANTS_H_
#include <string>

namespace lynx {
namespace piper {

/*
 * https://www.npmjs.com/package/jsbi
 */

static constexpr const char* const CONSTRUCTOR_BIG_INT = "BigInt";
static constexpr const char* const BIG_INT_VAL = "__lynx_val__";
static constexpr const char* const TO_JSON = "toJSON";
static constexpr const char* const OPERATOR_ADD = "add";
static constexpr const char* const OPERATOR_SUBTRACT = "subtract";
static constexpr const char* const OPERATOR_MULTIPLY = "multiply";
static constexpr const char* const OPERATOR_DIVIDE = "divide";
static constexpr const char* const OPERATOR_REMAINDER = "remainder";
static constexpr const char* const OPERATOR_EQUAL = "equal";
static constexpr const char* const OPERATOR_NOT_EQUAL = "notEqual";
static constexpr const char* const OPERATOR_LESS_THAN = "lessThan";
static constexpr const char* const OPERATOR_LESS_THAN_OR_EQUAL =
    "lessThanOrEqual";
static constexpr const char* const OPERATOR_GREATER_THAN = "greaterThan";
static constexpr const char* const OPERATOR_GREATER_THAN_OR_EQUAL =
    "greaterThanOrEqual";
static constexpr const int BIG_INT_OBJ_PARAMS_COUNT = 4;
static constexpr const int64_t kMaxJavaScriptNumber = 9007199254740991;
static constexpr const int64_t kMinJavaScriptNumber = -9007199254740991;

// TODO: add more

}  // namespace piper
}  // namespace lynx

#endif  // CORE_BASE_JS_CONSTANTS_H_
