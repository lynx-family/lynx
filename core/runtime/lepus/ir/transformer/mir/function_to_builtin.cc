// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

Value* StripToDefiningValue(Value* v) {
  // Peel through trivial register moves and toplevel store wrappers so builtin
  // matching sees the original producer instead of the forwarding chain.
  while (auto* mov = llvh::dyn_cast_or_null<MovInst>(v)) {
    v = mov->GetSingleOperand();
  }
  if (auto* set_tv = llvh::dyn_cast_or_null<SetToplevelVarInst>(v)) {
    v = set_tv->GetSrc();
  }
  if (auto* set_tc = llvh::dyn_cast_or_null<SetToplevelClosureVarInst>(v)) {
    v = set_tc->GetSrc();
  }
  while (auto* mov = llvh::dyn_cast_or_null<MovInst>(v)) {
    v = mov->GetSingleOperand();
  }
  return v;
}

FuncOp* FindDeepCloneFunc(IRContext* ir_ctx) {
  if (ir_ctx == nullptr || ir_ctx->GetMainMod() == nullptr) {
    return nullptr;
  }
  for (auto* func : *ir_ctx->GetMainMod()) {
    if (func == nullptr || func->GetLepusFunction() == nullptr) {
      continue;
    }
    if (func->GetLepusFunction()->GetFunctionName() ==
        constants::kDeepCloneName) {
      return func;
    }
  }
  return nullptr;
}

bool IsDirectDeepCloneToplevelClosure(Value* v, IRContext* ir_ctx) {
  auto* def_val = StripToDefiningValue(v);
  auto* create_closure = llvh::dyn_cast_or_null<CreateClosureInst>(def_val);
  if (create_closure == nullptr || ir_ctx == nullptr ||
      ir_ctx->GetMainMod() == nullptr) {
    return false;
  }

  auto* root_func_op = ir_ctx->GetMainMod()->GetRootFunction();
  // Only root-function child closures are considered here. The deep-clone
  // builtin is materialized as a root child function before later lowering
  // rewrites closure accesses to their final runtime representation.
  if (root_func_op == nullptr ||
      create_closure->GetFunction() != root_func_op) {
    return false;
  }

  auto root_func = root_func_op->GetLepusFunction();
  if (root_func == nullptr) {
    return false;
  }

  const auto child_idx = create_closure->GetChildrenIndex();
  if (child_idx >= root_func->GetChildFunction().size()) {
    return false;
  }
  const auto& child_func = root_func->GetChildFunction()[child_idx];
  return child_func != nullptr &&
         child_func->GetFunctionName() == constants::kDeepCloneName;
}

bool IsDirectDeepCloneUpvalue(Value* v, FuncOp* owner_func) {
  auto* def_val = StripToDefiningValue(v);
  auto* get_upvalue = llvh::dyn_cast_or_null<GetUpvalueInst>(def_val);
  if (get_upvalue == nullptr) {
    return false;
  }

  auto* idx = get_upvalue->GetIndex();
  if (!llvh::isa<LiteralUint8>(idx)) {
    return false;
  }

  auto* current_func = get_upvalue->GetFunc();
  if (current_func == nullptr) {
    current_func = owner_func;
  }
  if (current_func == nullptr || current_func->GetLepusFunction() == nullptr) {
    return false;
  }

  const auto upvalue_index = llvh::cast<LiteralUint8>(idx)->GetValue();
  if (upvalue_index >= current_func->GetLepusFunction()->UpvaluesSize()) {
    return false;
  }
  // Match the original captured symbol name here. This pass runs before
  // instruction selection rewrites closure accesses to final shared slots.
  auto* upvalue_info =
      current_func->GetLepusFunction()->GetUpvalue(upvalue_index);
  return upvalue_info != nullptr &&
         upvalue_info->name_ == constants::kDeepCloneName;
}

bool IsDirectDeepCloneValue(Value* v, FuncOp* owner_func, IRContext* ir_ctx) {
  return IsDirectDeepCloneToplevelClosure(v, ir_ctx) ||
         IsDirectDeepCloneUpvalue(v, owner_func);
}

class FunctionToBuiltInPass : public FunctionPass {
 public:
  explicit FunctionToBuiltInPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "function-to-builtin") {}
  ~FunctionToBuiltInPass() override = default;

  bool RunOnFunction(FuncOp* func) override;
};

bool FunctionToBuiltInPass::RunOnFunction(FuncOp* func) {
  if (func == nullptr || ir_ctx_ == nullptr) {
    return false;
  }

  auto* deep_clone_func = FindDeepCloneFunc(ir_ctx_);
  if (deep_clone_func == nullptr) {
    return false;
  }

  bool changed = false;
  for (auto& bb : *func) {
    for (auto& inst : bb) {
      auto* value = llvh::dyn_cast<Instruction>(&inst);
      if (value == nullptr || !IsDirectDeepCloneValue(value, func, ir_ctx_)) {
        continue;
      }

      for (auto* user : value->GetUsers()) {
        // The fast path only handles the canonical builtin shape:
        //   callee == $deepClone value
        //   exactly one payload argument
        // Anything else means the function escapes as a first-class value.
        auto* call = llvh::dyn_cast<CallInst>(user);
        if (call != nullptr && call->GetFunction() == value &&
            call->GetNumArguments() == 1) {
          if (!call->IsDeepCloneCall()) {
            call->SetDeepCloneCall(true);
            changed = true;
          }
          continue;
        }

        // Any first-class use other than the recognized direct builtin call
        // needs the real $deepClone function body to survive lowering.
        deep_clone_func->MarkDeepCloneLoweringRequired();
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
