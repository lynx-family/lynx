// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/utils/eval.h"

#include "core/runtime/lepus/ir/literal.h"
#include "core/runtime/lepus/ir/op_builder.h"

namespace lynx {
namespace lepus {
namespace ir {

LiteralBool* EvalToBoolean(OpBuilder* builder, Literal* operand) {
  bool value;
  switch (operand->GetKind()) {
    case ValueKind::LiteralNullKind:
    case ValueKind::LiteralUndefinedKind:
      value = false;
      break;
    case ValueKind::LiteralBoolKind:
      value = llvh::cast<LiteralBool>(operand)->GetValue();
      break;
    case ValueKind::LiteralInt32Kind: {
      const auto n = llvh::cast<LiteralInt32>(operand)->GetValue();
      value = n != 0;
      break;
    }
    case ValueKind::LiteralInt8Kind: {
      const auto n = llvh::cast<LiteralInt8>(operand)->GetValue();
      value = n != 0;
      break;
    }
    case ValueKind::LiteralUint32Kind: {
      const auto n = llvh::cast<LiteralUint32>(operand)->GetValue();
      value = n != 0;
      break;
    }
    case ValueKind::LiteralUint8Kind: {
      const auto n = llvh::cast<LiteralUint8>(operand)->GetValue();
      value = n != 0;
      break;
    }
    case ValueKind::LiteralFloat64Kind: {
      const auto n = llvh::cast<LiteralFloat64>(operand)->GetValue();
      value = !std::isnan(n) && n != 0.0;
      break;
    }
    default:
      return nullptr;
  }

  return builder->GetLiteralBool(value);
}

LiteralBool* EvalToBoolean(OpBuilder* builder, Value* operand) {
  if (auto* l = llvh::dyn_cast<Literal>(operand)) {
    return EvalToBoolean(builder, l);
  }

  TypeOp* type = operand->GetType();
  if (type->IsTableType()) {
    return builder->GetLiteralBool(true);
  }
  return nullptr;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
