// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/analysis/cfg.h"

#include <utility>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/instruction.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/GenericDomTree.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/GenericDomTreeConstruction.h"

template class llvh::DominatorTreeBase<lynx::lepus::ir::Block, false>;
template class llvh::DomTreeNodeBase<lynx::lepus::ir::Block>;
namespace lynx {
namespace lepus {
namespace ir {

DominanceInfo::DominanceInfo(FuncOp* func) : DominatorTreeBase() {
  if (LEPUS_UNLIKELY(func->begin() == func->end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: DominanceInfo constructed with empty function");
  }
  recalculate(*func->GetSingleRegion());
}

bool DominanceInfo::ProperlyDominates(const Instruction* a,
                                      const Instruction* b) const {
  auto* a_bb = a->GetParent();
  auto* b_bb = b->GetParent();

  if (a_bb != b_bb) {
    return ProperlyDominates(a_bb, b_bb);
  }

  // Otherwise, they're in the same block, and we just need to check
  // whether B comes after A.
  auto it_a = Block::const_iterator(a);
  auto it_b = Block::const_iterator(b);
  auto begin = a_bb->begin();
  while (it_b != begin) {
    --it_b;
    if (it_a == it_b) {
      return true;
    }
  }

  return false;
}

RegionDominanceInfo::RegionDominanceInfo(Region* region) : DominatorTreeBase() {
  if (LEPUS_UNLIKELY(region->begin() == region->end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegionDominanceInfo constructed with empty region");
  }
  recalculate(*region);
}

bool RegionDominanceInfo::ProperlyDominates(const Instruction* a,
                                            const Instruction* b) const {
  auto* a_bb = a->GetParent();
  auto* b_bb = b->GetParent();

  if (a_bb != b_bb) {
    return ProperlyDominates(a_bb, b_bb);
  }

  // Otherwise, they're in the same block, and we just need to check
  // whether B comes after A.
  auto it_a = Block::const_iterator(a);
  auto it_b = Block::const_iterator(b);
  auto begin = a_bb->begin();
  while (it_b != begin) {
    --it_b;
    if (it_a == it_b) {
      return true;
    }
  }

  return false;
}

Block* lynx::lepus::ir::RegionDominanceInfo::FindRegionNearestCommonDominator(
    Block* a, Block* b) const {
  if (LEPUS_UNLIKELY(!a || !b)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FindRegionNearestCommonDominator called with nullptr "
        "block");
  }
  if (LEPUS_UNLIKELY(a->GetParentOp() != b->GetParentOp())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FindRegionNearestCommonDominator requires blocks in "
        "the same function/module");
  }

  // If either A or B is a entry block then it is nearest common dominator
  // (for forward-dominators).
  if (!isPostDominator()) {
    Block& entry = a->GetParent()->Front();
    if (a == &entry || b == &entry) {
      return &entry;
    }
  }

  auto node_a = getNode(a);
  auto node_b = getNode(b);

  if (!node_a || !node_b) {
    return nullptr;
  }

  // Use level information to go up the tree until the levels match. Then
  // continue going up til we arrive at the same node.
  while (node_a && node_a != node_b) {
    if (node_a->getLevel() < node_b->getLevel()) {
      std::swap(node_a, node_b);
    }

    node_a = node_a->getIDom();
  }

  return node_a ? node_a->getBlock() : nullptr;
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
