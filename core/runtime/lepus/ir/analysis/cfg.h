// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_ANALYSIS_CFG_H_
#define CORE_RUNTIME_LEPUS_IR_ANALYSIS_CFG_H_

#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {
//===----------------------------------------------------------------------===//
// Block pred_iterator definition
//===----------------------------------------------------------------------===//

template <class Ptr, class USER_iterator>  // Predecessor Iterator
class PredIterator {
  typedef PredIterator<Ptr, USER_iterator> Self;
  USER_iterator it_;
  USER_iterator it_end_;

  inline void AdvancePastNonTerminators() {
    // Loop to ignore non-terminator uses (for example BlockAddresses).
    while (it_ != it_end_ && !llvh::dyn_cast<TerminatorInst>(*it_)) ++it_;
  }

 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = Ptr;
  using difference_type = std::ptrdiff_t;
  using pointer = Ptr*;
  using reference = Ptr*;

  PredIterator() = default;

  explicit inline PredIterator(Ptr* bb)
      : it_(bb->UsersBegin()), it_end_(bb->UsersEnd()) {
    AdvancePastNonTerminators();
  }

  inline PredIterator(Ptr* bb, bool)
      : it_(bb->UsersEnd()), it_end_(bb->UsersEnd()) {}

  inline bool operator==(const Self& x) const { return it_ == x.it_; }

  inline bool operator!=(const Self& x) const { return !operator==(x); }

  inline reference operator*() const {
    if (LEPUS_UNLIKELY(it_ == it_end_)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: iterator out of range");
    }
    return llvh::cast<TerminatorInst>(*it_)->GetParent();
  }

  inline pointer* operator->() const { return &operator*(); }

  inline Self& operator++() {  // Preincrement
    if (LEPUS_UNLIKELY(it_ == it_end_)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: iterator out of range");
    }
    ++it_;
    AdvancePastNonTerminators();
    return *this;
  }

  inline Self operator++(int) {  // Postincrement
    Self tmp = *this;
    ++*this;
    return tmp;
  }
};

typedef PredIterator<Block, Value::iterator> pred_iterator;
typedef PredIterator<const Block, Value::const_iterator> const_pred_iterator;
using pred_range = llvh::iterator_range<pred_iterator>;
using pred_const_range = llvh::iterator_range<const_pred_iterator>;

inline pred_iterator PredBegin(Block* bb) { return pred_iterator(bb); }

inline const_pred_iterator PredBegin(const Block* bb) {
  return const_pred_iterator(bb);
}

inline pred_iterator PredEnd(Block* bb) { return pred_iterator(bb, true); }

inline const_pred_iterator PredEnd(const Block* bb) {
  return const_pred_iterator(bb, true);
}

inline bool PredEmpty(const Block* bb) { return PredBegin(bb) == PredEnd(bb); }

inline pred_range Predecessors(Block* bb) {
  return pred_range(PredBegin(bb), PredEnd(bb));
}

inline pred_const_range Predecessors(const Block* bb) {
  return pred_const_range(PredBegin(bb), PredEnd(bb));
}

inline bool PredContains(const Block* bb, const Block* search) {
  return std::find(PredBegin(bb), PredEnd(bb), search) != PredEnd(bb);
}

inline unsigned PredCount(const Block* bb) {
  return std::distance(PredBegin(bb), PredEnd(bb));
}

inline unsigned PredCountUnique(const Block* bb) {
  llvh::SmallPtrSet<const Block*, 8> predecessors(PredBegin(bb), PredEnd(bb));
  return predecessors.size();
}

//===----------------------------------------------------------------------===//
// Block succ_iterator helpers
//===----------------------------------------------------------------------===//

typedef llvh::SuccIterator<TerminatorInst, Block> succ_iterator;
typedef llvh::SuccIterator<const TerminatorInst, const Block>
    succ_const_iterator;
using SuccRange = llvh::iterator_range<succ_iterator>;
using SuccConstRange = llvh::iterator_range<succ_const_iterator>;

inline succ_iterator SuccBegin(Block* bb) {
  return succ_iterator(bb->GetTerminator());
}

inline succ_const_iterator SuccBegin(const Block* bb) {
  return succ_const_iterator(bb->GetTerminator());
}

inline succ_iterator SuccEnd(Block* bb) {
  return succ_iterator(bb->GetTerminator(), true);
}

inline succ_const_iterator SuccEnd(const Block* bb) {
  return succ_const_iterator(bb->GetTerminator(), true);
}

inline bool SuccEmpty(const Block* bb) { return SuccBegin(bb) == SuccEnd(bb); }

inline SuccRange Successors(Block* bb) {
  return SuccRange(SuccBegin(bb), SuccEnd(bb));
}

inline SuccConstRange Successors(const Block* bb) {
  return SuccConstRange(SuccBegin(bb), SuccEnd(bb));
}

inline bool SuccContains(const Block* bb, const Block* search) {
  return std::find(SuccBegin(bb), SuccEnd(bb), search) != SuccEnd(bb);
}

inline unsigned SuccCount(const Block* bb) {
  return std::distance(SuccBegin(bb), SuccEnd(bb));
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

namespace llvh {

//===----------------------------------------------------------------------===//
// GraphTraits for Block
//===----------------------------------------------------------------------===//
template <>
struct GraphTraits<lynx::lepus::ir::Block*> {
  using NodeType = lynx::lepus::ir::Block;
  using NodeRef = lynx::lepus::ir::Block*;

  using ChildIteratorType = lynx::lepus::ir::succ_iterator;

  static NodeType* getEntryNode(NodeType* bb) { return bb; }

  static ChildIteratorType child_begin(NodeType* n) { return SuccBegin(n); }
  static ChildIteratorType child_end(NodeType* n) { return SuccEnd(n); }
};

template <>
struct GraphTraits<Inverse<lynx::lepus::ir::Block*>> {
  using NodeType = lynx::lepus::ir::Block;
  using NodeRef = lynx::lepus::ir::Block*;

  using ChildIteratorType = lynx::lepus::ir::pred_iterator;

  static NodeType* getEntryNode(Inverse<NodeType*> G) { return G.Graph; }
  static inline ChildIteratorType child_begin(NodeType* n) {
    return PredBegin(n);
  }
  static inline ChildIteratorType child_end(NodeType* n) { return PredEnd(n); }
};

template <>
struct GraphTraits<lynx::lepus::ir::Region*> {
  using NodeType = lynx::lepus::ir::Block;
  using NodeRef = lynx::lepus::ir::Block*;
  using GraphType = lynx::lepus::ir::Region*;

  static NodeType* getEntryNode(GraphType f) { return f->GetEntryBlock(); }

  typedef pointer_iterator<lynx::lepus::ir::Region::iterator> nodes_iterator;
  static nodes_iterator nodes_begin(GraphType f) {
    return nodes_iterator(f->begin());
  }
  static nodes_iterator nodes_end(GraphType f) {
    return nodes_iterator(f->end());
  }
  static unsigned size(GraphType f) { return f->GetBlockSize(); }
};

template <>
struct GraphTraits<Inverse<lynx::lepus::ir::Region*>>
    : public GraphTraits<Inverse<lynx::lepus::ir::Block*>> {
  using GraphType = Inverse<lynx::lepus::ir::Region*>;

  static NodeType* getEntryNode(GraphType f) {
    return f.Graph->GetEntryBlock();
  }

  typedef pointer_iterator<lynx::lepus::ir::Region::iterator> nodes_iterator;
  static nodes_iterator nodes_begin(GraphType f) {
    return nodes_iterator(f.Graph->begin());
  }
  static nodes_iterator nodes_end(GraphType f) {
    return nodes_iterator(f.Graph->end());
  }
  static unsigned size(GraphType f) { return f.Graph->GetBlockSize(); }
};

template <>
struct GraphTraits<lynx::lepus::ir::FuncOp*>
    : public GraphTraits<lynx::lepus::ir::Block*> {
  using GraphType = lynx::lepus::ir::FuncOp*;

  static NodeType* getEntryNode(GraphType f) { return &f->Front(); }

  typedef pointer_iterator<lynx::lepus::ir::FuncOp::iterator> nodes_iterator;
  static nodes_iterator nodes_begin(GraphType f) {
    return nodes_iterator(f->begin());
  }
  static nodes_iterator nodes_end(GraphType f) {
    return nodes_iterator(f->end());
  }
  static unsigned size(GraphType f) { return f->size(); }
};

template <>
struct GraphTraits<Inverse<lynx::lepus::ir::FuncOp*>>
    : public GraphTraits<Inverse<lynx::lepus::ir::Block*>> {
  using GraphType = Inverse<lynx::lepus::ir::FuncOp*>;

  static NodeType* getEntryNode(GraphType f) { return &f.Graph->Front(); }

  typedef pointer_iterator<lynx::lepus::ir::FuncOp::iterator> nodes_iterator;
  static nodes_iterator nodes_begin(GraphType f) {
    return nodes_iterator(f.Graph->begin());
  }
  static nodes_iterator nodes_end(GraphType f) {
    return nodes_iterator(f.Graph->end());
  }
  static unsigned size(GraphType f) { return f.Graph->size(); }
};

}  // namespace llvh

//===----------------------------------------------------------------------===//
// Dominators
//===----------------------------------------------------------------------===//

extern template class llvh::DominatorTreeBase<lynx::lepus::ir::Block, false>;
extern template class llvh::DomTreeNodeBase<lynx::lepus::ir::Block>;

namespace lynx {
namespace lepus {
namespace ir {

using DominanceInfoNode = llvh::DomTreeNodeBase<Block>;

/// A class for computing basic dominance info.
class DominanceInfo : public llvh::DominatorTreeBase<Block, false> {
 public:
  explicit DominanceInfo(FuncOp* f);

  bool ProperlyDominates(const Instruction* a, const Instruction* b) const;

  bool ProperlyDominates(const Block* a, const Block* b) const {
    return properlyDominates(a, b);
  }

  using DominatorTreeBase::properlyDominates;

  void Reset() { llvh::DominatorTreeBase<Block, false>::reset(); }
};

class RegionDominanceInfo : public llvh::DominatorTreeBase<Block, false> {
 public:
  explicit RegionDominanceInfo(Region* r);
  bool ProperlyDominates(const Instruction* a, const Instruction* b) const;

  bool ProperlyDominates(const Block* a, const Block* b) const {
    return properlyDominates(a, b);
  }

  using DominatorTreeBase::properlyDominates;
  void Reset() { llvh::DominatorTreeBase<Block, false>::reset(); }

  Block* FindRegionNearestCommonDominator(Block* a, Block* b) const;
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_ANALYSIS_CFG_H_
