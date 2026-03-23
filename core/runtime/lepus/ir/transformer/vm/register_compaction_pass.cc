// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/register_compaction_pass.h"

#include <algorithm>

#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

static unsigned ComputePrefixRegisterCount(IRContext* ir_ctx, FuncOp* func) {
  if (!func) return 0;
  if (func->IsToplevelFunc()) {
    // Toplevel function has no parameters and preallocates all toplevel vars.
    return static_cast<unsigned>(ir_ctx->GetToplevelVariables().size());
  }
  return static_cast<unsigned>(func->GetParamSize());
}

bool RegisterCompactionPass::RunOnFunction(FuncOp* func) {
  RegisterAllocator* ra =
      ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(func);
  if (!ra) return false;

  const unsigned prefix = ComputePrefixRegisterCount(ir_ctx_, func);

  // Collect used physical registers in the remappable range [prefix, +inf).
  llvh::SmallDenseSet<unsigned, 64> seen;
  llvh::SmallVector<unsigned, 64> used_sorted;
  for (const auto& kv : ra->GetAllocatedMap()) {
    Register r = kv.second;
    if (!r.IsValid()) continue;
    unsigned idx = r.GetIndex();
    if (idx < prefix) continue;
    if (seen.insert(idx).second) {
      used_sorted.push_back(idx);
    }
  }

  if (used_sorted.empty()) {
    // Still rebuild the register file in case earlier post-RA passes changed
    // mappings without updating the internal RegisterFile bitmap.
    ra->RebuildRegisterFileFromAllocated();
    return false;
  }

  std::sort(used_sorted.begin(), used_sorted.end());

  // Build a monotonic old->new mapping that preserves register order.
  llvh::DenseMap<unsigned, unsigned> remap;
  remap.reserve(used_sorted.size());
  unsigned next = prefix;
  for (unsigned old_reg : used_sorted) {
    remap[old_reg] = next++;
  }

  bool changed = false;
  for (auto& kv : ra->GetAllocatedMap()) {
    Register r = kv.second;
    if (!r.IsValid()) continue;
    unsigned idx = r.GetIndex();
    if (idx < prefix) continue;
    auto it = remap.find(idx);
    if (it == remap.end()) continue;
    unsigned new_idx = it->second;
    if (new_idx != idx) {
      kv.second = Register(new_idx);
      changed = true;
    }
  }

  // Keep `GetMaxRegisterUsage()` consistent with the remapped registers.
  ra->RebuildRegisterFileFromAllocated();

  return changed;
}

Pass* CreateRegisterCompactionPass(IRContext* ir_ctx) {
  return new RegisterCompactionPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
