// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/cse.h"

#include <set>
#include <utility>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
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

    // Idempotent calls (parseFloat, parseInt, isNaN) are pure: same inputs
    // produce the same output with no observable side effects.
    if (auto* call = llvh::dyn_cast<CallInst>(inst)) {
      if (call->IsIdempotentCall()) return true;
    }

    // GetToplevelClosureVarInst and GetUpvalueInst can be CSE'd when the
    // target register/slot is proven never-written across the entire module.
    // The never-written check is performed in ProcessNode; here we just
    // allow them as valid hash table keys.
    if (llvh::isa<GetToplevelClosureVarInst>(inst) ||
        llvh::isa<GetUpvalueInst>(inst))
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
  CSEContext(const DominanceInfo& dt, FuncOp* func,
             const std::set<uint32_t>& never_written_toplevel_closure_regs,
             const std::set<uint8_t>& never_written_upvalue_indices)
      : DomTreeDFS::Visitor<CSEContext, StackNode>(dt),
        never_written_toplevel_closure_regs_(
            never_written_toplevel_closure_regs),
        never_written_upvalue_indices_(never_written_upvalue_indices) {}

  bool Run() { return DFS(); }

  bool ProcessNode(StackNode* sn);

 private:
  friend StackNode;

  const std::set<uint32_t>& never_written_toplevel_closure_regs_;
  const std::set<uint8_t>& never_written_upvalue_indices_;

  /// AvailableValues - This scoped hash table contains the current values of
  /// all of our simple scalar expressions.  As we walk down the domtree, we
  /// look to see if instructions are in this. If so, we replace them with what
  /// we find, otherwise we insert them so that dominated values can succeed in
  /// their lookup.
  ScopedHTType available_values_{};

  /// Check if a never-written load instruction should be CSE'd.
  bool IsNeverWrittenLoad(Instruction* inst) const;
};

inline StackNode::StackNode(CSEContext* ctx, const DominanceInfoNode* n)
    : DomTreeDFS::StackNode(n), scope_{ctx->available_values_} {}

//===----------------------------------------------------------------------===//
//                             CSE Implementation
//===----------------------------------------------------------------------===//

bool CSEContext::IsNeverWrittenLoad(Instruction* inst) const {
  if (auto* get_inst = llvh::dyn_cast<GetToplevelClosureVarInst>(inst)) {
    auto* reg = llvh::dyn_cast<LiteralUint32>(get_inst->GetClosureReg());
    return reg &&
           never_written_toplevel_closure_regs_.count(reg->GetValue()) > 0;
  }
  if (auto* get_inst = llvh::dyn_cast<GetUpvalueInst>(inst)) {
    auto* idx = llvh::dyn_cast<LiteralUint8>(get_inst->GetIndex());
    return idx && never_written_upvalue_indices_.count(idx->GetValue()) > 0;
  }
  return false;
}

bool CSEContext::ProcessNode(StackNode* stack_node) {
  Block* bb = stack_node->GetNode()->getBlock();
  bool changed = false;

  // Keep a list of instructions that should be deleted when the basic block
  // is processed.
  InstructionDestroyer destroyer;

  // Local map for closure/upvalue loads.  Cleared when a non-pure call is
  // encountered, because C functions may modify toplevel closure variables
  // at runtime (e.g. via VMContext::UpdateTopLevelVariable).
  llvh::DenseMap<CSEValue, Value*> available_closure_loads;

  // See if any instructions in the block can be eliminated.  If so, do it.  If
  // not, add them to AvailableValues.
  for (auto* inst : bb->InstRange()) {
    // Non-pure, non-readonly calls invalidate toplevel closure load caches.
    // Upvalue loads are safe: no C function API can modify upvalue slots.
    if (auto* call = llvh::dyn_cast<CallInst>(inst)) {
      if (!call->IsIdempotentCall() && !call->IsReadonlyCall()) {
        // Only clear toplevel closure loads; upvalue loads are safe since
        // no C function API can modify upvalue slots.
        llvh::SmallVector<CSEValue, 4> to_erase;
        for (auto& kv : available_closure_loads) {
          if (llvh::isa<GetToplevelClosureVarInst>(kv.second)) {
            to_erase.push_back(kv.first);
          }
        }
        for (auto& key : to_erase) {
          available_closure_loads.erase(key);
        }
      }
    }

    // If this is not a simple instruction that we can value number, skip it.
    if (!CSEValue::CanHandle(inst)) {
      continue;
    }

    // For GetToplevelClosureVarInst / GetUpvalueInst, use the local map
    // (which is invalidated on non-pure calls) instead of the scoped table.
    if (llvh::isa<GetToplevelClosureVarInst>(inst) ||
        llvh::isa<GetUpvalueInst>(inst)) {
      if (!IsNeverWrittenLoad(inst)) {
        continue;
      }
      auto it = available_closure_loads.find(CSEValue(inst));
      if (it != available_closure_loads.end()) {
        inst->ReplaceAllUsesWith(it->second);
        destroyer.Add(inst);
        changed = true;
      } else {
        available_closure_loads.try_emplace(CSEValue(inst),
                                            static_cast<Value*>(inst));
      }
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
  auto& never_written_toplevel = ir_ctx_->GetNeverWrittenToplevelClosureRegs();
  auto& written_upvalue_names = ir_ctx_->GetWrittenUpvalueNames();
  auto never_written_upvalue_indices =
      ComputeNeverWrittenUpvalueIndices(written_upvalue_names, f);

  DominanceInfo dt{f};
  CSEContext cse_ctx{dt, f, never_written_toplevel,
                     never_written_upvalue_indices};
  return cse_ctx.Run();
}

Pass* CreateCSE(IRContext* ir_ctx) { return new CSE(ir_ctx); }

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
