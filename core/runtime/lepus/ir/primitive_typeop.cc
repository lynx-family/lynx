// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/primitive_typeop.h"

#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {
PrimitiveTypeOp::PrimitiveTypeOp(Block* parent, OpBuilder* builder,
                                 int64_t location, TypeOp::TypeKind kind)
    : TypeOp(ValueKind::PrimitiveTypeOpKind, parent,
             kind == TypeOp::INVALID_TYPE ? nullptr : builder, location, kind) {
}

PrimitiveTypeOp* PrimitiveTypeOp::GetInstance(OpBuilder* builder,
                                              TypeOp::TypeKind kind) {
  if (LEPUS_UNLIKELY(!builder)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: PrimitiveTypeOp::GetInstance called with nullptr "
        "OpBuilder");
  }
  auto* ir_ctx = builder->GetMod()->GetIRCtx();
  auto& primitive_types = ir_ctx->GetPrimitiveTypeMap();
  uint32_t key = static_cast<uint32_t>(kind);
  if (primitive_types.find(key) == primitive_types.end()) {
    auto* new_primitive_type = new PrimitiveTypeOp(nullptr, builder, {}, kind);
    builder->CreateType<PrimitiveTypeOp>(new_primitive_type, {}, kind);
    primitive_types[key] = new_primitive_type;
  }
  return primitive_types[key];
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

void llvh::ilist_alloc_traits<::lynx::lepus::ir::PrimitiveTypeOp>::deleteNode(
    ::lynx::lepus::ir::PrimitiveTypeOp* v) {
  ::lynx::lepus::ir::Value::Destroy(v);
}
