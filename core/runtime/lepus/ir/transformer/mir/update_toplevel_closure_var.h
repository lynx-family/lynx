// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_UPDATE_TOPLEVEL_CLOSURE_VAR_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_UPDATE_TOPLEVEL_CLOSURE_VAR_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class IRContext;
class PhiInst;
class FuncOp;
class RegisterAllocator;

class UpdateToplevelClosureVar : public ModulePass {
 public:
  explicit UpdateToplevelClosureVar(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "update-toplevel-closure-var") {}
  ~UpdateToplevelClosureVar() override = default;
  bool RunOnModule(ModuleOp* mod) override;

 private:
  void ProcessChildFunction(FuncOp* parent_func_op);
  FuncOp* toplevel_func_op_;
  RegisterAllocator* ra_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_UPDATE_TOPLEVEL_CLOSURE_VAR_H_
