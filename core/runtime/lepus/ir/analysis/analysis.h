// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_ANALYSIS_ANALYSIS_H_
#define CORE_RUNTIME_LEPUS_IR_ANALYSIS_ANALYSIS_H_

#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/RecyclingAllocator.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/region_op.h"

namespace lynx {
namespace lepus {
namespace ir {
class IRContext;

/// Common base for post-order based block orderings.
///
/// This class only provides storage (`order_`) and iterator helpers.
/// The concrete derived analyses define:
/// - how the underlying traversal enumerates successors
/// - whether and how to incorporate catch blocks / layout constraints
class PostOrderAnalysisBase {
 protected:
  using BlockList = llvh::SmallVector<Block*, 16>;

  /// Holds the ordered list of basic blocks.
  BlockList order_;

 public:
  using iterator = BlockList::iterator;
  using const_iterator = BlockList::const_iterator;
  using reverse_iterator = BlockList::reverse_iterator;
  using const_reverse_iterator = BlockList::const_reverse_iterator;

  using range = llvh::iterator_range<iterator>;
  using const_range = llvh::iterator_range<const_iterator>;
  using reverse_range = llvh::iterator_range<reverse_iterator>;
  using const_reverse_range = llvh::iterator_range<const_reverse_iterator>;

  inline iterator begin() { return order_.begin(); }
  inline iterator end() { return order_.end(); }
  inline reverse_iterator rbegin() { return order_.rbegin(); }
  inline reverse_iterator rend() { return order_.rend(); }
  inline const_iterator begin() const { return order_.begin(); }
  inline const_iterator end() const { return order_.end(); }
  inline const_reverse_iterator rbegin() const { return order_.rbegin(); }
  inline const_reverse_iterator rend() const { return order_.rend(); }
};

/// This is an implementation of the post-order scan. We use our own
/// implementation and not the LLVM RPO analysis because we currently have
/// basic blocks that are not linked to the entry blocks (catch blocks),
/// and LLVM's graph traits expect all blocks to be reachable from the entry
/// blocks. The analysis does not enumerate unreachable blocks.
class PostOrderAnalysis : public PostOrderAnalysisBase {
  using BlockList = PostOrderAnalysisBase::BlockList;

  /// This function does the recursive scan of the function. \p bb is the basic
  /// block that starts the scan. \p order is the ordered list of blocks, and
  /// the output.
  static void VisitPostOrder(Block* bb, BlockList& order);

 public:
  explicit PostOrderAnalysis(FuncOp* f);
  explicit PostOrderAnalysis(Region* region);
};

/// PostOrderAnalysis2 is primarily used by instruction selection.
///
/// Differences vs PostOrderAnalysis:
/// - Successor traversal order is intentionally biased (see implementation) to
///   influence the resulting reverse-post-order emission order.
///   In particular, for conditional branches where successors are ordered as
///   [true, false], PostOrderAnalysis2 traverses successors in reverse so that
///   the emitted order (RPO) prefers the "true" (often treated as the left)
///   subtree first.
/// - When catch blocks exist, it may switch from graph-based traversal to the
///   region layout order so that instruction selection preserves VM exception
///   handling semantics that depend on catch-label placement in the bytecode.
class PostOrderAnalysis2 : public PostOrderAnalysisBase {
  using BlockList = PostOrderAnalysisBase::BlockList;

  /// This function does the recursive scan of the function. \p bb is the basic
  /// block that starts the scan. \p order_ is the ordered list of blocks, and
  /// the output.
  static void VisitPostOrder(Block* bb, BlockList& order);

 public:
  explicit PostOrderAnalysis2(FuncOp* f);
  explicit PostOrderAnalysis2(Region* region);
};

/// A namespace encapsulating utilities for implementing optimization passes
/// based on a DFS visit of a dominator tree.
namespace DomTreeDFS {

/// StackNode - contains the needed information to create a stack for doing
/// a depth first traversal of a dominator tree.
/// It can be subclassed to attach more information like scoped tables, etc.
class StackNode {
 public:
  StackNode(const StackNode&) = delete;
  void operator=(const StackNode&) = delete;

  StackNode(const DominanceInfoNode* n)
      : node_(n), child_iter_(n->begin()), end_iter_(n->end()), done_(false) {}
  /// A convenience constructor matching the signature of the derived class.
  StackNode(void*, const DominanceInfoNode* n) : StackNode(n) {}

  /// The dominator tree node associated with this stack node.
  const DominanceInfoNode* GetNode() { return node_; }

 private:
  template <typename Derived, typename StackNode>
  friend class Visitor;

  /// The dominator tree node associated with this stack node.
  const DominanceInfoNode* node_;
  /// The next child of the dominance tree node to process.
  DominanceInfoNode::const_iterator child_iter_;
  /// The end iterator of child dominance tree nodes.
  const DominanceInfoNode::const_iterator end_iter_;
  /// This flag indicates that this dominance tree node has been processed
  /// and we have moved onto iterating its children.
  bool done_;
};

/// A DFS visitor for nodes in a dominator tree. Derived needs to implement:
/// bool ProcessNode(StackNode &);
template <typename Derived, typename StackNode>
class Visitor {
  llvh::RecyclingAllocator<llvh::BumpPtrAllocator, StackNode> node_allocator_;

  Derived* GetDerived() { return static_cast<Derived*>(this); }

  StackNode* NewStackEntry(const DominanceInfoNode* n) {
    auto* sn = node_allocator_.Allocate();
    return new (sn) StackNode(GetDerived(), n);
  }
  void FreeStackEntry(StackNode* n) {
    n->~StackNode();
    node_allocator_.Deallocate(n);
  }

 protected:
  const DominanceInfo& dominance_info_;

  Visitor(const DominanceInfo& dt) : dominance_info_(dt) {}

  /// Starting DFS from root node.
  /// \return the changed flag.
  bool DFS() { return DFS(dominance_info_.getRootNode()); }

  /// Starting DFS from a specific node.
  /// \return the changed flag.
  bool DFS(const DominanceInfoNode* dominance_info_node) {
    llvh::SmallVector<StackNode*, 4> stack{};

    bool changed = false;

    // Process the root node.
    stack.push_back(NewStackEntry(dominance_info_node));

    // Process the stack.
    while (!stack.empty()) {
      // Grab the first item off the stack. Set the current generation, remove
      // the node from the stack, and process it.
      StackNode* to_process = stack.back();

      // Check if the node needs to be processed.
      if (!to_process->done_) {
        // Process the node.
        changed |= GetDerived()->ProcessNode(to_process);
        // This node has been processed.
        to_process->done_ = true;
      } else if (to_process->child_iter_ != to_process->end_iter_) {
        auto* dn = *to_process->child_iter_++;
        // Push the next child onto the stack.
        stack.push_back(NewStackEntry(dn));
      } else {
        // It has been processed, and there are no more children to process,
        // so pop it off the stack
        FreeStackEntry(stack.pop_back_val());
      }
    }

    return changed;
  }
};

}  // namespace DomTreeDFS

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_ANALYSIS_ANALYSIS_H_
