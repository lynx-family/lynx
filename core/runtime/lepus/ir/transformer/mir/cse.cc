// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/cse.h"

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/Hashing.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ScopedHashTable.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/RecyclingAllocator.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

//===----------------------------------------------------------------------===//
//                                Simple Value
//===----------------------------------------------------------------------===//

namespace lynx {
namespace lepus {
namespace ir {

/// CSEValue - Instances of this struct represent available values in the
/// scoped hash table.
struct CSEValue {
  Instruction* inst_;

  CSEValue(Instruction* i) : inst_(i) {
    if (LEPUS_UNLIKELY(!(IsSentinel() || CanHandle(i)))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: CSEValue constructed with instruction that cannot "
          "be CSE'd");
    }
  }

  bool IsSentinel() const {
    return inst_ == llvh::DenseMapInfo<Instruction*>::getEmptyKey() ||
           inst_ == llvh::DenseMapInfo<Instruction*>::getTombstoneKey();
  }

  /// Return true if we know how to CSE this instruction.
  static bool CanHandle(Instruction* inst) {
    // Check that the instruction can be freely reordered and deduplicated,
    // and that it is not a terminator.
    if (llvh::isa<TerminatorInst>(inst)) return false;

    // Instructions with ClosureVarReg attribute cannot be CSE'd
    // because they must always read from the original toplevel register.
    if (inst->GetClosureVarReg() != constants::kInvalidSignedValue ||
        inst->GetToplevelVarReg() != constants::kInvalidSignedValue)
      return false;

    // LoadConstInst and GetBuiltinInst are always safe to CSE as they read from
    // read-only pools.
    if (llvh::isa<LoadConstInst>(inst) || llvh::isa<GetBuiltinInst>(inst))
      return true;

    return inst->GetSideEffect().IsPure() &&
           !inst->GetSideEffect().GetFirstInBlock();
  }
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

namespace llvh {
template <>
struct DenseMapInfo<lynx::lepus::ir::CSEValue> {
  static inline lynx::lepus::ir::CSEValue getEmptyKey() {
    return DenseMapInfo<lynx::lepus::ir::Instruction*>::getEmptyKey();
  }
  static inline lynx::lepus::ir::CSEValue getTombstoneKey() {
    return DenseMapInfo<lynx::lepus::ir::Instruction*>::getTombstoneKey();
  }
  static unsigned getHashValue(lynx::lepus::ir::CSEValue val);
  static bool isEqual(lynx::lepus::ir::CSEValue lhs,
                      lynx::lepus::ir::CSEValue rhs);
};

}  // namespace llvh

unsigned llvh::DenseMapInfo<lynx::lepus::ir::CSEValue>::getHashValue(
    lynx::lepus::ir::CSEValue val) {
  return val.inst_->GetHashCode();
}

bool llvh::DenseMapInfo<lynx::lepus::ir::CSEValue>::isEqual(
    lynx::lepus::ir::CSEValue lhs, lynx::lepus::ir::CSEValue rhs) {
  lynx::lepus::ir::Instruction* lhs_inst = lhs.inst_;
  lynx::lepus::ir::Instruction* rhs_inst = rhs.inst_;
  if (lhs.IsSentinel() || rhs.IsSentinel()) {
    return lhs_inst == rhs_inst;
  }

  return lhs_inst->GetKind() == rhs_inst->GetKind() &&
         lhs_inst->IsIdenticalTo(rhs_inst);
}
//===----------------------------------------------------------------------===//
//                               CSE Interface
//===----------------------------------------------------------------------===//

namespace lynx {
namespace lepus {
namespace ir {

class CSEContext;

using CSEValueHTType = llvh::ScopedHashTableVal<CSEValue, Value*>;
using AllocatorTy =
    llvh::RecyclingAllocator<llvh::BumpPtrAllocator, CSEValueHTType>;
using ScopedHTType =
    llvh::ScopedHashTable<CSEValue, Value*, llvh::DenseMapInfo<CSEValue>,
                          AllocatorTy>;

// StackNode - contains all the needed information to create a stack for doing
// a depth first traversal of the tree. This includes scopes for values and
// loads as well as the generation. There is a child iterator so that the
// children do not need to be stored separately.
class StackNode : public DomTreeDFS::StackNode {
 public:
  inline StackNode(CSEContext* ctx, const DominanceInfoNode* n);

 private:
  /// RAII to create and pop a scope when the stack node is created and
  /// destroyed.
  ScopedHTType::ScopeTy scope_;
};

/// CSEContext - This pass does a simple depth-first walk of the dominator
/// tree, eliminating trivially redundant instructions.
class CSEContext : public DomTreeDFS::Visitor<CSEContext, StackNode> {
 public:
  CSEContext(const DominanceInfo& dt)
      : DomTreeDFS::Visitor<CSEContext, StackNode>(dt) {}

  bool Run() { return DFS(); }

  bool ProcessNode(StackNode* sn);

 private:
  friend StackNode;

  /// AvailableValues - This scoped hash table contains the current values of
  /// all of our simple scalar expressions.  As we walk down the domtree, we
  /// look to see if instructions are in this. If so, we replace them with what
  /// we find, otherwise we insert them so that dominated values can succeed in
  /// their lookup.
  ScopedHTType available_values_{};
};

inline StackNode::StackNode(CSEContext* ctx, const DominanceInfoNode* n)
    : DomTreeDFS::StackNode(n), scope_{ctx->available_values_} {}

//===----------------------------------------------------------------------===//
//                             CSE Implementation
//===----------------------------------------------------------------------===//

bool CSEContext::ProcessNode(StackNode* stack_node) {
  Block* bb = stack_node->GetNode()->getBlock();
  bool changed = false;

  // Keep a list of instructions that should be deleted when the basic block
  // is processed.
  InstructionDestroyer destroyer;

  // See if any instructions in the block can be eliminated.  If so, do it.  If
  // not, add them to AvailableValues.
  for (auto* inst : bb->InstRange()) {
    // If this is not a simple instruction that we can value number, skip it.
    if (!CSEValue::CanHandle(inst)) {
      continue;
    }

    // Now that we know we have an instruction we understand see if the
    // instruction has an available value.  If so, use it.
    if (Value* value = available_values_.lookup(inst)) {
      inst->ReplaceAllUsesWith(value);
      destroyer.Add(inst);
      changed = true;
      continue;
    }

    // Otherwise, just remember that this value is available.
    available_values_.insert(inst, inst);
  }

  return changed;
}

bool CSE::RunOnFunction(FuncOp* f) {
  DominanceInfo dt{f};
  CSEContext cse_ctx{dt};
  return cse_ctx.Run();
}

Pass* CreateCSE(IRContext* ir_ctx) { return new CSE(ir_ctx); }

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
