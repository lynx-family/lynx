// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/op_builder.h"

#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/region_op.h"
#include "core/runtime/lepus/ir/type_op.h"

namespace lynx {
namespace lepus {
namespace ir {

Region* OpBuilder::CreateRegion(Operation* op, const llvh::StringRef name) {
  return op->AddRegion(op, name);
}

Block* OpBuilder::CreateBlock(Region* region, const BlockType type,
                              int64_t range, llvh::StringRef name) {
  return new Block(this, range, region, type, name);
}

Region* OpBuilder::CreateEmptyRegion(Operation* op,
                                     const llvh::StringRef name) {
  return op->AddRegion(op, name);
}

Block* OpBuilder::CloneBlock(Region* region, Block* bb, llvh::StringRef name) {
  return new Block(this, region, bb, name);
}

Block* OpBuilder::CreateTmpBlock(IRContext* ir_ctx) {
  return new Block(ir_ctx);
}

LiteralNull* OpBuilder::GetLiteralNull() { return mod_->GetLiteralNull(); }
LiteralUndefined* OpBuilder::GetLiteralUndefined() {
  return mod_->GetLiteralUndefined();
}
LiteralInt8* OpBuilder::GetLiteralInt8(int64_t value) {
  return mod_->GetLiteralInt8(static_cast<int8_t>(value));
}
LiteralInt32* OpBuilder::GetLiteralInt32(int64_t value) {
  return mod_->GetLiteralInt32(value);
}

LiteralUint8* OpBuilder::GetLiteralUint8(int64_t value) {
  return mod_->GetLiteralUint8(static_cast<uint8_t>(value));
}

LiteralUint32* OpBuilder::GetLiteralUint32(int64_t value) {
  return mod_->GetLiteralUint32(static_cast<uint32_t>(value));
}

LiteralFloat64* OpBuilder::GetLiteralFloat64(double value) {
  return mod_->GetLiteralFloat64(value);
}

LiteralBool* OpBuilder::GetLiteralBool(bool value) {
  return mod_->GetLiteralBool(value);
}

Instruction* OpBuilder::CloneInst(const Instruction* source_inst,
                                  llvh::ArrayRef<Value*> operands) {
  Instruction* inst = nullptr;
  switch (source_inst->GetKind()) {
#define ENABLE_MIR_INSTR 1

#define DEF_VALUE(name, parent)                                              \
  case ValueKind::name##Kind:                                                \
    inst =                                                                   \
        new name(GetBlock(), this, llvh::cast<name>(source_inst), operands); \
    break;

#define DEF_TAG(name, parent)                                            \
  case ValueKind::name##Kind:                                            \
    inst = new parent(GetBlock(), this, llvh::cast<parent>(source_inst), \
                      operands);                                         \
    break;

#include "core/runtime/lepus/ir/instrs.def"

#undef DEF_TAG
#undef DEF_VALUE
#undef ENABLE_MIR_INSTR
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OpBuilder::CloneInst does not support cloning this "
          "ValueKind");
  }
  Insert(inst);
  return inst;
}

Instruction* OpBuilder::CloneInst(Instruction* source_inst) {
  llvh::SmallVector<Value*, 8> operands;
  for (uint32_t idx = 0; idx < source_inst->GetNumOperands(); idx++) {
    operands.push_back(source_inst->GetOperand(idx));
  }
  return CloneInst(source_inst, operands);
}

InstructionDestroyer::~InstructionDestroyer() {
  for (auto* inst : list_) {
    inst->EraseFromParent();
  }
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
