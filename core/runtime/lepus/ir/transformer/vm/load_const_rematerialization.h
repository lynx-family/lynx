// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_LOAD_CONST_REMATERIALIZATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_LOAD_CONST_REMATERIALIZATION_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class Block;
class FuncOp;
class Instruction;
class LoadConstInst;
class ModuleOp;
class RegisterAllocator;
class Value;

// This is a late MIR optimization pass that only considers the root/toplevel
// function. It selectively rematerializes long-lived scalar LoadConstInst
// values when the root function is likely to exceed the 8-bit VM register
// limit.
//
// Before running the expensive preliminary register allocation, the pass first
// applies a conservative cheap pre-filter on the root function. If that simple
// structural upper bound already proves the root function cannot exceed the
// limit, the pass exits without invoking any register-allocation logic.
// Otherwise it runs one preliminary allocation on the root function and uses
// the result to guide rematerialization around aggregate-building users.
class LoadConstRematerializationPass : public ModulePass {
 public:
  explicit LoadConstRematerializationPass(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "load-const-rematerialization") {}

  bool RunOnModule(ModuleOp* mod) override;

 private:
  enum class RematerializationPriority {
    kNone = 0,
    kHotAggregate,
    kLongTailAggregate,
  };

  static RematerializationPriority GetRematerializationPriority(
      const LoadConstInst* inst,
      const llvh::DenseSet<const Instruction*>& allocated_insts,
      size_t aggregate_user_count, size_t span);
  static bool ShouldSkipUsers(const LoadConstInst* inst);
  static void ReplaceAllOperandUsesInInstruction(Instruction* user, Value* from,
                                                 Value* to);
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_LOAD_CONST_REMATERIALIZATION_H_
