// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_PRIMITIVE_TYPEOP_H_
#define CORE_RUNTIME_LEPUS_IR_PRIMITIVE_TYPEOP_H_

#include "core/runtime/lepus/ir/type_op.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {
class PrimitiveTypeOp : public TypeOp {
  friend class Value;
  NON_COPYABLE(PrimitiveTypeOp);

 public:
  static PrimitiveTypeOp* GetInstance(OpBuilder* builder, TypeKind kind);

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::PrimitiveTypeOpKind;
  }

 private:
  explicit PrimitiveTypeOp(Block* parent, OpBuilder* builder, int64_t location,
                           TypeOp::TypeKind kind);
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_PRIMITIVE_TYPEOP_H_
