// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/analysis/analysis.h"

#include <algorithm>
#include <string>

#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

// Helpers local to this translation unit.
//
// Keep these as `static` functions instead of an anonymous namespace to follow
// the project's style preference.

using BlockList = llvh::SmallVector<Block*, 16>;
using VisitPostOrderFn = void (*)(Block*, BlockList&);

static void AssertOrderEmpty(const char* name, const BlockList& order) {
  if (LEPUS_UNLIKELY(!order.empty())) {
    const std::string msg = std::string("Lepus IR error: ") + name +
                            " expects empty order_ in the beginning";
    throw ::lynx::lepus::CompileException(msg.c_str());
  }
}

static void AssertEntryIsLast(const char* name, const BlockList& order,
                              Block* entry) {
  if (LEPUS_UNLIKELY(order.empty() || order.back() != entry)) {
    const std::string msg = std::string("Lepus IR error: ") + name +
                            " expects entry block to be the last element";
    throw ::lynx::lepus::CompileException(msg.c_str());
  }
}

static bool IsCatchEntryBlock(Block* b, Block* entry) {
  if (b == entry) return false;
  if (b->empty()) return false;
  return b->Front()->GetKind() == ValueKind::CatchInstKind;
}

static bool RegionHasCatchBlocks(Region* region, Block* entry) {
  for (auto& bb : *region) {
    if (IsCatchEntryBlock(&bb, entry)) {
      return true;
    }
  }
  return false;
}

/// Ensure `entry` is placed as the last element of `order`.
///
/// Many passes treat `rbegin()..rend()` as the *emission* (RPO) order and rely
/// on the entry block being the last element in post-order.
static void EnsureEntryIsLast(BlockList& order, Block* entry) {
  auto it = std::find(order.begin(), order.end(), entry);
  if (it != order.end() && (it + 1) != order.end()) {
    std::iter_swap(it, order.end() - 1);
  }
}

/// Append subgraphs reachable from catch entry blocks that are not reachable
/// from `entry` via explicit IR edges.
///
/// In Lepus VM, the runtime may transfer control to catch blocks without an
/// explicit IR CFG edge (e.g. via exception dispatch). Some optimization passes
/// (notably reg-alloc) still need to visit these blocks to keep them valid.
static void AppendUnreachableCatchSubgraphs(Region* region, Block* entry,
                                            VisitPostOrderFn visit_postorder,
                                            BlockList& order) {
  llvh::SmallPtrSet<Block*, 32> seen;
  seen.insert(order.begin(), order.end());

  BlockList extra;
  for (auto& bb : *region) {
    auto* b = &bb;
    if (!IsCatchEntryBlock(b, entry)) continue;
    if (seen.count(b)) continue;

    BlockList local;
    visit_postorder(b, local);
    for (auto* x : local) {
      if (x == entry) continue;
      if (seen.insert(x).second) {
        extra.push_back(x);
      }
    }
  }

  if (!extra.empty()) {
    // Keep the entry block as the last element.
    order.insert(order.end() - 1, extra.begin(), extra.end());
  }
}

void PostOrderAnalysis::VisitPostOrder(Block* bb, BlockList& order) {
  // Standard iterative DFS post-order traversal.
  //
  // This variant uses the CFG successor iterators from `cfg.h` and therefore
  // follows the canonical successor order as encoded in the IR terminators.
  struct State {
    Block* bb;
    succ_iterator cur, end;
    explicit State(Block* bb) : bb(bb), cur(SuccBegin(bb)), end(SuccEnd(bb)) {}
  };

  llvh::SmallPtrSet<Block*, 16> visited{};
  llvh::SmallVector<State, 32> stack{};

  stack.emplace_back(bb);
  do {
    while (stack.back().cur != stack.back().end) {
      bb = *stack.back().cur++;
      if (visited.insert(bb).second) stack.emplace_back(bb);
    }

    order.push_back(stack.back().bb);
    stack.pop_back();
  } while (!stack.empty());
}

PostOrderAnalysis::PostOrderAnalysis(FuncOp* func)
    : PostOrderAnalysis(func->GetSingleRegion()) {}

PostOrderAnalysis::PostOrderAnalysis(Region* region) {
  AssertOrderEmpty("PostOrderAnalysis", order_);
  Block* entry = region->GetEntryBlock();

  // Finally, do an PO scan from the entry block.
  VisitPostOrder(entry, order_);

  // The VM may transfer control to catch blocks (TypeLabel_Catch) without an
  // explicit IR edge. Make sure these blocks and their subgraphs are included
  // in the traversal order so that optimization passes (e.g. reg-alloc) won't
  // drop or ignore them.
  AppendUnreachableCatchSubgraphs(region, entry,
                                  PostOrderAnalysis::VisitPostOrder, order_);

  AssertEntryIsLast("PostOrderAnalysis", order_, entry);
}

void PostOrderAnalysis2::VisitPostOrder(Block* bb, BlockList& order) {
  struct State {
    Block* bb;
    using VecType = llvh::SmallVector<Block*, 4>;
    VecType succ_vec;
    explicit State(Block* bb) : bb(bb) {
      if (LEPUS_UNLIKELY(bb->GetTerminator() == nullptr)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: PostOrderAnalysis2 requires each block to have a "
            "terminator");
      }
      for (auto succ : bb->GetTerminator()->Successors()) {
        succ_vec.push_back(succ);
      }
    }

    // Index of the next successor to visit (from the end towards the start).
    //
    // We intentionally traverse successors in *reverse* here.
    //
    // Important detail: for our conditional terminators, `GetSuccessorImpl(0)`
    // is the "true" destination and `GetSuccessorImpl(1)` is the "false"
    // destination (see `CondBranchInst/EqCondBranchInst/NeqCondBranchInst`).
    // That means `TerminatorInst::Successors()` enumerates successors in
    // [true, false] order.
    //
    // Instruction selection consumes `PO.rbegin()..PO.rend()` as the *emission*
    // order (reverse post-order). To make the emitted order prefer the "left"
    // (true) subtree first, we must visit the "right" (false) successor first
    // during the DFS so that the true subtree appears earlier in RPO.
    // NOTE: We intentionally do not store iterators into `succ_vec` because
    // `State` objects are stored in a `SmallVector` stack and may be moved when
    // the stack grows. Moving would invalidate iterators and cause the DFS to
    // miss blocks (which later breaks relocation resolution in isel).
    int NextSuccIdx() const { return static_cast<int>(succ_vec.size()) - 1; }
  };

  llvh::SmallPtrSet<Block*, 16> visited{};
  llvh::SmallVector<State, 32> stack{};

  // Track per-frame successor index separately from State to avoid iterator
  // invalidation and to keep State trivially movable.
  llvh::SmallVector<int, 32> succ_idx_stack{};

  stack.emplace_back(bb);
  succ_idx_stack.push_back(stack.back().NextSuccIdx());
  do {
    while (succ_idx_stack.back() >= 0) {
      bb = stack.back().succ_vec[succ_idx_stack.back()--];
      if (visited.insert(bb).second) {
        stack.emplace_back(bb);
        succ_idx_stack.push_back(stack.back().NextSuccIdx());
      }
    }

    order.push_back(stack.back().bb);
    stack.pop_back();
    succ_idx_stack.pop_back();
  } while (!stack.empty());
}

PostOrderAnalysis2::PostOrderAnalysis2(FuncOp* func)
    : PostOrderAnalysis2(func->GetSingleRegion()) {}

PostOrderAnalysis2::PostOrderAnalysis2(Region* region) {
  AssertOrderEmpty("PostOrderAnalysis2", order_);

  Block* entry = region->GetEntryBlock();

  // Exception handling in Lepus VM relies on the *physical placement* of
  // `TypeLabel_Catch` blocks in the bytecode stream. The VM scans forward from
  // the throwing PC to find the first catch label, so instruction selection
  // must preserve the original layout order of blocks when catch blocks exist.
  //
  // When there are no catch blocks, keep the original behavior: emit blocks in
  // reverse-post-order (a simple topological sort) for better fall-through.
  if (!RegionHasCatchBlocks(region, entry)) {
    // Finally, do an PO scan from the entry block.
    VisitPostOrder(entry, order_);
    AssertEntryIsLast("PostOrderAnalysis2", order_, entry);
    return;
  }

  // Use the region (layout) order for codegen. The instruction selection pass
  // uses `PO.rbegin()..PO.rend()` as the emission order, so store the reverse
  // layout here to make the final emitted order equal to the layout order.
  order_.reserve(region->GetBlockSize());
  for (auto& bb : *region) {
    order_.push_back(&bb);
  }
  std::reverse(order_.begin(), order_.end());

  // Ensure entry is the last element.
  EnsureEntryIsLast(order_, entry);

  AssertEntryIsLast("PostOrderAnalysis2", order_, entry);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
