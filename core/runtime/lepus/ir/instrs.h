// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_INSTRS_H_
#define CORE_RUNTIME_LEPUS_IR_INSTRS_H_

#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/IR/CFG.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/side_effect.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

using ArgList = llvh::SmallVector<Value*, 2>;

class NopInst : public Instruction {
  NON_COPYABLE(NopInst);

 public:
  explicit NopInst(Block* parent, OpBuilder* builder, int64_t location)
      : Instruction(ValueKind::NopInstKind, parent, builder, location) {}

  DEF_DEFAULT_COPY_CONSTRUCTOR(NopInst, Instruction);

  static bool HasOutput() { return false; }
  static bool IsTyped() { return true; }
  static SideEffect GetSideEffectImpl() { return SideEffect{}.SetIdempotent(); }
  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::NopInstKind;
  }
};

class SingleOperandInst : public Instruction {
  NON_COPYABLE(SingleOperandInst);

  // Make PushOperand private to ensure derived classes don't use it.
  using Instruction::PushOperand;

 protected:
  explicit SingleOperandInst(ValueKind value_kind, Block* parent,
                             OpBuilder* builder, int64_t location, Value* op)
      : Instruction(value_kind, parent, builder, location) {
    PushOperand(op);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SingleOperandInst, Instruction);

 public:
  enum OperandKind : uint8_t { SingleOperandIdx = 0 };

  Value* GetSingleOperand() const { return GetOperand(0); }

  static bool HasOutput() {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: SingleOperandInst::HasOutput must be overridden by "
        "subclasses");
  }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: SingleOperandInst::GetSideEffectImpl must be "
        "overridden by subclasses");
  }
};

/// Subclasses of this class are all able to terminate a basic
/// block. Thus, these are all the flow control type of operations.
class TerminatorInst : public Instruction {
  NON_COPYABLE(TerminatorInst);

 protected:
  explicit TerminatorInst(ValueKind kind, Block* parent, OpBuilder* builder,
                          int64_t location)
      : Instruction(kind, parent, builder, location) {}

  DEF_DEFAULT_COPY_CONSTRUCTOR(TerminatorInst, Instruction);

 public:
  unsigned GetNumSuccessors() const;
  Block* GetSuccessor(unsigned idx) const;
  void SetSuccessor(unsigned idx, Block* b);

  // Wrappers for llvh::SuccIterator compatibility
  unsigned getNumSuccessors() const { return GetNumSuccessors(); }
  Block* getSuccessor(unsigned idx) const { return GetSuccessor(idx); }
  void setSuccessor(unsigned idx, Block* b) { SetSuccessor(idx, b); }

  static bool HasOutput() {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: TerminatorInst::HasOutput must be overridden by "
        "subclasses");
  }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: TerminatorInst::GetSideEffectImpl must be overridden "
        "by subclasses");
  }

  using succ_iterator = llvh::SuccIterator<TerminatorInst, Block>;
  using succ_const_iterator =
      llvh::SuccIterator<const TerminatorInst, const Block>;
  using SuccRange = llvh::iterator_range<succ_iterator>;
  using SuccConstRange = llvh::iterator_range<succ_const_iterator>;

 private:
  inline succ_iterator succ_begin() { return succ_iterator(this); }
  inline succ_const_iterator succ_begin() const {
    return succ_const_iterator(this);
  }
  inline succ_iterator succ_end() { return succ_iterator(this, true); }
  inline succ_const_iterator succ_end() const {
    return succ_const_iterator(this, true);
  }

 public:
  inline SuccRange Successors() { return SuccRange(succ_begin(), succ_end()); }
  inline SuccConstRange Successors() const {
    return SuccConstRange(succ_begin(), succ_end());
  }

  static bool classof(const Value* v) {
    return LEPUS_IR_KIND_IN_CLASS(v->GetKind(), TerminatorInst);
  }
};

class BranchInst : public TerminatorInst {
  NON_COPYABLE(BranchInst);

 public:
  enum OperandKind : uint8_t { BranchDestIdx = 0 };

  explicit BranchInst(Block* parent, OpBuilder* builder, int64_t location,
                      Block* dest);
  DEF_DEFAULT_COPY_CONSTRUCTOR(BranchInst, TerminatorInst);

  Block* GetBranchDest() const {
    return llvh::cast<Block>(GetOperand(BranchDestIdx));
  }

  void SetBranchDest(Block* dest) {
    SetOperand(llvh::cast<Value>(dest), BranchDestIdx);
  }

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const { return {}; }

  unsigned GetNumSuccessorsImpl() const { return 1; }
  Block* GetSuccessorImpl(unsigned idx) const {
    if (LEPUS_UNLIKELY(idx != 0)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BranchInst::GetSuccessorImpl idx out of range");
    }
    return GetBranchDest();
  }
  void SetSuccessorImpl(unsigned idx, Block* b) {
    if (LEPUS_UNLIKELY(idx != 0)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BranchInst::SetSuccessorImpl idx out of range");
    }
    SetOperand(llvh::cast<Value>(b), idx);
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::BranchInstKind;
  }
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_INSTRS_H_
