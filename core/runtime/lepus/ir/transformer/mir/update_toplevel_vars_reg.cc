// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/update_toplevel_closure_var.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/type_op.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class UpdateToplevelVarReg : public ModulePass {
 public:
  UpdateToplevelVarReg(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "update-toplevel-vars") {}
  ~UpdateToplevelVarReg() override = default;
  bool RunOnModule(ModuleOp* mod) override;
  llvh::SmallDenseMap<unsigned, unsigned, 16> toplevel_vars_reg_map_;

 private:
  RegisterAllocator* ra_;
};

// we need to update regs in top_level_variables_ in vm_context to new reg
bool UpdateToplevelVarReg::RunOnModule(ModuleOp* mod) {
  if (LEPUS_UNLIKELY(!mod)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelVarReg called with nullptr ModuleOp");
  }
  auto* toplevel_func_op = mod->GetRootFunction();
  if (LEPUS_UNLIKELY(!toplevel_func_op)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelVarReg requires non-null root function");
  }
  ra_ = ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(toplevel_func_op);
  if (LEPUS_UNLIKELY(!ra_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateToplevelVarReg requires RegisterAllocator "
        "analysis");
  }
  OpBuilder* builder = ir_ctx_->GetOpBuilder();

  auto& toplevel_vars = ir_ctx_->GetToplevelVariables();
  auto* vm_context = ir_ctx_->GetVMContext();

  for (const auto& item : toplevel_vars) {
    auto inst = item.second;
    unsigned new_reg = ra_->GetRegister(inst).GetIndex();

    if (LEPUS_UNLIKELY(new_reg >= toplevel_vars.size())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: UpdateToplevelVarReg produced toplevel var register "
          "index out of range");
    }

    auto original_reg = item.first;

    // original_reg -> new_reg
    bool find = false;
    for (const auto& item : vm_context->GetToplevelVariables()) {
      if (item.second == original_reg) {
        find = true;
        vm_context->UpdateToplevelVarReg(item.first, new_reg);
        toplevel_vars_reg_map_[original_reg] = new_reg;
        break;
      }
    }
    if (LEPUS_UNLIKELY(!find)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: UpdateToplevelVarReg cannot find toplevel variable "
          "in VMContext");
    }
  }

  // update SetToplevelVarInst & GetToplevelVarInst reg with new reg
  auto root_func = mod->GetRootFunction();
  for (auto& bb : *root_func) {
    for (auto& op : bb) {
      if (auto* set_toplevel_var_inst =
              llvh::dyn_cast<SetToplevelVarInst>(&op)) {
        auto reg =
            (llvh::cast<LiteralUint32>(set_toplevel_var_inst->GetToplevelReg()))
                ->GetValue();
        auto it = toplevel_vars_reg_map_.find(reg);
        if (LEPUS_UNLIKELY(it == toplevel_vars_reg_map_.end())) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: UpdateToplevelVarReg cannot find toplevel "
              "register remap for SetToplevelVarInst");
        }
        auto new_reg = it->second;
        set_toplevel_var_inst->SetToplevelReg(
            builder->GetLiteralUint32(new_reg));

      } else if (auto* get_toplevel_var_inst =
                     llvh::dyn_cast<GetToplevelVarInst>(&op)) {
        auto reg =
            (llvh::cast<LiteralUint32>(get_toplevel_var_inst->GetToplevelReg()))
                ->GetValue();
        auto it = toplevel_vars_reg_map_.find(reg);
        if (LEPUS_UNLIKELY(it == toplevel_vars_reg_map_.end())) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: UpdateToplevelVarReg cannot find toplevel "
              "register remap for GetToplevelVarInst");
        }
        auto new_reg = it->second;
        get_toplevel_var_inst->SetToplevelReg(
            builder->GetLiteralUint32(new_reg));
      }
    }
  }

  return true;
}

Pass* CreateUpdateToplevelVarRegPass(IRContext* ir_ctx) {
  return new UpdateToplevelVarReg(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
