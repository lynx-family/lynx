// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/load_null_elimination.h"

#include <unordered_map>

#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/op_builder.h"

namespace lynx {
namespace lepus {
namespace ir {

static Block::iterator FindBlockInsertionPoint(Block* bb) {
  // Respect FirstInBlock instructions: they must precede all other
  // instructions. We insert after the contiguous prefix of FirstInBlock.
  auto it = bb->begin();
  for (; it != bb->end(); ++it) {
    auto* inst = llvh::dyn_cast<Instruction>(&*it);
    if (!inst) continue;
    if (!inst->GetSideEffect().GetFirstInBlock()) {
      break;
    }
  }
  return it;
}

bool LoadNullEliminationPass::RunOnFunction(FuncOp* f) {
  if (!f || f->GetBlockSize() == 0) return false;

  auto* region = f->GetSingleRegion();
  if (LEPUS_UNLIKELY(!region)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: LoadNullEliminationPass expected single region");
  }
  // Collect all LoadNullOrUndefinedInst in this function, grouped by type.
  std::unordered_map<int8_t, llvh::SmallVector<LoadNullOrUndefinedInst*, 8>>
      load_nils;

  for (auto& bb : *f) {
    for (auto* inst : bb.InstRange()) {
      auto* load_nil = llvh::dyn_cast<LoadNullOrUndefinedInst>(inst);
      if (!load_nil) continue;
      // Be conservative: don't touch instructions carrying special attrs.
      if (load_nil->GetClosureVarReg() != constants::kInvalidSignedValue ||
          load_nil->GetToplevelVarReg() != constants::kInvalidSignedValue) {
        continue;
      }
      int8_t t = load_nil->GetLoadNilType()->GetValue();
      load_nils[t].push_back(load_nil);
    }
  }

  if (load_nils.empty()) return false;

  // Only worth doing if we can remove at least one redundant load.
  bool has_redundancy = false;
  for (const auto& kv : load_nils) {
    if (kv.second.size() > 1) {
      has_redundancy = true;
      break;
    }
  }
  if (!has_redundancy) return false;

  bool changed = false;
  OpBuilder* builder = ir_ctx_ ? ir_ctx_->GetOpBuilder() : nullptr;
  if (LEPUS_UNLIKELY(!builder)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: LoadNullEliminationPass expected OpBuilder");
  }

  DominanceInfo dom(f);
  llvh::SmallVector<Instruction*, 32> to_remove;

  // for each type, insert one canonical load at the nearest common dominator
  // block of all occurrences.
  for (auto& kv : load_nils) {
    int8_t t = kv.first;
    auto& loads = kv.second;
    if (loads.size() <= 1) continue;

    Block* common_dom = loads.front()->GetParent();
    for (auto* inst : loads) {
      common_dom =
          dom.findNearestCommonDominator(common_dom, inst->GetParent());
      if (!common_dom) break;
    }
    if (!common_dom) continue;

    // Insert canonical load early in the common dominator block.
    Block* pre_block = builder->GetBlock();
    auto pre_it = builder->GetInsertionPoint();

    builder->SetInsertionPoint(common_dom, FindBlockInsertionPoint(common_dom));
    auto* lit = builder->GetLiteralInt8(t);
    int64_t loc = loads.front()->GetLocation();
    auto* canon = builder->Create<LoadNullOrUndefinedInst>(loc, lit);

    builder->SetInsertionPoint(pre_block, pre_it);

    for (auto* inst : loads) {
      inst->ReplaceAllUsesWith(canon);
      to_remove.push_back(inst);
      changed = true;
    }
  }

  for (auto* inst : to_remove) {
    if (LEPUS_UNLIKELY(inst->HasUsers())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: LoadNullEliminationPass deletion still has users");
    }
    inst->EraseFromParent();
  }

  return changed;
}

Pass* CreateLoadNullEliminationPass(IRContext* ir_ctx) {
  return new LoadNullEliminationPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
