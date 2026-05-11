// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/update_toplevel_closure_var.h"

#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/type_op.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

long ResolveRootFuncDeoptSlotByName(VMContext* vm_ctx,
                                    const UpvalueInfo* upvalue) {
  // In root-function deopt mode, child closures reconnect to the VMContext's
  // logical toplevel slots by variable name instead of the optimized root
  // function's physical registers.
  const auto& toplevel_vars = vm_ctx->GetToplevelVariables();
  auto it = toplevel_vars.find(upvalue->name_);
  if (it == toplevel_vars.end()) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelClosureVar failed to resolve root "
        "function deopt slot for child upvalue by name");
  }
  return it->second;
}

void UpdateToplevelClosureVar::ProcessChildFunction(FuncOp* parent_func_op) {
  if (!parent_func_op) return;
  auto parent_func = parent_func_op->GetLepusFunction();
  if (!parent_func) return;

  for (auto child : parent_func->GetChildFunction()) {
    auto* child_func_op = ir_ctx_->GetFuncOp(child);
    if (LEPUS_UNLIKELY(!child_func_op)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: UpdateToplevelClosureVar cannot find child FuncOp "
          "for lepus::Function");
    }

    for (auto i = 0; i < child->UpvaluesSize(); i++) {
      auto upvalue = child->GetUpvalue(i);

      auto new_reg =
          parent_func_op->GetClosureVarToplevelReg(upvalue->register_);
      if (LEPUS_UNLIKELY(new_reg == constants::kInvalidSignedValue)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: UpdateToplevelClosureVar failed to map closure "
            "var to toplevel reg");
      }
      child_func_op->RecordUpvalueIndex2ToplevelReg(i, new_reg);
    }
    ProcessChildFunction(child_func_op);
  }
}

bool UpdateToplevelClosureVar::RunOnModule(ModuleOp* mod) {
  if (LEPUS_UNLIKELY(!mod)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelClosureVar called with nullptr "
        "ModuleOp");
  }
  toplevel_func_op_ = mod->GetRootFunction();
  if (LEPUS_UNLIKELY(!toplevel_func_op_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelClosureVar requires non-null root "
        "function");
  }
  ra_ =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(toplevel_func_op_);
  if (LEPUS_UNLIKELY(!ra_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelClosureVar requires RegisterAllocator "
        "analysis");
  }
  auto* target_ctx = ir_ctx_->GetTargetContext();
  target_ctx->PlanRootFuncDeopt(mod);
  bool root_func_deopt = target_ctx->IsRootFuncDeopt();

  if (!root_func_deopt &&
      LEPUS_UNLIKELY(toplevel_func_op_->GetClosureVarReg2ValueMap().size() !=
                     toplevel_func_op_->GetToplevelClosureVarRegs().size())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelClosureVar detected mismatch in global "
        "closure vars size");
  }

  auto lepus_function = toplevel_func_op_->GetLepusFunction();
  if (LEPUS_UNLIKELY(lepus_function->UpvaluesSize() != 0)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelClosureVar expects toplevel function to "
        "have no upvalues");
  }

  auto* vm_ctx = ir_ctx_->GetVMContext();
  for (auto child : lepus_function->GetChildFunction()) {
    auto* child_func_op = ir_ctx_->GetFuncOp(child);
    if (LEPUS_UNLIKELY(!child_func_op)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: UpdateToplevelClosureVar cannot find child FuncOp "
          "for lepus::Function");
    }
    for (auto i = 0; i < child->UpvaluesSize(); i++) {
      auto upvalue = child->GetUpvalue(i);
      if (LEPUS_UNLIKELY(!upvalue->in_parent_vars_)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: UpdateToplevelClosureVar expected child upvalue "
            "to be in_parent_vars_");
      }

      auto old_reg = upvalue->register_;
      long new_reg = old_reg;
      if (root_func_deopt) {
        new_reg = ResolveRootFuncDeoptSlotByName(vm_ctx, upvalue);
      } else {
        auto value = toplevel_func_op_->GetClosureVarGivenReg(old_reg);
        if (LEPUS_UNLIKELY(!value)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: UpdateToplevelClosureVar failed to find closure "
              "value for old reg");
        }
        if (LEPUS_UNLIKELY(!(value->GetClosureVarReg() == old_reg ||
                             value->GetToplevelVarReg() == old_reg))) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: UpdateToplevelClosureVar detected inconsistent "
              "closure var mapping");
        }
        new_reg = static_cast<long>(ra_->GetRegister(value).GetIndex());
      }

      // This side table stores either the optimized root physical register or
      // the resolved root-function deopt slot, depending on the selected path.
      child_func_op->RecordUpvalueIndex2ToplevelReg(i, new_reg);
    }

    ProcessChildFunction(child_func_op);
  }
  return true;
}

Pass* CreateUpdateToplevelClosureVarPass(IRContext* ir_ctx) {
  return new UpdateToplevelClosureVar(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
