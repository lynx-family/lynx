// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/inst_combine/inst_combine.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallPtrSet.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/utils/block_utils.h"
#include "core/runtime/lepus/ir/utils/eval.h"
#include "core/runtime/lepus/restricted_value.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

// Resolve whether a MIR value is *proven* to be a const-string key coming from
// LepusFunction constant table.
//
// This is intentionally conservative:
// - Only accepts keys that trace back to `LoadConstInst` for a string constant.
// - Supports simple SSA propagation through `PhiInst`.
// - Rejects cycles (phi self loops) and mixed constants.
//
// Why do we need this?
// - In real-world code, the property key often flows through Phi nodes / copy
//   chains, so a pure pattern match `GetTableInst(obj, LoadConstInst)` misses a
//   lot of cases.
// - Once we recover the const string index, we can rewrite:
//     GetTableInst(obj, key) -> GetTableConstStringKeyInst(obj, const_idx)
//   and similarly for SetTable.
//   This enables subsequent passes (e.g. LSE) to reason about table keys more
//   precisely, and helps instruction selection emit `GetTableConstString`.

// Try to resolve `v` to a const-string key index (0..255) in lepus constant
// table.
//
// Returns true if success and writes index into `out_index`.
// Returns false if not proven.
static bool ResolveConstStringKeyIndex(
    lepus::Function* lepus_func, Value* v, int32_t* out_index,
    llvh::DenseMap<Value*, int32_t>& memo,
    llvh::SmallPtrSet<Value*, 16>& visiting) {
  if (!lepus_func || !v || !out_index) return false;

  auto it = memo.find(v);
  if (it != memo.end()) {
    if (it->second == -1) return false;
    *out_index = it->second;
    return true;
  }

  // Break potential cycles (e.g. ill-formed phi).
  if (!visiting.insert(v).second) {
    memo[v] = -1;
    return false;
  }

  int32_t result = -1;

  if (auto* lc = llvh::dyn_cast<LoadConstInst>(v)) {
    // Only accept const-table string keys.
    auto* lit = llvh::dyn_cast<LiteralUint32>(lc->GetConst());
    if (lit) {
      const uint32_t idx = lit->GetValue();
      // Current bytecode encoding for const-string key uses 8-bit indices.
      // If the index cannot fit, keep the generic GetTable/SetTable.
      if (idx <= 255) {
        auto* cv = lepus_func->GetConstValue(idx);
        if (cv && cv->IsString()) {
          result = static_cast<int32_t>(idx);
        }
      }
    }
  } else if (auto* phi = llvh::dyn_cast<PhiInst>(v)) {
    // All incoming values must resolve to the same const-string index.
    const unsigned n = phi->GetNumEntries();
    if (n > 0) {
      bool ok = true;
      int32_t merged = -1;
      for (unsigned i = 0; i < n; i++) {
        auto entry = phi->GetEntry(i);
        Value* in_val = entry.first;
        int32_t cur = -1;
        if (!ResolveConstStringKeyIndex(lepus_func, in_val, &cur, memo,
                                        visiting)) {
          ok = false;
          break;
        }
        if (merged == -1) {
          merged = cur;
        } else if (merged != cur) {
          ok = false;
          break;
        }
      }
      if (ok) result = merged;
    }
  }

  visiting.erase(v);
  memo[v] = result;
  if (result == -1) return false;
  *out_index = result;
  return true;
}

static bool ResolveFreshTableFromNewTable(
    Value* v, llvh::DenseMap<Value*, bool>& memo,
    llvh::SmallPtrSet<Value*, 16>& visiting) {
  if (!v) return false;

  auto it = memo.find(v);
  if (it != memo.end()) return it->second;

  // Break cycles (e.g. ill-formed phi).
  if (!visiting.insert(v).second) {
    memo[v] = false;
    return false;
  }

  bool result = false;
  if (llvh::isa<NewTableInst>(v)) {
    // NewTableInst always materializes a table at runtime, even if its IR type
    // has been widened to `any` by other passes.
    result = true;
  } else if (auto* phi = llvh::dyn_cast<PhiInst>(v)) {
    const unsigned n = phi->GetNumEntries();
    if (n > 0) {
      result = true;
      for (unsigned i = 0; i < n; i++) {
        auto entry = phi->GetEntry(i);
        if (!ResolveFreshTableFromNewTable(entry.first, memo, visiting)) {
          result = false;
          break;
        }
      }
    }
  }

  visiting.erase(v);
  memo[v] = result;
  return result;
}

static Value* CombineSetTableConstStringKey(OpBuilder* builder,
                                            Instruction* inst) {
  // loadConst
  // setTable obj, loadConst, value
  // ->
  // setTableConstStringKey obj, literal_index, value
  if (auto* set_table = llvh::dyn_cast<SetTableInst>(inst)) {
    auto* obj = set_table->GetObject();
    // `TypeOp_SetObjectConstString` assumes the receiver is a table (no runtime
    // check). Be conservative:
    // - If IR type already proves `table`, ok.
    // - Otherwise, only allow when the receiver can be proven to originate from
    //   a fresh NewTableInst through Phi chains.
    if (!obj || !obj->GetType()) return nullptr;
    if (!obj->GetType()->IsTableType()) {
      llvh::DenseMap<Value*, bool> table_memo;
      llvh::SmallPtrSet<Value*, 16> table_visiting;
      if (!ResolveFreshTableFromNewTable(obj, table_memo, table_visiting)) {
        return nullptr;
      }
    }

    // Resolve key to a const-string index through Phi chains.
    auto* func_op = inst->GetFunction();
    auto lepus_func = func_op ? func_op->GetLepusFunction().get() : nullptr;
    llvh::DenseMap<Value*, int32_t> memo;
    llvh::SmallPtrSet<Value*, 16> visiting;
    int32_t key_idx = -1;
    if (!ResolveConstStringKeyIndex(lepus_func, set_table->GetProp(), &key_idx,
                                    memo, visiting)) {
      return nullptr;
    }

    builder->SetInsertionPointAfter(set_table);
    auto* res = builder->Create<SetTableConstStringKeyInst>(
        set_table->GetLocation(), obj,
        builder->GetLiteralUint32(static_cast<uint32_t>(key_idx)),
        set_table->GetStoreVal());
    return res;
  }
  return nullptr;
}

static Value* CombineGetTableConstStringKey(OpBuilder* builder,
                                            Instruction* inst) {
  // loadConst(string)
  // getTable any, loadConst
  // ->
  // getTableConstStringKey any, literal_index
  auto* get_table = llvh::dyn_cast<GetTableInst>(inst);
  if (!get_table) return nullptr;

  auto* obj = get_table->GetObject();
  if (!obj || !obj->GetType()) return nullptr;
  // Be permissive here: many objects are typed as `any` even if they are
  // actually tables at runtime.
  if (!obj->GetType()->IsAnyType() && !obj->GetType()->IsTableType())
    return nullptr;

  // Resolve key to a const-string index through Phi chains.
  auto* func_op = inst->GetFunction();
  auto lepus_func = func_op ? func_op->GetLepusFunction().get() : nullptr;
  llvh::DenseMap<Value*, int32_t> memo;
  llvh::SmallPtrSet<Value*, 16> visiting;
  int32_t key_idx = -1;
  if (!ResolveConstStringKeyIndex(lepus_func, get_table->GetProp(), &key_idx,
                                  memo, visiting)) {
    return nullptr;
  }

  builder->SetInsertionPointAfter(get_table);
  auto* res = builder->Create<GetTableConstStringKeyInst>(
      get_table->GetLocation(), obj,
      builder->GetLiteralUint32(static_cast<uint32_t>(key_idx)),
      get_table->GetType());
  return res;
}

static bool IsNullishTrampolineBlock(Block* bb, Block* merge_bb,
                                     int8_t expected_nullish_type) {
  if (!bb || !merge_bb) return false;
  auto* term = llvh::dyn_cast_or_null<BranchInst>(bb->GetTerminator());
  if (!term || term->GetBranchDest() != merge_bb) return false;

  llvh::SmallVector<Instruction*, 4> body;
  for (auto* inst : bb->InstRange()) {
    if (llvh::isa<TerminatorInst>(inst)) continue;
    body.push_back(inst);
  }

  if (body.empty()) return true;
  if (body.size() != 1) return false;
  auto* inst = body[0];
  {
    const auto se = inst->GetSideEffect();
    // Avoid instructions that constrain block layout (catch boundaries, etc).
    if (se.GetFirstInBlock() || se.GetIsCatch()) return false;
    // Only allow trivially removable instructions.
    if (se.HasSideEffect()) return false;
  }

  // Allow a locally materialized nullish literal, used only by Phi(s) in merge.
  auto* ln = llvh::dyn_cast<LoadNullOrUndefinedInst>(inst);
  if (!ln) return false;
  if (ln->GetLoadNilType()->GetValue() != expected_nullish_type) return false;
  for (auto* u : inst->GetUsers()) {
    if (!u) continue;
    if (u->GetParent() != merge_bb) return false;
    if (!llvh::isa<PhiInst>(u)) return false;
  }
  return true;
}

struct NullishGuardedGetTableNode {
  EqCondBranchInst* guard = nullptr;
  LoadNullOrUndefinedInst* nullish = nullptr;
  Value* receiver = nullptr;
  Block* nil_bb = nullptr;
  Block* get_bb = nullptr;
  Block* merge_bb = nullptr;
  PhiInst* phi = nullptr;
  Instruction* get_inst = nullptr;  // GetTableInst / GetTableConstStringKeyInst
};

static Instruction* FindSingleGetTableInBlock(Block* bb) {
  if (!bb) return nullptr;
  Instruction* found = nullptr;
  for (auto* inst : bb->InstRange()) {
    if (!inst) continue;
    if (llvh::isa<TerminatorInst>(inst)) continue;
    // Allow only a single GetTable* in the block body.
    if (llvh::isa<GetTableInst>(inst) ||
        llvh::isa<GetTableConstStringKeyInst>(inst)) {
      if (found) return nullptr;
      found = inst;
      continue;
    }
    // Be conservative: reject other instructions.
    return nullptr;
  }
  return found;
}

static PhiInst* FindSinglePhiInMerge(Block* merge_bb) {
  if (!merge_bb) return nullptr;
  PhiInst* phi = nullptr;
  for (auto* inst : merge_bb->InstRange()) {
    if (!inst) continue;
    auto* p = llvh::dyn_cast<PhiInst>(inst);
    if (!p) break;
    if (phi) return nullptr;  // multiple phis not supported
    phi = p;
  }
  return phi;
}

static bool MatchNullishGuardedGetTable(EqCondBranchInst* br,
                                        NullishGuardedGetTableNode* out) {
  if (!br || !out) return false;

  Value* lhs = br->GetLeftHandSide();
  Value* rhs = br->GetRightHandSide();
  auto* lhs_nullish = llvh::dyn_cast_or_null<LoadNullOrUndefinedInst>(lhs);
  auto* rhs_nullish = llvh::dyn_cast_or_null<LoadNullOrUndefinedInst>(rhs);
  LoadNullOrUndefinedInst* nullish = nullptr;
  Value* receiver = nullptr;
  if (lhs_nullish && !rhs_nullish) {
    nullish = lhs_nullish;
    receiver = rhs;
  } else if (rhs_nullish && !lhs_nullish) {
    nullish = rhs_nullish;
    receiver = lhs;
  } else {
    return false;
  }
  if (!receiver) return false;

  Block* nil_bb = br->GetTrueDest();
  Block* get_bb = br->GetFalseDest();
  if (!nil_bb || !get_bb) return false;

  auto* nil_term = llvh::dyn_cast_or_null<BranchInst>(nil_bb->GetTerminator());
  auto* get_term = llvh::dyn_cast_or_null<BranchInst>(get_bb->GetTerminator());
  if (!nil_term || !get_term) return false;

  Block* merge_bb = nil_term->GetBranchDest();
  if (!merge_bb || merge_bb != get_term->GetBranchDest()) return false;

  const int8_t expected_type = nullish->GetLoadNilType()->GetValue();
  if (!IsNullishTrampolineBlock(nil_bb, merge_bb, expected_type)) return false;

  Instruction* get_inst = FindSingleGetTableInBlock(get_bb);
  if (!get_inst) return false;

  Value* get_receiver = nullptr;
  if (auto* g = llvh::dyn_cast<GetTableInst>(get_inst)) {
    get_receiver = g->GetObject();
  } else if (auto* g = llvh::dyn_cast<GetTableConstStringKeyInst>(get_inst)) {
    get_receiver = g->GetObject();
  }
  if (get_receiver != receiver) return false;

  PhiInst* phi = FindSinglePhiInMerge(merge_bb);
  if (!phi || phi->GetNumEntries() != 2) return false;

  // Incoming blocks must be exactly nil_bb and get_bb.
  Value* in_nil = nullptr;
  Value* in_get = nullptr;
  bool has_nil = false;
  bool has_get = false;
  for (unsigned i = 0; i < phi->GetNumEntries(); i++) {
    auto entry = phi->GetEntry(i);
    if (entry.second == nil_bb) {
      has_nil = true;
      in_nil = entry.first;
    } else if (entry.second == get_bb) {
      has_get = true;
      in_get = entry.first;
    } else {
      return false;
    }
  }
  if (!has_nil || !has_get) return false;

  // The get incoming must be the GetTable* result.
  if (in_get != get_inst) return false;

  // The nil incoming must be a nullish value of the same kind.
  if (in_nil != nullish) {
    auto* other_nullish =
        llvh::dyn_cast_or_null<LoadNullOrUndefinedInst>(in_nil);
    if (!other_nullish) return false;
    if (other_nullish->GetLoadNilType()->GetValue() != expected_type)
      return false;
  }

  out->guard = br;
  out->nullish = nullish;
  out->receiver = receiver;
  out->nil_bb = nil_bb;
  out->get_bb = get_bb;
  out->merge_bb = merge_bb;
  out->phi = phi;
  out->get_inst = get_inst;
  return true;
}

// Canonicalize a lowering pattern commonly produced by optional chaining
// expansion (e.g. `(_a = x) == null ? undefined : _a.k`):
//
//   EqCondBranchInst recv == nullish ? merge : get
//   get:    v = GetTable*(recv, key); Branch merge
//   merge:  v2 = GetTable*(recv, key); ... uses v2 ...
//
// Into a CFG shape that can be matched by MatchNullishGuardedGetTable and then
// folded by FoldNullishGuardChainInPlace:
//
//   EqCondBranchInst recv == nullish ? nil : get
//   nil:   Branch merge
//   get:   v = GetTable*(recv, key); Branch merge
//   merge: phi = Phi(nullish, v); ... uses phi ...
static bool CanonicalizeNullishGuardedGetTableMergeRecompute(
    FuncOp* func, OpBuilder* builder) {
  if (!func || !builder) return false;
  bool changed = false;
  Region* region = func->GetSingleRegion();
  if (!region) return false;

  llvh::SmallVector<Instruction*, 16> to_erase;

  for (auto& bb : *func) {
    auto* br = llvh::dyn_cast_or_null<EqCondBranchInst>(bb.GetTerminator());
    if (!br) continue;

    // Identify receiver and nullish literal.
    Value* lhs = br->GetLeftHandSide();
    Value* rhs = br->GetRightHandSide();
    auto* lhs_nullish = llvh::dyn_cast_or_null<LoadNullOrUndefinedInst>(lhs);
    auto* rhs_nullish = llvh::dyn_cast_or_null<LoadNullOrUndefinedInst>(rhs);
    LoadNullOrUndefinedInst* nullish = nullptr;
    Value* receiver = nullptr;
    if (lhs_nullish && !rhs_nullish) {
      nullish = lhs_nullish;
      receiver = rhs;
    } else if (rhs_nullish && !lhs_nullish) {
      nullish = rhs_nullish;
      receiver = lhs;
    } else {
      continue;
    }
    if (!nullish || !receiver) continue;

    // Try both successor orientations.
    struct Candidate {
      Block* merge = nullptr;
      Block* get_bb = nullptr;
      int merge_succ_idx =
          -1;  // which successor of br points directly to merge
      Instruction* get_inst = nullptr;
      Instruction* merge_get_inst = nullptr;
    } cand;

    auto try_match = [&](Block* merge_side, Block* get_side,
                         int merge_succ_idx) -> bool {
      Block* merge = nullptr;
      Instruction* get_inst = nullptr;
      {
        if (!get_side) return false;
        auto* term =
            llvh::dyn_cast_or_null<BranchInst>(get_side->GetTerminator());
        if (!term) return false;
        merge = term->GetBranchDest();
        if (!merge) return false;
        get_inst = FindSingleGetTableInBlock(get_side);
        if (!get_inst) return false;
      }
      if (merge != merge_side) return false;

      // We only canonicalize when merge doesn't already start with phis.
      for (auto* inst : merge->InstRange()) {
        if (!inst) continue;
        if (llvh::isa<PhiInst>(inst)) return false;
        break;
      }

      // Merge must start with a GetTable* that is identical to the one in
      // get_bb.
      Instruction* merge_first = nullptr;
      for (auto* inst : merge->InstRange()) {
        if (!inst) continue;
        if (llvh::isa<PhiInst>(inst)) continue;
        if (llvh::isa<TerminatorInst>(inst)) continue;
        merge_first = inst;
        break;
      }
      if (!merge_first) return false;
      if (!llvh::isa<GetTableInst>(merge_first) &&
          !llvh::isa<GetTableConstStringKeyInst>(merge_first)) {
        return false;
      }
      if (get_inst->GetKind() != merge_first->GetKind()) return false;
      if (auto* ga = llvh::dyn_cast<GetTableInst>(get_inst)) {
        auto* gb = llvh::dyn_cast<GetTableInst>(merge_first);
        if (!gb || ga->GetObject() != gb->GetObject() ||
            ga->GetProp() != gb->GetProp()) {
          return false;
        }
      } else if (auto* ga =
                     llvh::dyn_cast<GetTableConstStringKeyInst>(get_inst)) {
        auto* gb = llvh::dyn_cast<GetTableConstStringKeyInst>(merge_first);
        if (!gb || ga->GetObject() != gb->GetObject() ||
            ga->GetConstIndex() != gb->GetConstIndex()) {
          return false;
        }
      } else {
        return false;
      }

      // Also require that the GetTable* receiver is exactly the compared
      // receiver.
      Value* obj = nullptr;
      if (auto* g = llvh::dyn_cast<GetTableInst>(merge_first)) {
        obj = g->GetObject();
      } else if (auto* g =
                     llvh::dyn_cast<GetTableConstStringKeyInst>(merge_first)) {
        obj = g->GetObject();
      }
      if (obj != receiver) return false;

      cand.merge = merge;
      cand.get_bb = get_side;
      cand.merge_succ_idx = merge_succ_idx;
      cand.get_inst = get_inst;
      cand.merge_get_inst = merge_first;
      return true;
    };

    Block* s0 = br->GetTrueDest();
    Block* s1 = br->GetFalseDest();
    if (!s0 || !s1) continue;
    if (!try_match(/*merge_side*/ s0, /*get_side*/ s1, /*merge_succ_idx*/ 0) &&
        !try_match(/*merge_side*/ s1, /*get_side*/ s0, /*merge_succ_idx*/ 1)) {
      continue;
    }

    // Create nil trampoline block.
    Block* nil_bb =
        builder->CreateBlock(region, BlockType::BT_INST, {}, "nullish_nil_bb");
    if (!nil_bb) continue;
    {
      OpBuilderRestoreInsertPointerRAII _restore(builder);
      builder->SetInsertionPointToEnd(nil_bb);
      builder->Create<BranchInst>(br->GetLocation(), cand.merge);
    }

    // Insert phi at merge start.
    {
      OpBuilderRestoreInsertPointerRAII _restore(builder);
      builder->SetInsertionPointToStart(cand.merge);
      PhiInst::ValueListType values{nullish, cand.get_inst};
      PhiInst::BlockListType blocks{nil_bb, cand.get_bb};
      auto* phi = builder->Create<PhiInst>(br->GetLocation(), values, blocks);
      if (!phi) continue;
      cand.merge_get_inst->ReplaceAllUsesWith(phi);
    }

    // Remove the redundant GetTable* in merge.
    to_erase.push_back(cand.merge_get_inst);

    // Redirect the direct edge to merge into the new nil trampoline.
    br->SetSuccessorImpl(cand.merge_succ_idx, nil_bb);
    changed = true;
  }

  llvh::for_each(to_erase, [&](Instruction* inst) {
    if (inst && inst->GetParent()) inst->EraseFromParent();
  });

  return changed;
}

static bool FoldNullishGuardChainInPlace(OpBuilder* builder, FuncOp* func) {
  if (!func) return false;
  bool changed = false;

  // First, canonicalize optional-chaining-like patterns into a matchable CFG.
  if (CanonicalizeNullishGuardedGetTableMergeRecompute(func, builder)) {
    changed = true;
  }

  llvh::SmallPtrSet<EqCondBranchInst*, 16> visited_guards;

  for (auto& bb : *func) {
    auto* term = llvh::dyn_cast_or_null<EqCondBranchInst>(bb.GetTerminator());
    if (!term) continue;
    if (visited_guards.count(term)) continue;

    // Try to build a forward chain starting from this guard.
    llvh::SmallVector<NullishGuardedGetTableNode, 8> chain;
    EqCondBranchInst* cur = term;
    llvh::SmallPtrSet<Block*, 16> seen_merge;

    while (cur && !visited_guards.count(cur) && chain.size() < 8) {
      NullishGuardedGetTableNode node;
      if (!MatchNullishGuardedGetTable(cur, &node)) break;

      // Avoid cycles.
      if (!seen_merge.insert(node.merge_bb).second) break;

      chain.push_back(node);
      visited_guards.insert(cur);

      // Next guard must be the terminator of the merge block and must test the
      // current phi result against the same nullish value.
      auto* next = llvh::dyn_cast_or_null<EqCondBranchInst>(
          node.merge_bb->GetTerminator());
      if (!next) break;

      Value* n_lhs = next->GetLeftHandSide();
      Value* n_rhs = next->GetRightHandSide();
      const int8_t expected_type = node.nullish->GetLoadNilType()->GetValue();
      auto* n_lhs_nullish =
          llvh::dyn_cast_or_null<LoadNullOrUndefinedInst>(n_lhs);
      auto* n_rhs_nullish =
          llvh::dyn_cast_or_null<LoadNullOrUndefinedInst>(n_rhs);
      const bool rhs_nullish_ok =
          (n_rhs == node.nullish) ||
          (n_rhs_nullish &&
           n_rhs_nullish->GetLoadNilType()->GetValue() == expected_type);
      const bool lhs_nullish_ok =
          (n_lhs == node.nullish) ||
          (n_lhs_nullish &&
           n_lhs_nullish->GetLoadNilType()->GetValue() == expected_type);
      const bool uses_phi = (n_lhs == node.phi && rhs_nullish_ok) ||
                            (n_rhs == node.phi && lhs_nullish_ok);
      if (!uses_phi) break;
      cur = next;
    }

    if (chain.size() < 2) continue;

    // Unify all earlier null branches to jump into the last null trampoline.
    Block* unified_nil = chain.back().nil_bb;
    // Extra safety: ensure the chosen trampoline is still a nullish trampoline
    // to its merge.
    if (!unified_nil || !chain.back().merge_bb) continue;
    const int8_t expected_type =
        chain.back().nullish->GetLoadNilType()->GetValue();
    if (!IsNullishTrampolineBlock(unified_nil, chain.back().merge_bb,
                                  expected_type)) {
      continue;
    }

    for (size_t i = 0; i + 1 < chain.size(); i++) {
      auto* g = chain[i].guard;
      if (!g) continue;
      if (g->GetTrueDest() == unified_nil) continue;
      g->SetSuccessorImpl(0, unified_nil);
      changed = true;
    }
  }

  return changed;
}

static bool InvertCondBranchUnaryNotInPlace(FuncOp* func) {
  // cond = !x
  // CondBranch(cond, T, F)
  //   => CondBranch(x, F, T)
  //
  // Do this in-place to keep terminator constraints intact.
  if (!func) return false;
  bool changed = false;
  for (auto& block : *func) {
    for (auto* inst : block.InstRange()) {
      auto* cond_br = llvh::dyn_cast<CondBranchInst>(inst);
      if (!cond_br) continue;

      auto* cond = cond_br->GetCondition();
      auto* unary = llvh::dyn_cast<UnaryOperatorInst>(cond);
      if (!unary || unary->GetKind() != ValueKind::UnaryNotInstKind) continue;

      // Only safe if the `!` result is exclusively used by this branch.
      if (!cond->HasOneUser()) continue;

      auto* src = unary->GetOperand(0);
      if (!src) continue;

      // Swap successors and use the original operand as condition.
      Block* old_true = cond_br->GetTrueDest();
      Block* old_false = cond_br->GetFalseDest();
      cond_br->SetTrueDest(old_false);
      cond_br->SetFalseDest(old_true);
      cond_br->SetOperand(src, CondBranchInst::ConditionIdx);
      changed = true;
    }
  }
  return changed;
}

static Value* CombineAddEmptyStringToToString(OpBuilder* builder,
                                              Instruction* inst) {
  auto* add = llvh::dyn_cast<BinaryOperatorInst>(inst);
  if (!add || add->GetKind() != ValueKind::BinaryAddInstKind) {
    return nullptr;
  }

  auto* out_ty = add->GetType();
  if (out_ty == nullptr ||
      !(out_ty->IsStringType() || out_ty->IsStringProtoAPIType())) {
    // Only rewrite when add is known to be string concatenation.
    return nullptr;
  }

  auto* func = inst->GetFunction();
  auto lepus_func = func ? func->GetLepusFunction() : nullptr;
  if (!lepus_func) return nullptr;

  auto is_const_empty_string = [&](Value* v) -> bool {
    auto* lc = llvh::dyn_cast_or_null<LoadConstInst>(v);
    if (!lc) return false;
    auto* lit = llvh::dyn_cast<LiteralUint32>(lc->GetConst());
    if (!lit) return false;
    auto* cv = lepus_func->GetConstValue(lit->GetValue());
    if (!cv || !cv->IsString()) return false;
    return cv->StdString().empty();
  };

  Value* lhs = add->GetLeftHandSide();
  Value* rhs = add->GetRightHandSide();
  const bool lhs_empty = is_const_empty_string(lhs);
  const bool rhs_empty = is_const_empty_string(rhs);
  if (!lhs_empty && !rhs_empty) return nullptr;

  Value* other = lhs_empty ? rhs : lhs;
  if (other->GetType()->IsStringType()) {
    return other;
  }

  builder->SetInsertionPointAfter(add);
  auto* ts = builder->Create<ToStringInst>(add->GetLocation(), other);
  // Preserve the original output type (string / string-proto) for downstream
  // type-based lowering.
  ts->SetType(out_ty);
  return ts;
}

Instruction* CombineCompareAndJmp(OpBuilder* builder, Instruction* inst) {
  if (auto cond_branch_inst = llvh::dyn_cast<CondBranchInst>(inst)) {
    if (!cond_branch_inst->IsSmallJmp()) return nullptr;

    auto cond_val = cond_branch_inst->GetCondition();
    if (auto* binary_inst = llvh::dyn_cast<BinaryOperatorInst>(cond_val)) {
      // Safety: only combine when the comparison result is used exclusively by
      // this CondBranchInst. Otherwise, lowering to cmp-jmp would remove the
      // boolean value materialization and break other uses.
      if (!binary_inst->HasOneUser()) return nullptr;
      auto* only_user = llvh::dyn_cast<Instruction>(*binary_inst->UsersBegin());
      if (only_user != cond_branch_inst) return nullptr;

      builder->SetInsertionPointAfter(cond_branch_inst);
      switch (binary_inst->GetKind()) {
        case ValueKind::BinaryStrictlyNotEqualInstKind: {
          auto new_inst = builder->Create<NeqCondBranchInst>(
              cond_branch_inst->GetLocation(), binary_inst->GetLeftHandSide(),
              binary_inst->GetRightHandSide(), cond_branch_inst->GetTrueDest(),
              cond_branch_inst->GetFalseDest());
          return new_inst;
        }
        case ValueKind::BinaryStrictlyEqualInstKind: {
          auto new_inst = builder->Create<EqCondBranchInst>(
              cond_branch_inst->GetLocation(), binary_inst->GetLeftHandSide(),
              binary_inst->GetRightHandSide(), cond_branch_inst->GetTrueDest(),
              cond_branch_inst->GetFalseDest());
          return new_inst;
        }
        default:
          break;
      }
    }
    return nullptr;
  }
  return nullptr;
}

Value* FoldBinaryOperatorInst(OpBuilder* builder, Instruction* inst) {
  auto binary_inst = llvh::dyn_cast<BinaryOperatorInst>(inst);
  if (!binary_inst) return nullptr;

  auto left = binary_inst->GetLeftHandSide();
  auto right = binary_inst->GetRightHandSide();
  auto* left_const = llvh::dyn_cast<LoadConstInst>(left);
  auto* right_const = llvh::dyn_cast<LoadConstInst>(right);

  if (left_const && right_const) {
    auto left_const_index =
        llvh::cast<LiteralUint32>(left_const->GetConst())->GetValue();
    auto right_const_index =
        llvh::cast<LiteralUint32>(right_const->GetConst())->GetValue();
    auto* func = inst->GetFunction();
    auto lepus_func = func->GetLepusFunction();
    if (LEPUS_UNLIKELY(!lepus_func)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: InstCombine constant fold requires non-null "
          "LepusFunction");
    }

    auto* left_val = lepus_func->GetConstValue(left_const_index);
    auto* right_val = lepus_func->GetConstValue(right_const_index);

    auto kind = binary_inst->GetKind();
    builder->SetInsertionPointAfter(binary_inst);

    auto create_bool_const = [&](bool res) {
      auto idx = lepus_func->AddConstBoolean(res);
      return builder->Create<LoadConstInst>(binary_inst->GetLocation(),
                                            builder->GetLiteralUint32(idx),
                                            TypeOp::CreateBoolean(builder));
    };

    auto create_const_value = [&](const lynx::lepus::Value& v,
                                  TypeOp* ty) -> LoadConstInst* {
      auto idx = lepus_func->AddConstValue(v);
      return builder->Create<LoadConstInst>(binary_inst->GetLocation(),
                                            builder->GetLiteralUint32(idx), ty);
    };

    auto create_int64_const = [&](int64_t v) -> LoadConstInst* {
      lynx::lepus::Value res;
      res.SetNumber(v);
      return create_const_value(res, binary_inst->GetType());
    };

    auto create_double_const = [&](double v) -> LoadConstInst* {
      lynx::lepus::Value res;
      res.SetNumber(v);
      return create_const_value(res, binary_inst->GetType());
    };

    switch (kind) {
      case ValueKind::BinaryStrictlyEqualInstKind:
        return create_bool_const(left_val == right_val);
      case ValueKind::BinaryStrictlyNotEqualInstKind:
        return create_bool_const(left_val != right_val);
      case ValueKind::BinaryGreaterThanInstKind:
      case ValueKind::BinaryGreaterThanOrEqualInstKind:
      case ValueKind::BinaryLessThanInstKind:
      case ValueKind::BinaryLessThanOrEqualInstKind: {
        bool res = false;
        if (left_val->IsNumber() && right_val->IsNumber()) {
          auto l_num = left_val->Number();
          auto r_num = right_val->Number();
          if (kind == ValueKind::BinaryGreaterThanInstKind)
            res = l_num > r_num;
          else if (kind == ValueKind::BinaryGreaterThanOrEqualInstKind)
            res = l_num >= r_num;
          else if (kind == ValueKind::BinaryLessThanInstKind)
            res = l_num < r_num;
          else
            res = l_num <= r_num;
        } else if (left_val->IsString() && right_val->IsString()) {
          auto l_str = left_val->StdString();
          auto r_str = right_val->StdString();
          if (kind == ValueKind::BinaryGreaterThanInstKind)
            res = l_str > r_str;
          else if (kind == ValueKind::BinaryGreaterThanOrEqualInstKind)
            res = l_str >= r_str;
          else if (kind == ValueKind::BinaryLessThanInstKind)
            res = l_str < r_str;
          else
            res = l_str <= r_str;
        }
        return create_bool_const(res);
      }
      default:
        break;
    }

    // Numeric/string folding for typed binary operators.
    auto* out_ty = binary_inst->GetType();
    const bool out_is_int64 = out_ty && out_ty->IsInt64Type();
    const bool out_is_float64 = out_ty && out_ty->IsFloat64Type();
    const bool out_is_number = out_ty && out_ty->IsNumberType();
    const bool out_is_string =
        out_ty && (out_ty->IsStringType() || out_ty->IsStringProtoAPIType());

    auto both_int64 = [&]() -> bool {
      return left_val && right_val && left_val->IsInt64() &&
             right_val->IsInt64();
    };
    auto both_number = [&]() -> bool {
      return left_val && right_val && left_val->IsNumber() &&
             right_val->IsNumber();
    };
    auto both_string = [&]() -> bool {
      return left_val && right_val && left_val->IsString() &&
             right_val->IsString();
    };

    switch (kind) {
      case ValueKind::BinaryAddInstKind: {
        if (out_is_string && both_string()) {
          lynx::base::String s(left_val->StdString() + right_val->StdString());
          auto idx = lepus_func->AddConstString(s);
          return builder->Create<LoadConstInst>(binary_inst->GetLocation(),
                                                builder->GetLiteralUint32(idx),
                                                out_ty);
        }

        if (out_is_int64 && both_int64()) {
          int64_t l = left_val->Int64();
          int64_t r = right_val->Int64();
          int64_t res;
          if (__builtin_add_overflow(l, r, &res)) return nullptr;
          return create_int64_const(res);
        }
        if ((out_is_float64 || out_is_number) && both_number()) {
          return create_double_const(left_val->Number() + right_val->Number());
        }
        break;
      }
      case ValueKind::BinarySubInstKind: {
        if (out_is_int64 && both_int64()) {
          int64_t l = left_val->Int64();
          int64_t r = right_val->Int64();
          int64_t res;
          if (__builtin_sub_overflow(l, r, &res)) return nullptr;
          return create_int64_const(res);
        }
        if ((out_is_float64 || out_is_number) && both_number()) {
          return create_double_const(left_val->Number() - right_val->Number());
        }
        break;
      }
      case ValueKind::BinaryMulInstKind: {
        if (out_is_int64 && both_int64()) {
          int64_t l = left_val->Int64();
          int64_t r = right_val->Int64();
          int64_t res;
          if (__builtin_mul_overflow(l, r, &res)) return nullptr;
          return create_int64_const(res);
        }
        if ((out_is_float64 || out_is_number) && both_number()) {
          return create_double_const(left_val->Number() * right_val->Number());
        }
        break;
      }
      case ValueKind::BinaryBitOrInstKind:
      case ValueKind::BinaryBitAndInstKind:
      case ValueKind::BinaryBitXorInstKind: {
        if (!out_is_int64) break;
        if (!both_number()) break;

        int64_t x = 0;
        int64_t y = 0;
        if (both_int64()) {
          x = left_val->Int64();
          y = right_val->Int64();
        } else {
          // Mirror VMContext::RunFrame_Op_Bit{Or,And,Xor}: cast to int64 then
          // apply 32-bit mask.
          double lx = left_val->Number();
          double ry = right_val->Number();
          if (!std::isfinite(lx) || !std::isfinite(ry)) break;
          if (lx < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
              lx > static_cast<double>(std::numeric_limits<int64_t>::max()) ||
              ry < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
              ry > static_cast<double>(std::numeric_limits<int64_t>::max())) {
            break;
          }
          x = (static_cast<int64_t>(lx) & 0xffffffff);
          y = (static_cast<int64_t>(ry) & 0xffffffff);
        }

        int64_t res = 0;
        if (kind == ValueKind::BinaryBitOrInstKind) {
          res = x | y;
        } else if (kind == ValueKind::BinaryBitAndInstKind) {
          res = x & y;
        } else {
          res = x ^ y;
        }
        return create_int64_const(res);
      }
      default:
        break;
    }
  }
  return nullptr;
}

static Value* FoldUnaryOperatorInst(OpBuilder* builder, Instruction* inst) {
  auto* unary_inst = llvh::dyn_cast<UnaryOperatorInst>(inst);
  if (!unary_inst) return nullptr;

  auto* src = unary_inst->GetSingleOperand();

  auto* func = inst->GetFunction();
  auto lepus_func = func ? func->GetLepusFunction() : nullptr;
  if (!lepus_func) return nullptr;

  auto is_always_boolean_value = [](Value* v) -> bool {
    if (!v) return false;
    if (auto* u = llvh::dyn_cast<UnaryOperatorInst>(v)) {
      return u->GetKind() == ValueKind::UnaryNotInstKind;
    }
    if (auto* b = llvh::dyn_cast<BinaryOperatorInst>(v)) {
      switch (b->GetKind()) {
        case ValueKind::BinaryStrictlyEqualInstKind:
        case ValueKind::BinaryStrictlyNotEqualInstKind:
        case ValueKind::BinaryGreaterThanInstKind:
        case ValueKind::BinaryGreaterThanOrEqualInstKind:
        case ValueKind::BinaryLessThanInstKind:
        case ValueKind::BinaryLessThanOrEqualInstKind:
          return true;
        default:
          return false;
      }
    }
    return false;
  };

  // Logical-not patterns and constant folding.
  if (unary_inst->GetKind() == ValueKind::UnaryNotInstKind) {
    // 1) !!!x  ==>  !x
    // 2) !!b   ==>  b  (when b is proven to be boolean)
    // 3) !!x used only by CondBranch ==> x
    if (auto* inner_not = llvh::dyn_cast_or_null<UnaryOperatorInst>(src)) {
      if (inner_not->GetKind() == ValueKind::UnaryNotInstKind) {
        Value* inner_src = inner_not->GetSingleOperand();

        if (auto* inner2_not =
                llvh::dyn_cast_or_null<UnaryOperatorInst>(inner_src)) {
          if (inner2_not->GetKind() == ValueKind::UnaryNotInstKind) {
            // !!!x -> !x
            return inner2_not;
          }
        }

        // !!b -> b, where b is always boolean at runtime.
        if (is_always_boolean_value(inner_src)) {
          return inner_src;
        }

        // !!x in boolean context of a cond branch.
        if (unary_inst->HasOneUser()) {
          auto* user = llvh::dyn_cast<Instruction>(*unary_inst->UsersBegin());
          if (llvh::isa<CondBranchInst>(user)) {
            return inner_src;
          }
        }
      }
    }

    // !<const> -> <bool const>
    if (auto* src_const = llvh::dyn_cast_or_null<LoadConstInst>(src)) {
      auto* lit = llvh::dyn_cast<LiteralUint32>(src_const->GetConst());
      if (!lit) return nullptr;
      auto* val = lepus_func->GetConstValue(lit->GetValue());
      if (!val) return nullptr;

      builder->SetInsertionPointAfter(unary_inst);
      bool res = RestrictedValue(*val).IsFalse();
      uint32_t idx = lepus_func->AddConstBoolean(res);
      return builder->Create<LoadConstInst>(unary_inst->GetLocation(),
                                            builder->GetLiteralUint32(idx),
                                            TypeOp::CreateBoolean(builder));
    }

    return nullptr;
  }

  // Only fold cases that are provably side-effect free and type-stable.
  // For now we keep it conservative: only fold `-<int64/number const>`.
  if (unary_inst->GetKind() != ValueKind::UnaryNegInstKind) return nullptr;

  auto* src_const = llvh::dyn_cast<LoadConstInst>(src);
  if (!src_const) return nullptr;

  auto* lit = llvh::dyn_cast<LiteralUint32>(src_const->GetConst());
  if (!lit) return nullptr;
  uint32_t const_id = lit->GetValue();

  auto* val = lepus_func->GetConstValue(const_id);
  if (!val) return nullptr;

  // Prefer exact integer folding when possible.
  builder->SetInsertionPointAfter(unary_inst);
  if (val->IsInt64()) {
    int64_t v = val->Int64();
    if (v == std::numeric_limits<int64_t>::min()) return nullptr;
    uint32_t neg_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(
        static_cast<double>(-v)));  // stored as int64 by AddConstNumber
    return builder->Create<LoadConstInst>(unary_inst->GetLocation(),
                                          builder->GetLiteralUint32(neg_idx),
                                          TypeOp::CreateInt64(builder));
  }

  // Fall back to numeric folding for non-int64 numbers.
  // Skip 0 to avoid introducing potential -0 semantics differences.
  if (!val->IsNumber()) return nullptr;
  double n = val->Number();
  if (n == 0.0) return nullptr;
  uint32_t neg_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(-n));
  return builder->Create<LoadConstInst>(unary_inst->GetLocation(),
                                        builder->GetLiteralUint32(neg_idx),
                                        TypeOp::CreateFloat64(builder));
}

Value* ConstantFold(OpBuilder* builder, Instruction* inst) {
  if (auto new_inst = FoldUnaryOperatorInst(builder, inst)) {
    return new_inst;
  }
  if (auto new_inst = FoldBinaryOperatorInst(builder, inst)) {
    return new_inst;
  }
  return nullptr;
}

Instruction* CombineCondBranch(OpBuilder* builder, Instruction* inst) {
  Block* cur_bb = inst->GetBlock();
  if (auto cond_branch_inst = llvh::dyn_cast<CondBranchInst>(inst)) {
    auto cond_val = cond_branch_inst->GetCondition();

    // Pattern: CondBranch(!x, T, F) -> CondBranch(x, F, T)
    //
    // Motivation: In real-world bytecode, `!x` is very often materialized only
    // for branching. Swapping the successors avoids emitting extra `Not` and
    // usually also reduces register moves in later stages.
    //
    // Safety: only apply when the `!x` value is used exclusively by this
    // CondBranchInst.
    if (auto* unary = llvh::dyn_cast<UnaryOperatorInst>(cond_val)) {
      if (unary->GetKind() == ValueKind::UnaryNotInstKind &&
          unary->HasOneUser()) {
        auto* user = llvh::dyn_cast<Instruction>(*unary->UsersBegin());
        if (user == cond_branch_inst) {
          Value* inner = unary->GetSingleOperand();
          builder->SetInsertionPointAfter(cond_branch_inst);
          auto* new_inst = builder->Create<CondBranchInst>(
              cond_branch_inst->GetLocation(), inner,
              cond_branch_inst->GetFalseDest(),
              cond_branch_inst->GetTrueDest());
          return new_inst;
        }
      }
    }

    if (auto load_const = llvh::dyn_cast<LoadConstInst>(cond_val)) {
      auto literal = load_const->GetConst();
      if (LEPUS_UNLIKELY(!llvh::isa<LiteralUint32>(literal))) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: CombineCondBranch expects condition constant "
            "index to be LiteralUint32");
      }
      auto lit_uint32 = llvh::dyn_cast<LiteralUint32>(literal);
      uint32_t const_id = lit_uint32->GetValue();

      auto* func = inst->GetFunction();
      auto lepus_func = func->GetLepusFunction();
      if (LEPUS_UNLIKELY(!lepus_func)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: CombineCondBranch requires non-null "
            "LepusFunction");
      }

      auto* val = lepus_func->GetConstValue(const_id);
      if (LEPUS_UNLIKELY(!val)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: CombineCondBranch failed to read const value");
      }

      builder->SetInsertionPointAfter(cond_branch_inst);
      Block* dest = RestrictedValue(*val).IsFalse()
                        ? cond_branch_inst->GetFalseDest()
                        : cond_branch_inst->GetTrueDest();
      auto new_inst =
          builder->Create<BranchInst>(cond_branch_inst->GetLocation(), dest);

      Block* unreached_bb = dest == cond_branch_inst->GetTrueDest()
                                ? cond_branch_inst->GetFalseDest()
                                : cond_branch_inst->GetTrueDest();

      // for phi_inst in unreached_bb, replace its use with phi_inst's
      // incoming value from dest
      for (auto& op : *unreached_bb) {
        if (auto* phi_inst = llvh::dyn_cast<PhiInst>(&op)) {
          for (int i = phi_inst->GetNumEntries() - 1; i >= 0; i--) {
            auto bb = phi_inst->GetEntry(i).second;
            if (bb == cur_bb) {
              phi_inst->RemoveEntry(i);
            }
          }
        } else {
          break;
        }
      }
      return new_inst;
    }
  }
  return nullptr;
}

bool InstCombinePass::RunOnFunction(FuncOp* func) {
  changed_ = true;
  uint32_t counter = 0;
  while (changed_) {
    counter++;
    if (counter >= constants::kMaxCombineTimes) {
      return true;
    }

    changed_ = false;
    llvh::SmallVector<Instruction*, 64> to_removed;
    // First, perform in-place CFG-friendly combines.
    if (InvertCondBranchUnaryNotInPlace(func)) {
      changed_ = true;
    }
    auto builder = ir_ctx_->GetOpBuilder();
    if (FoldNullishGuardChainInPlace(builder, func)) {
      changed_ = true;
    }
    for (auto& block : *func) {
      for (auto* inst : block.InstRange()) {
        Value* combined = nullptr;

        do {
          // 1. combine compare + jmp
          combined = CombineCompareAndJmp(builder, inst);
          if (combined) break;

          // 2. combine cond branch with constant
          combined = CombineCondBranch(builder, inst);
          if (combined) break;

          // 3. constfold
          combined = ConstantFold(builder, inst);
          if (combined) break;

          // 3.1 x + "" or "" + x  ==>  ToString(x)
          combined = CombineAddEmptyStringToToString(builder, inst);
          if (combined) break;

          combined = CombineSetTableConstStringKey(builder, inst);
          if (combined) break;

          combined = CombineGetTableConstStringKey(builder, inst);
          if (combined) break;
        } while (0);

        if (combined) {
          changed_ = true;
          ir_ctx_->UpdateSpecialAttribute(inst, combined);
          inst->ReplaceAllUsesWith(combined);
          to_removed.push_back(inst);
          continue;
        }
      }
    }
    llvh::for_each(to_removed,
                   [&](Instruction* inst) { inst->EraseFromParent(); });
  }
  return true;
}

Pass* CreateInstCombinePass(IRContext* ir_ctx) {
  return new InstCombinePass(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
