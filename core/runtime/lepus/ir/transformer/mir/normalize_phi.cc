// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/normalize_phi.h"

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/type_op.h"

namespace lynx {
namespace lepus {
namespace ir {

void NormalizePhiPass::SetPhiType(PhiInst* phi_inst) {
  TypeOp* phi_type = nullptr;
  for (auto i = 0; i < phi_inst->GetNumEntries(); i++) {
    auto* value = phi_inst->GetEntry(i).first;

    if (!phi_type)
      phi_type = value->GetType();
    else {
      if (value->GetType() != phi_type) {
        phi_type = TypeOp::CreateAnyType(phi_inst->GetBuilder());
        break;
      }
    }
  }
  phi_inst->SetType(phi_type);
}

void NormalizePhiPass::SetInstLocation(FuncOp* func) {
  for (auto& bb : *func) {
    int64_t loc = 0;
    int64_t last_loc = 0;

    for (auto& inst : bb) {
      // set corrent location for BranchInst
      loc = inst.GetLocation();
      if (LEPUS_UNLIKELY(loc == 0)) {
        inst.SetLocation(last_loc == 0 ? bb.GetLocation() : last_loc);
      }
      last_loc = loc;

      if (auto* phi_inst = llvh::dyn_cast<PhiInst>(&inst)) {
        SetPhiType(phi_inst);
      }
    }
  }
}

void NormalizePhiPass::NormalizePhi(FuncOp* func) {
  bool changed = false;
  do {
    changed = false;
    to_removed_.clear();
    for (auto& bb : *func) {
      for (auto& inst : bb) {
        auto* phi_inst = llvh::dyn_cast<PhiInst>(&inst);
        if (!phi_inst) continue;

        auto num_entries = phi_inst->GetNumEntries();
        if (num_entries == 0) continue;

        // 1. If the entry is phi_inst itself, remove it
        for (int i = num_entries - 1; i >= 0; i--) {
          if (phi_inst->GetEntry(i).first == phi_inst) {
            phi_inst->RemoveEntry(i);
            changed = true;
          }
        }

        // Update num_entries after potential removal
        num_entries = phi_inst->GetNumEntries();
        if (num_entries == 0) continue;

        // 2. If all remaining entries are the same value, replace with this
        // value
        Value* first_val = phi_inst->GetEntry(0).first;
        bool all_same = true;
        for (unsigned i = 1; i < num_entries; ++i) {
          if (phi_inst->GetEntry(i).first != first_val) {
            all_same = false;
            break;
          }
        }

        if (all_same) {
          phi_inst->ReplaceAllUsesWith(first_val);
          ir_ctx_->UpdateSpecialAttribute(phi_inst, first_val);
          to_removed_.emplace_back(phi_inst);
          changed = true;
        }
      }
    }

    for (auto* inst : to_removed_) {
      inst->EraseFromParent();
    }
  } while (changed);
}

bool NormalizePhiPass::RunOnFunction(FuncOp* func) {
  // set correct location for each inst
  SetInstLocation(func);

  // normalize phi_inst
  NormalizePhi(func);
  return true;
}

Pass* CreateNormalizePhiPass(IRContext* ir_ctx) {
  return new NormalizePhiPass(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
