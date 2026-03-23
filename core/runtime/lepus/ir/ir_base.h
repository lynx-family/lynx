// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_IR_BASE_H_
#define CORE_RUNTIME_LEPUS_IR_IR_BASE_H_

#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ArrayRef.h"

namespace lynx {
namespace lepus {
namespace ir {

#define NON_COPYABLE(Class)              \
  Class(const Class&) = delete;          \
  Class(const Class&&) = delete;         \
  void operator=(const Class&) = delete; \
  void operator=(Class&&) = delete;

#define DEF_DEFAULT_COPY_CONSTRUCTOR(class_inst, base_inst)                   \
  explicit class_inst(Block* parent, OpBuilder* builder,                      \
                      const class_inst* src, llvh::ArrayRef<Value*> operands) \
      : base_inst(parent, builder, src, operands) {}

#define LEPUS_LIKELY(EXPR) __builtin_expect((bool)(EXPR), true)
#define LEPUS_UNLIKELY(EXPR) __builtin_expect((bool)(EXPR), false)

#define LEPUS_ATTRIBUTE_NOINLINE __attribute__((noinline))
#define LEPUS_ATTRIBUTE_USED __attribute__((__used__))
#define LEPUS_DUMP_METHOD LEPUS_ATTRIBUTE_NOINLINE LEPUS_ATTRIBUTE_USED

enum class StageMode : uint8_t { SM_HIR, SM_MIR, SM_REG_ALLOC, SM_VM_TARGET };

namespace constants {

constexpr uint32_t kInvalidSignedValue = -1U;
constexpr uint32_t kLoadStoreEliminationIter = 10;
constexpr uint32_t kTypePropagationIter = 3;
constexpr uint32_t kMaxCombineTimes = 8;
constexpr uint8_t kInstMaxOperands = 3;

// Global Objects
constexpr const char kGlobalMath[] = "Math";
constexpr const char kGlobalString[] = "String";
constexpr const char kBuiltinJSON[] = "JSON";
constexpr const char kBuiltinDate[] = "Date";
constexpr const char kBuiltinObject[] = "Object";

// Builtin Functions
constexpr const char kParseInt[] = "parseInt";
constexpr const char kParseFloat[] = "parseFloat";
constexpr const char kIsNaN[] = "isNaN";
constexpr const char kIsFinite[] = "isFinite";
constexpr const char kEncodeURI[] = "encodeURI";
constexpr const char kEncodeURIComponent[] = "encodeURIComponent";
constexpr const char kDecodeURI[] = "decodeURI";
constexpr const char kDecodeURIComponent[] = "decodeURIComponent";

// Renderer Global Functions
constexpr const char kGetElementUniqueID[] = "__GetElementUniqueID";
constexpr const char kCreateElement[] = "__CreateElement";
constexpr const char kCreateView[] = "__CreateView";
constexpr const char kCreateImage[] = "__CreateImage";
constexpr const char kCreateText[] = "__CreateText";
constexpr const char kCreatePage[] = "__CreatePage";
constexpr const char kCreateComponent[] = "__CreateComponent";
constexpr const char kGetDiffData[] = "__GetDiffData";
constexpr const char kGetSystemInfo[] = "__GetSystemInfo";
constexpr const char kGetTextInfo[] = "__GetTextInfo";
constexpr const char kIsArray[] = "__IsArray";

// Math Methods
constexpr const char kMathAbs[] = "abs";
constexpr const char kMathAcos[] = "acos";
constexpr const char kMathAsin[] = "asin";
constexpr const char kMathAtan[] = "atan";
constexpr const char kMathCeil[] = "ceil";
constexpr const char kMathCos[] = "cos";
constexpr const char kMathExp[] = "exp";
constexpr const char kMathFloor[] = "floor";
constexpr const char kMathLog[] = "log";
constexpr const char kMathMax[] = "max";
constexpr const char kMathMin[] = "min";
constexpr const char kMathPow[] = "pow";
constexpr const char kMathRound[] = "round";
constexpr const char kMathSin[] = "sin";
constexpr const char kMathSqrt[] = "sqrt";
constexpr const char kMathTan[] = "tan";

// String Methods
constexpr const char kStringIndexOf[] = "indexOf";
constexpr const char kStringLength[] = "length";
constexpr const char kStringSubstr[] = "substr";
constexpr const char kStringSplit[] = "split";
constexpr const char kStringTrim[] = "trim";
constexpr const char kStringCharAt[] = "charAt";
constexpr const char kStringSearch[] = "search";
constexpr const char kStringMatch[] = "match";
constexpr const char kStringSlice[] = "slice";
constexpr const char kStringSubstring[] = "substring";
constexpr const char kStringReplace[] = "replace";

// JSON Methods
constexpr const char kJSONStringify[] = "stringify";
constexpr const char kJSONParse[] = "parse";

// Date Methods
constexpr const char kDateNow[] = "now";

// Object Methods
constexpr const char kObjectKeys[] = "keys";
constexpr const char kObjectAssign[] = "assign";
constexpr const char kObjectFreeze[] = "freeze";

// Array Methods
constexpr const char kArrayConcat[] = "concat";
constexpr const char kArrayJoin[] = "join";
constexpr const char kArrayIncludes[] = "includes";
constexpr const char kArraySlice[] = "slice";
constexpr const char kArrayPush[] = "push";
constexpr const char kArrayPop[] = "pop";
constexpr const char kArrayFindIndex[] = "findIndex";
constexpr const char kArrayMap[] = "map";
constexpr const char kArrayFilter[] = "filter";

// RegExp Methods
constexpr const char kRegExpTest[] = "test";

// Number Methods
constexpr const char kNumberToFixed[] = "toFixed";

// Builtin Functions Helper
constexpr const char kDeepCloneName[] = "$deepClone";

}  // namespace constants

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_IR_BASE_H_
