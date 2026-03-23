// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_PROCESS_SPECIAL_MOV_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_PROCESS_SPECIAL_MOV_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

// ProcessSpecialMovPass: Replace MovInst with specialized instructions
// for toplevel closure variables.
//
// This pass runs after SSA construction and identifies MovInst where the source
// or destination has the ClosureVarReg attribute set. It replaces these MovInst
// with GetToplevelClosureVarInst and/or SetToplevelClosureVarInst.
//
// Scenarios:
// 1. src has ClosureVarReg: Replace with GetToplevelClosureVarInst
// 2. dst has ClosureVarReg: Replace with SetToplevelClosureVarInst
// 3. Both have ClosureVarReg: Replace with Get + Set
// 4. Neither has ClosureVarReg: Remove MovInst (SSA form, direct binding)

class MovInst;
class IRContext;
class MovInst;
class GetToplevelVarInst;
class SetToplevelVarInst;
class ProcessSpecialMovPass : public FunctionPass {
 public:
  explicit ProcessSpecialMovPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "process-special-mov") {}

  bool RunOnFunction(FuncOp* func) override;

  void RemoveNormalUselessMovInst(FuncOp* func);
  bool IsSpecialMovInst(MovInst* mov_inst);

  void UpdateClosureAttributes(IRContext* ir_ctx, MovInst* mov_inst);

  void ProcessMovInstForClosureVar(IRContext* ir_ctx, MovInst* mov_inst);
  void ProcessMovInstForToplevelVar(IRContext* ir_ctx, MovInst* mov_inst,
                                    unsigned dst_toplevel_reg,
                                    unsigned src_toplevel_reg);

  void ProcessNonMovInstForSpecialAttribute(IRContext* ir_ctx,
                                            Instruction* inst);

 private:
  llvh::SmallVector<MovInst*, 16> to_removed_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_PROCESS_SPECIAL_MOV_H_
