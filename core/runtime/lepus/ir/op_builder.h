// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_OP_BUILDER_H_
#define CORE_RUNTIME_LEPUS_IR_OP_BUILDER_H_

#include <utility>

#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/instrs.h"

namespace lynx {
namespace lepus {
namespace ir {
class Block;
class Region;
class ModuleOp;

class OpBuilder {
 public:
  OpBuilder() = default;
  Region* CreateRegion(Operation* op, const llvh::StringRef name = {});
  Block* CreateBlock(Region* parent, const BlockType type, int64_t range,
                     llvh::StringRef name = "");
  Region* CreateEmptyRegion(Operation* op, const llvh::StringRef name);
  Block* CloneBlock(Region* region, Block* bb, llvh::StringRef name = "");
  Block* CreateTmpBlock(IRContext* ir_ctx);

  // Legacy aliases for existing call sites.
  void Insert(Operation* op, bool reset_parent = true) {
    block_->Insert(insert_point_, op, reset_parent);
  }

  template <typename op_ty, typename... Args>
  op_ty* Create(int64_t location, Args&&... args) {
    if (LEPUS_UNLIKELY(!GetBlock())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OpBuilder::Create called with nullptr insertion "
          "block");
    }
    op_ty* op = new op_ty(GetBlock(), this, location, args...);
    if (LEPUS_UNLIKELY(!llvh::isa<Operation>(op))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OpBuilder::Create created a non-Operation value");
    }
    block_->Insert(insert_point_, op);
    return op;
  }

  template <typename op_ty, typename... Args>
  op_ty* CreateType(int64_t location, Args&&... args) {
    op_ty* res = nullptr;
    {
      Block* pre_block = GetBlock();
      auto iter = GetInsertionPoint();
      SetInsertionPointToEnd(GetMod()->GetTypeBlock());
      res = Create<op_ty>(location, std::forward<Args>(args)...);
      SetInsertionPoint(pre_block, iter);
    }
    return res;
  }

  template <typename op_ty, typename... Args>
  void CreateType(op_ty* new_type, int64_t location, Args&&... args) {
    {
      Block* pre_block = GetBlock();
      auto iter = GetInsertionPoint();
      SetInsertionPointToEnd(GetMod()->GetTypeBlock());
      new_type->SetParent(GetMod()->GetTypeBlock());
      block_->Insert(insert_point_, new_type);
      SetInsertionPoint(pre_block, iter);
    }
  }

  template <typename op_ty, typename... Args>
  op_ty* CreateOnly(int64_t location, Args&&... args) {
    if (LEPUS_UNLIKELY(!GetBlock())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OpBuilder::CreateOnly called with nullptr insertion "
          "block");
    }
    op_ty* op =
        new op_ty(GetBlock(), this, location, std::forward<Args>(args)...);
    if (LEPUS_UNLIKELY(!llvh::isa<Operation>(op))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OpBuilder::CreateOnly created a non-Operation "
          "value");
    }
    return op;
  }

  /// Returns the current insertion point of the builder.
  Block::iterator GetInsertionPoint() const { return insert_point_; }

  /// reset
  void Reset() {
    block_ = nullptr;
    insert_point_ = {};
  }

  /// Set the insertion point to the specified location.
  void SetInsertionPoint(Block* block, Block::iterator insert_point) {
    block_ = block;
    insert_point_ = insert_point;
  }

  /// Sets the insertion point to the specified operation, which will cause
  /// subsequent insertions to go right before it.
  void SetInsertionPoint(Operation* op) {
    SetInsertionPoint(op->GetBlock(), Block::iterator(op));
  }

  /// Sets the insertion point to the node after the specified operation, which
  /// will cause subsequent insertions to go right after it.
  void SetInsertionPointAfter(Operation* op) {
    SetInsertionPoint(op->GetBlock(), ++Block::iterator(op));
  }

  /// Sets the insertion point to the start of the specified block.
  void SetInsertionPointToStart(Block* block) {
    SetInsertionPoint(block, block->begin());
  }

  /// Sets the insertion point to the end of the specified block.
  void SetInsertionPointToEnd(Block* block) {
    SetInsertionPoint(block, block->end());
  }

  // ---------------------- getter && setter ---------------------
  void SetModuleOp(ModuleOp* mod) { mod_ = mod; }

  ModuleOp* GetMod() { return mod_; }

  /// Returns the current block of the builder.
  Block* GetBlock() const { return block_; }

  Region* GetRegion() const { return GetBlock()->GetParent(); }

  // ----------------------- literal ------------------------------
  LiteralNull* GetLiteralNull();
  LiteralUndefined* GetLiteralUndefined();
  LiteralInt8* GetLiteralInt8(int64_t value);
  LiteralInt32* GetLiteralInt32(int64_t value);
  LiteralUint8* GetLiteralUint8(int64_t value);
  LiteralUint32* GetLiteralUint32(int64_t value);
  LiteralFloat64* GetLiteralFloat64(double value);
  LiteralBool* GetLiteralBool(bool value);

  Instruction* CloneInst(const Instruction* source_inst,
                         llvh::ArrayRef<Value*> operands);
  Instruction* CloneInst(Instruction* source_inst);

 private:
  ModuleOp* mod_ = nullptr;
  Block* block_ = nullptr;
  Block::iterator insert_point_;
};

struct OpBuilderRestoreInsertPointerRAII {
  explicit OpBuilderRestoreInsertPointerRAII(OpBuilder* builder)
      : builder(builder), preBlock(nullptr) {
    if (LEPUS_UNLIKELY(!builder)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: OpBuilderRestoreInsertPointerRAII constructed with "
          "nullptr OpBuilder");
    }
    preBlock = builder->GetBlock();
  }
  ~OpBuilderRestoreInsertPointerRAII() {
    builder->SetInsertionPointToEnd(preBlock);
  }

  OpBuilder* builder;
  Block* preBlock;
};
/// This is an RAII object that destroys instructions when it is destroyed.
class InstructionDestroyer {
  NON_COPYABLE(InstructionDestroyer);

 public:
  explicit InstructionDestroyer() = default;
  /// \returns true if the instruction \p A is already in the destruction
  /// queue. Notice that this is an O(n) search and should only be used for
  /// debugging.
  bool HasInstruction(Instruction* a) {
    return std::find(list_.begin(), list_.end(), a) != list_.end();
  }
  /// Add the instruction \p  A to the list of instructions to delete.
  void Add(Instruction* a) { list_.push_back(a); }
  ~InstructionDestroyer();

 private:
  llvh::SmallVector<Instruction*, 8> list_{};
};

}  // namespace ir

}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_OP_BUILDER_H_
