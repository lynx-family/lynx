// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/lepus/ir/module_op.h"

#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/op_builder.h"

#ifdef LEPUS_TEST
#include "core/runtime/lepus/ir/dialects/mir/mir_dumper.h"
#endif

namespace lynx {
namespace lepus {
namespace ir {

ModuleOp::ModuleOp(Block* parent, OpBuilder* builder, int64_t location,
                   IRContext* ir_ctx, llvh::StringRef name)
    : Operation(ValueKind::ModuleOpKind, nullptr, builder, location),
      name_(name) {
  // module op should set ir_ctx by hands
  this->ir_ctx_ = ir_ctx;

  // 1. create ir region
  auto* region = builder->CreateRegion(this);
  auto convert_type = [](ModBlockType type) -> BlockType {
    switch (type) {
      case ModBlockType::MRT_FUNC:
        return BlockType::BT_FUNC;
      case ModBlockType::MRT_TOPLEVEL_CODE:
        return BlockType::BT_INST;
      case ModBlockType::MRT_TYPE:
        return BlockType::BT_TYPE;
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleOp failed to convert ModBlockType to "
            "BlockType");
    }
  };
  for (uint32_t idx = 0; idx < ModBlockType::MRT_COUNT; idx++)
    builder->CreateBlock(region, convert_type(static_cast<ModBlockType>(idx)),
                         location);

  if (LEPUS_UNLIKELY(GetRegionSize() != 1)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: ModuleOp must have exactly one region");
  }
  if (LEPUS_UNLIKELY(GetIRRegion() != region)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: ModuleOp region pointer mismatch after construction");
  }
}

void ModuleOp::Init() {
  auto* builder = ir_ctx_->GetOpBuilder();
  literal_false_.SetLiteralBoolType(builder);
  literal_true_.SetLiteralBoolType(builder);
  literal_null_.SetLiteralNullType(builder);
  literal_undefined_.SetLiteralUndefinedType(builder);
}

IRContext* ModuleOp::GetIRCtx() const {
  if (LEPUS_UNLIKELY(!ir_ctx_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: ModuleOp::GetIRCtx called before ir_ctx_ is set");
  }
  return ir_ctx_;
}

LiteralInt8* ModuleOp::GetLiteralInt8(int8_t value) {
  return literal_int8s_.GetOrEmplaceWithNew(ir_ctx_->GetOpBuilder(), value)
      .first;
}

LiteralInt32* ModuleOp::GetLiteralInt32(int64_t value) {
  return literal_int32s_.GetOrEmplaceWithNew(ir_ctx_->GetOpBuilder(), value)
      .first;
}

LiteralUint8* ModuleOp::GetLiteralUint8(uint8_t value) {
  return literal_uint8s_.GetOrEmplaceWithNew(ir_ctx_->GetOpBuilder(), value)
      .first;
}

LiteralUint32* ModuleOp::GetLiteralUint32(uint32_t value) {
  return literal_uint32s_.GetOrEmplaceWithNew(ir_ctx_->GetOpBuilder(), value)
      .first;
}

LiteralFloat64* ModuleOp::GetLiteralFloat64(double value) {
  return literal_float64s_.GetOrEmplaceWithNew(ir_ctx_->GetOpBuilder(), value)
      .first;
}

LiteralBool* ModuleOp::GetLiteralBool(bool value) {
  if (value) return &literal_true_;
  return &literal_false_;
}

void ModuleOp::Dump(StageMode mode, std::ostream& os) {
  if (mode == StageMode::SM_MIR) {
#ifdef LEPUS_TEST
    MIRPrinter printer(ir_ctx_, os);
    printer.VisitModuleOp(*this);
#else
    (void)os;
#endif
    return;
  }

  throw ::lynx::lepus::CompileException(
      "Lepus IR error: ModuleOp::Dump only supports SM_MIR currently");
}

Parameter::Parameter(FuncOp* parent, int32_t param_index)
    : Value(ValueKind::ParameterKind,
            parent ? parent->GetIRCtx()->GetOpBuilder() : nullptr),
      parent_(parent),
      param_index_(param_index) {
  if (LEPUS_UNLIKELY(!parent_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Parameter constructed with nullptr parent FuncOp");
  }
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

void llvh::ilist_alloc_traits<::lynx::lepus::ir::ModuleOp>::deleteNode(
    ::lynx::lepus::ir::ModuleOp* v) {
  ::lynx::lepus::ir::Value::Destroy(v);
}
