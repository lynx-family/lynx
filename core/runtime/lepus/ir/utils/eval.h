// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_UTILS_EVAL_H_
#define CORE_RUNTIME_LEPUS_IR_UTILS_EVAL_H_

#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

class OpBuilder;

/// Converts a literal to a boolean literal.
/// \returns a bool if the operand can be converted to one, nullptr otherwise.
LiteralBool* EvalToBoolean(OpBuilder* builder, Literal* operand);

/// Converts a value to a boolean literal.
/// Returns a boolean if the operand can be converted to one, nullptr otherwise.
LiteralBool* EvalToBoolean(OpBuilder* builder, Value* operand);
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_UTILS_EVAL_H_
