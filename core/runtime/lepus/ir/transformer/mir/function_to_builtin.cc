// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class FunctionToBuiltInPass : public FunctionPass {
 public:
  explicit FunctionToBuiltInPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "function-to-builtin") {}
  ~FunctionToBuiltInPass() override = default;

  bool RunOnFunction(FuncOp* func) override;

 private:
  static Value* StripToDefiningValue(Value* v) {
    if (v == nullptr) {
      return nullptr;
    }
    while (auto* mov = llvh::dyn_cast<MovInst>(v)) {
      v = mov->GetSingleOperand();
    }
    if (auto* set_tv = llvh::dyn_cast<SetToplevelVarInst>(v)) {
      v = set_tv->GetSrc();
      while (auto* mov = llvh::dyn_cast<MovInst>(v)) {
        v = mov->GetSingleOperand();
      }
    }
    if (auto* set_tc = llvh::dyn_cast<SetToplevelClosureVarInst>(v)) {
      v = set_tc->GetSrc();
      while (auto* mov = llvh::dyn_cast<MovInst>(v)) {
        v = mov->GetSingleOperand();
      }
    }
    return v;
  }

  bool IsDeepCloneViaPhysicalClosureReg(uint32_t physical_reg) const {
    auto* toplevel_func_op = ir_ctx_->GetMainMod()->GetRootFunction();
    if (toplevel_func_op == nullptr) {
      return false;
    }

    auto* ra =
        ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(toplevel_func_op);
    if (ra == nullptr) {
      return false;
    }

    auto root_func = toplevel_func_op->GetLepusFunction();
    if (root_func == nullptr) {
      return false;
    }

    // Iterate all toplevel closure anchors and find the one that:
    // - is allocated to `physical_reg`
    // - defines a closure whose child function name is `$deepClone`
    auto& closure_var_reg_map = toplevel_func_op->GetClosureVarReg2ValueMap();
    for (const auto& kv : closure_var_reg_map) {
      Value* anchor = kv.second;
      if (anchor == nullptr) continue;
      if (!ra->IsAllocated(anchor)) continue;
      if (ra->GetRegister(anchor).GetIndex() != physical_reg) continue;

      auto* def_val = StripToDefiningValue(anchor);
      auto* create_closure = llvh::dyn_cast_or_null<CreateClosureInst>(def_val);
      if (create_closure == nullptr) continue;

      const auto child_idx = create_closure->GetChildrenIndex();
      if (child_idx >= root_func->GetChildFunction().size()) continue;
      const auto& child_func = root_func->GetChildFunction()[child_idx];
      if (child_func != nullptr &&
          child_func->GetFunctionName() == constants::kDeepCloneName) {
        return true;
      }
    }
    return false;
  }
};

bool FunctionToBuiltInPass::RunOnFunction(FuncOp* func) {
  if (func == nullptr || ir_ctx_ == nullptr) {
    return false;
  }

  bool changed = false;

  for (auto& bb : *func) {
    for (auto it = bb.InstBegin(); it != bb.InstEnd(); ++it) {
      auto* inst = *it;
      auto* call = llvh::dyn_cast<CallInst>(inst);
      if (call == nullptr) {
        continue;
      }
      if (call->GetNumArguments() != 1) {
        continue;
      }
      if (call->IsDeepCloneCall()) {
        continue;
      }

      Value* callee = call->GetFunction();
      while (auto* mov = llvh::dyn_cast<MovInst>(callee)) {
        callee = mov->GetSingleOperand();
      }

      // 1) Nested function calls `$deepClone` via GetUpvalueInst.
      if (auto* get_upvalue = llvh::dyn_cast<GetUpvalueInst>(callee)) {
        auto* idx = get_upvalue->GetIndex();
        if (!llvh::isa<LiteralUint8>(idx)) {
          continue;
        }
        const auto upvalue_index = llvh::cast<LiteralUint8>(idx)->GetValue();

        // UpdateToplevelClosureVarPass records a mapping:
        //   (upvalue_index) -> (toplevel physical reg)
        // on FuncOp. This is the same mapping used by instruction selection to
        // emit GetToplevelClosureVar.
        auto* current_func_op = get_upvalue->GetFunc();
        if (current_func_op == nullptr) {
          continue;
        }
        const long toplevel_reg =
            current_func_op->GetClosureVarToplevelReg(upvalue_index);
        if (toplevel_reg < 0) {
          continue;
        }

        if (IsDeepCloneViaPhysicalClosureReg(
                static_cast<uint32_t>(toplevel_reg))) {
          call->SetDeepCloneCall(true);
          changed = true;
        }
        continue;
      }
    }
  }

  return changed;
}

Pass* CreateFunctionToBuiltInPass(IRContext* ir_ctx) {
  return new FunctionToBuiltInPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
