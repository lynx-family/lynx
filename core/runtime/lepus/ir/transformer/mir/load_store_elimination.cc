// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"

#include <string>

#include "base/include/value/base_string.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallPtrSet.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Casting.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

bool LoadStoreElimination::TableKey::ExtractInt32(Value* v, int32_t* out) {
  if (!v || !out) return false;
  if (auto* i8 = llvh::dyn_cast<LiteralInt8>(v)) {
    *out = static_cast<int32_t>(i8->GetValue());
    return true;
  }
  if (auto* i32 = llvh::dyn_cast<LiteralInt32>(v)) {
    *out = i32->GetValue();
    return true;
  }
  if (auto* u8 = llvh::dyn_cast<LiteralUint8>(v)) {
    *out = static_cast<int32_t>(u8->GetValue());
    return true;
  }
  if (auto* u32 = llvh::dyn_cast<LiteralUint32>(v)) {
    *out = static_cast<int32_t>(u32->GetValue());
    return true;
  }
  return false;
}

bool LoadStoreElimination::TableKey::TryGetConstIndexFromLoadConst(
    Value* v, uint32_t* out) {
  if (!v || !out) return false;
  auto* load_const = llvh::dyn_cast<LoadConstInst>(v);
  if (!load_const) return false;
  // Only treat it as a const-table string index if its type is string.
  // This avoids confusing numeric keys with const-table indices.
  if (!load_const->GetType() || !load_const->GetType()->IsStringType()) {
    return false;
  }
  if (auto* u32 = llvh::dyn_cast<LiteralUint32>(load_const->GetConst())) {
    *out = u32->GetValue();
    return true;
  }
  return false;
}

bool LoadStoreElimination::TableKey::TryGetConstIndex(
    Value* v, bool treat_literal_u32_as_const_index, uint32_t* out) {
  if (!v || !out) return false;

  // 1) String LoadConstInst(<LiteralUint32 idx>) => const-table index.
  if (TryGetConstIndexFromLoadConst(v, out)) return true;

  // 2) For SetTableConstStringKeyInst, the key is a LiteralUint32 index.
  if (treat_literal_u32_as_const_index) {
    if (auto* u32 = llvh::dyn_cast<LiteralUint32>(v)) {
      *out = u32->GetValue();
      return true;
    }
    // Be defensive: sometimes it can also be encoded as LoadConstInst.
    if (auto* load_const = llvh::dyn_cast<LoadConstInst>(v)) {
      if (auto* u32 = llvh::dyn_cast<LiteralUint32>(load_const->GetConst())) {
        *out = u32->GetValue();
        return true;
      }
    }
  }
  return false;
}

LoadStoreElimination::TableKey::NormalizedKey
LoadStoreElimination::TableKey::Normalize(
    Value* v, bool treat_literal_u32_as_const_index) {
  LoadStoreElimination::TableKey::NormalizedKey nk{
      LoadStoreElimination::TableKey::NormalizedKey::Kind::Ptr,
      static_cast<uint64_t>(reinterpret_cast<uintptr_t>(v))};
  if (!v) return nk;

  // Const-table key equivalence:
  // - LoadConstInst(string, idx)
  // - LiteralUint32(idx) when it is from SetTableConstStringKeyInst
  uint32_t const_idx = 0;
  if (TryGetConstIndex(v, treat_literal_u32_as_const_index, &const_idx)) {
    nk.kind = LoadStoreElimination::TableKey::NormalizedKey::Kind::ConstIndex;
    nk.payload = static_cast<uint64_t>(const_idx);
    return nk;
  }

  // GetGlobalInst / GetBuiltinInst equivalence: compare by constant index.
  if (auto* gg = llvh::dyn_cast<GetGlobalInst>(v)) {
    int32_t idx = 0;
    if (ExtractInt32(gg->GetGlobalIndex(), &idx)) {
      nk.kind = NormalizedKey::Kind::GlobalIndex;
      nk.payload = static_cast<uint64_t>(static_cast<uint32_t>(idx));
      return nk;
    }
  }
  if (auto* gb = llvh::dyn_cast<GetBuiltinInst>(v)) {
    int32_t idx = 0;
    if (ExtractInt32(gb->GetBuiltinIndex(), &idx)) {
      nk.kind = NormalizedKey::Kind::BuiltinIndex;
      nk.payload = static_cast<uint64_t>(static_cast<uint32_t>(idx));
      return nk;
    }
  }

  return nk;
}

bool LoadStoreElimination::TableKey::operator==(
    const LoadStoreElimination::TableKey& other) const {
  // Object: allow stable matching for canonical producers like GetGlobal /
  // GetBuiltin even if they are different Value* instances.
  NormalizedKey o1 =
      Normalize(object, /*treat_literal_u32_as_const_index*/ false);
  NormalizedKey o2 = Normalize(other.object,
                               /*treat_literal_u32_as_const_index*/ false);
  if (o1.kind != o2.kind || o1.payload != o2.payload) return false;

  // Key: match const-string-key store (LiteralUint32 idx) with a string
  // LoadConstInst carrying the same const-table index.
  NormalizedKey k1 = Normalize(key, key_is_const_string_index);
  NormalizedKey k2 = Normalize(other.key, other.key_is_const_string_index);
  return k1.kind == k2.kind && k1.payload == k2.payload;
}

// Collect blocks in reverse post order (RPO). RPO gives deterministic order and
// tends to converge faster for forward dataflow.
static llvh::SmallVector<Block*, 32> CollectBlocksInRPO(FuncOp* func) {
  PostOrderAnalysis PO(func);
  llvh::SmallVector<Block*, 32> order;
  order.append(PO.rbegin(), PO.rend());
  return order;
}

// Convert a constant lepus::Value number into its property-key string form.
//
// NOTE: LSE only uses this for *constant* keys to decide whether two constant
// keys may alias after JS ToPropertyKey/ToString conversion.
//
// If conversion is not supported for a specific value kind, return empty
// string and let callers fall back to conservative aliasing.
static std::string NumberToPropertyKeyString(const lepus::Value& v) {
  if (v.IsInt32()) return std::to_string(v.Int32());
  if (v.IsUInt32()) return std::to_string(v.UInt32());
  if (v.IsInt64()) return std::to_string(v.Int64());
  if (v.IsUInt64()) return std::to_string(v.UInt64());
  if (v.IsDouble()) {
    char buffer[128];
    const char* num_str = base::StringConvertHelper::NumberToString(
        v.Number(), buffer, sizeof(buffer));
    if (num_str) return std::string(num_str);
    return base::StringConvertHelper::DoubleToString(v.Number());
  }
  return std::string();
}

static bool ConstNumberStringKeysMayAlias(const lepus::Value& num,
                                          const lepus::Value& str) {
  if (!str.IsString()) return true;
  const std::string num_key = NumberToPropertyKeyString(num);
  if (num_key.empty()) {
    // Unknown conversion behavior: be conservative.
    return true;
  }
  return num_key == str.StdString();
}

void LoadStoreElimination::Reset(FuncOp* f) {
  func_ = f;
  to_remove_.clear();
  replaced_by_.clear();
}

bool LoadStoreElimination::IsReadonlyCall(CallInst* call) {
  if (!call) return false;
  return call->IsReadonlyCall();
}

Value* LoadStoreElimination::ResolveReplacement(Value* v) {
  if (!v) return v;
  // Follow the replacement chain.
  Value* cur = v;
  llvh::SmallPtrSet<Value*, 8> visited;
  while (cur) {
    auto it = replaced_by_.find(cur);
    if (it == replaced_by_.end()) break;
    Value* next = it->second;
    if (!next) break;
    if (!visited.insert(cur).second) break;
    cur = next;
  }
  // Path compression.
  if (cur != v) {
    replaced_by_[v] = cur;
  }
  return cur;
}

bool LoadStoreElimination::RunOnFunction(FuncOp* f) {
  if (f->GetBlockSize() == 0) return false;

  Reset(f);

  // ============================================================
  // Phase 0: block order
  // ============================================================
  // We use RPO for deterministic iteration and faster convergence.
  auto rpo_blocks = CollectBlocksInRPO(f);

  // ============================================================
  // Phase 1: forward dataflow to compute per-block IN/OUT caches
  // ============================================================
  // We want to eliminate redundant loads across basic blocks, including join
  // points. A dominator-tree walk is path-sensitive but cannot safely propagate
  // values through join blocks without either clearing everything or inserting
  // PHIs. Instead, we run a classic forward dataflow:
  //   IN[BB]  = meet( OUT[pred] )
  //   OUT[BB] = transfer(BB, IN[BB])
  //
  // Meet is a conservative intersection. We keep a cache entry only if ALL
  // predecessors provide the same key AND the same value definition.
  //
  // IMPORTANT: meet must be dominance-safe. Using canonical equality (e.g.
  // folding aliases) is UNSOUND because it could keep a value defined only on
  // one predecessor path.
  auto values_equal = [](Value* a, Value* b) { return a == b; };

  auto available_equal = [&](const AvailableValues& a,
                             const AvailableValues& b) {
    if (a.tables.size() != b.tables.size()) return false;
    if (a.globals.size() != b.globals.size()) return false;
    if (a.builtins.size() != b.builtins.size()) return false;
    if (a.toplevel_closures.size() != b.toplevel_closures.size()) return false;
    if (a.toplevel_vars.size() != b.toplevel_vars.size()) return false;
    if (a.constants.size() != b.constants.size()) return false;
    if (a.upvalues.size() != b.upvalues.size()) return false;
    if (a.context_slots.size() != b.context_slots.size()) return false;

    for (const auto& kv : a.tables) {
      auto it = b.tables.find(kv.first);
      if (it == b.tables.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    for (const auto& kv : a.globals) {
      auto it = b.globals.find(kv.first);
      if (it == b.globals.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    for (const auto& kv : a.builtins) {
      auto it = b.builtins.find(kv.first);
      if (it == b.builtins.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    for (const auto& kv : a.toplevel_closures) {
      auto it = b.toplevel_closures.find(kv.first);
      if (it == b.toplevel_closures.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    for (const auto& kv : a.toplevel_vars) {
      auto it = b.toplevel_vars.find(kv.first);
      if (it == b.toplevel_vars.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    for (const auto& kv : a.constants) {
      auto it = b.constants.find(kv.first);
      if (it == b.constants.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    for (const auto& kv : a.upvalues) {
      auto it = b.upvalues.find(kv.first);
      if (it == b.upvalues.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    for (const auto& kv : a.context_slots) {
      auto it = b.context_slots.find(kv.first);
      if (it == b.context_slots.end()) return false;
      if (!values_equal(kv.second, it->second)) return false;
    }
    return true;
  };

  auto intersect_into = [&](AvailableValues& acc,
                            const AvailableValues& other) {
    // tables
    for (auto it = acc.tables.begin(); it != acc.tables.end();) {
      auto jt = other.tables.find(it->first);
      if (jt == other.tables.end() || !values_equal(it->second, jt->second)) {
        it = acc.tables.erase(it);
      } else {
        ++it;
      }
    }
    // globals
    for (auto it = acc.globals.begin(); it != acc.globals.end();) {
      auto jt = other.globals.find(it->first);
      if (jt == other.globals.end() || !values_equal(it->second, jt->second)) {
        it = acc.globals.erase(it);
      } else {
        ++it;
      }
    }
    // builtins
    for (auto it = acc.builtins.begin(); it != acc.builtins.end();) {
      auto jt = other.builtins.find(it->first);
      if (jt == other.builtins.end() || !values_equal(it->second, jt->second)) {
        it = acc.builtins.erase(it);
      } else {
        ++it;
      }
    }
    // toplevel_closures
    for (auto it = acc.toplevel_closures.begin();
         it != acc.toplevel_closures.end();) {
      auto jt = other.toplevel_closures.find(it->first);
      if (jt == other.toplevel_closures.end() ||
          !values_equal(it->second, jt->second)) {
        it = acc.toplevel_closures.erase(it);
      } else {
        ++it;
      }
    }
    // toplevel_vars
    for (auto it = acc.toplevel_vars.begin(); it != acc.toplevel_vars.end();) {
      auto jt = other.toplevel_vars.find(it->first);
      if (jt == other.toplevel_vars.end() ||
          !values_equal(it->second, jt->second)) {
        it = acc.toplevel_vars.erase(it);
      } else {
        ++it;
      }
    }
    // constants
    for (auto it = acc.constants.begin(); it != acc.constants.end();) {
      auto jt = other.constants.find(it->first);
      if (jt == other.constants.end() ||
          !values_equal(it->second, jt->second)) {
        it = acc.constants.erase(it);
      } else {
        ++it;
      }
    }
    // upvalues
    for (auto it = acc.upvalues.begin(); it != acc.upvalues.end();) {
      auto jt = other.upvalues.find(it->first);
      if (jt == other.upvalues.end() || !values_equal(it->second, jt->second)) {
        it = acc.upvalues.erase(it);
      } else {
        ++it;
      }
    }
    // context_slots
    for (auto it = acc.context_slots.begin(); it != acc.context_slots.end();) {
      auto jt = other.context_slots.find(it->first);
      if (jt == other.context_slots.end() ||
          !values_equal(it->second, jt->second)) {
        it = acc.context_slots.erase(it);
      } else {
        ++it;
      }
    }
  };

  using BlockToAvail = std::unordered_map<Block*, AvailableValues>;
  BlockToAvail in_avail;
  BlockToAvail out_avail;

  // Initialize maps for all blocks we will visit.
  for (auto* bb : rpo_blocks) {
    in_avail.emplace(bb, AvailableValues{});
    out_avail.emplace(bb, AvailableValues{});
  }

  auto compute_meet_in = [&](Block* bb) {
    AvailableValues in;
    bool first_pred = true;
    for (auto* pred : Predecessors(bb)) {
      auto it = out_avail.find(pred);
      if (it == out_avail.end()) continue;
      if (first_pred) {
        in = it->second;
        first_pred = false;
      } else {
        intersect_into(in, it->second);
      }
    }
    // For entry blocks (no predecessors), `in` stays empty.
    return in;
  };

  bool flow_changed = true;
  int iter_guard = 0;
  while (flow_changed && ++iter_guard < constants::kLoadStoreEliminationIter) {
    flow_changed = false;
    for (auto* bb : rpo_blocks) {
      AvailableValues new_in = compute_meet_in(bb);
      if (!available_equal(in_avail[bb], new_in)) {
        in_avail[bb] = new_in;
        flow_changed = true;
      }

      AvailableValues new_out = new_in;
      // Transfer function: update caches as we scan the block.
      // NOTE: This phase must NOT perform IR rewrites or schedule deletions.
      ProcessBlock(bb, new_out, /*enable_elimination*/ false);
      if (!available_equal(out_avail[bb], new_out)) {
        out_avail[bb] = new_out;
        flow_changed = true;
      }
    }
  }

  // ============================================================
  // Phase 2: elimination pass using IN[BB] as seed
  // ============================================================
  // We now re-scan blocks and perform actual rewrites. We seed each block's
  // cache with the precomputed IN set so that redundant loads at the beginning
  // of a block (including join blocks) can be eliminated.
  bool changed = false;
  for (auto* bb : rpo_blocks) {
    AvailableValues avail = in_avail[bb];
    changed |= ProcessBlock(bb, avail, /*enable_elimination*/ true);
  }

  // ============================================================
  // Phase 3: erase scheduled instructions (order-independent)
  // ============================================================
  // `to_remove_` is populated during Phase 2. The set may contain dependency
  // chains, e.g. a load that is used only by another load that is also being
  // removed. We therefore erase with a small fixpoint loop:
  //   - erase instructions that currently have no users
  //   - repeat until no progress
  // Remaining instructions would imply uses outside the removal set, which
  // would create dangling references.
  llvh::SmallPtrSet<Instruction*, 32> seen;
  llvh::SmallVector<Instruction*, 32> pending;
  pending.reserve(to_remove_.size());
  for (auto* inst : to_remove_) {
    if (!inst) continue;
    if (!seen.insert(inst).second) continue;
    pending.push_back(inst);
  }

  bool made_progress = true;
  unsigned passes = 0;
  const unsigned max_passes = pending.size() + 1;
  while (made_progress && !pending.empty() && passes++ < max_passes) {
    made_progress = false;
    for (auto it = pending.begin(); it != pending.end();) {
      Instruction* inst = *it;
      if (!inst) {
        it = pending.erase(it);
        continue;
      }
      if (inst->HasUsers()) {
        ++it;
        continue;
      }
      inst->EraseFromParent();
      it = pending.erase(it);
      made_progress = true;
    }
  }

  // At this point, any remaining instructions still have users outside the set,
  // which would create dangling references if erased.
  if (LEPUS_UNLIKELY(!pending.empty())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: LSE scheduled deletions still have users after "
        "fixpoint");
  }
  return changed;
}

bool LoadStoreElimination::ProcessBlock(Block* bb, AvailableValues& available,
                                        bool enable_elimination) {
  bool changed = false;

  for (auto* inst : bb->InstRange()) {
    // is_special_inst means the instruction should not be deleted in this pass
    // should only appear in SetToplevelVarInst or SetToplevelClosureVarInst
    bool is_special_inst =
        inst->GetToplevelVarReg() != constants::kInvalidSignedValue ||
        inst->GetClosureVarReg() != constants::kInvalidSignedValue;

    auto ProcessLoad = [&](auto* i, auto& cache, auto key) {
      // Special instructions (toplevel vars / closure vars anchors) must not be
      // deleted by LSE. In debug builds we used to assert, but release builds
      // would silently optimize them away if the attribute ever leaked.
      if (is_special_inst) {
        cache[key] = i;
        return;
      }
      auto it = cache.find(key);
      if (it != cache.end()) {
        if (enable_elimination) {
          Value* rep = ResolveReplacement(it->second);
          // Keep cache canonical w.r.t. replacements.
          if (rep != it->second) {
            it->second = rep;
          }
          i->ReplaceAllUsesWith(rep);
          replaced_by_[i] = rep;
          to_remove_.push_back(i);
          changed = true;
        }
      } else {
        cache[key] = ResolveReplacement(i);
      }
    };

    if (auto* get_table_inst = llvh::dyn_cast<GetTableInst>(inst)) {
      ProcessLoad(
          get_table_inst, available.tables,
          TableKey{get_table_inst->GetObject(), get_table_inst->GetProp(),
                   /*key_is_const_string_index*/ false});
    } else if (auto* get_table_const_key =
                   llvh::dyn_cast<GetTableConstStringKeyInst>(inst)) {
      ProcessLoad(get_table_const_key, available.tables,
                  TableKey{get_table_const_key->GetObject(),
                           get_table_const_key->GetConstIndex(),
                           /*key_is_const_string_index*/ true});
    } else if (auto* set_table_inst = llvh::dyn_cast<SetTableInst>(inst)) {
      if (LEPUS_UNLIKELY(is_special_inst)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: LSE encountered special instruction attributes on "
            "SetTableInst");
      }
      TableKey key{set_table_inst->GetObject(), set_table_inst->GetProp(),
                   /*key_is_const_string_index*/ false};
      Value* val = set_table_inst->GetStoreVal();
      auto it = available.tables.find(key);
      if (it != available.tables.end() && it->second == val) {
        if (enable_elimination) {
          to_remove_.push_back(set_table_inst);
          changed = true;
        }
      } else {
        InvalidateTables(available, set_table_inst->GetObject(),
                         set_table_inst->GetProp(),
                         /*written_key_is_const_index*/ false);
        available.tables[key] = val;
      }
    } else if (auto* set_table_const_key =
                   llvh::dyn_cast<SetTableConstStringKeyInst>(inst)) {
      if (LEPUS_UNLIKELY(is_special_inst)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: LSE encountered special instruction attributes on "
            "SetTableConstStringKeyInst");
      }
      // const_index is a const-table index (uint32) for a string key.
      // We treat it as a normal table store with a canonicalized (obj, key).
      TableKey key{set_table_const_key->GetObject(),
                   set_table_const_key->GetConstIndex(),
                   /*key_is_const_string_index*/ true};
      Value* val = set_table_const_key->GetStoreVal();
      auto it = available.tables.find(key);
      if (it != available.tables.end() && it->second == val) {
        if (enable_elimination) {
          to_remove_.push_back(set_table_const_key);
          changed = true;
        }
      } else {
        InvalidateTables(available, set_table_const_key->GetObject(),
                         set_table_const_key->GetConstIndex(),
                         /*written_key_is_const_index*/ true);
        available.tables[key] = val;
      }
    } else if (auto* get_global = llvh::dyn_cast<GetGlobalInst>(inst)) {
      ProcessLoad(get_global, available.globals, get_global->GetGlobalIndex());
    } else if (auto* get_builtin = llvh::dyn_cast<GetBuiltinInst>(inst)) {
      ProcessLoad(get_builtin, available.builtins,
                  get_builtin->GetBuiltinIndex());
    } else if (auto* get_top_closure =
                   llvh::dyn_cast<GetToplevelClosureVarInst>(inst)) {
      ProcessLoad(get_top_closure, available.toplevel_closures,
                  get_top_closure->GetClosureReg());
    } else if (auto* set_top_closure =
                   llvh::dyn_cast<SetToplevelClosureVarInst>(inst)) {
      Value* reg = set_top_closure->GetClosureReg();
      Value* src = set_top_closure->GetSrc();
      auto it = available.toplevel_closures.find(reg);
      if (!is_special_inst && it != available.toplevel_closures.end() &&
          it->second == src) {
        if (enable_elimination) {
          to_remove_.push_back(set_top_closure);
          changed = true;
        }
      } else {
        available.toplevel_closures[reg] = src;
      }
    } else if (auto* get_top_var = llvh::dyn_cast<GetToplevelVarInst>(inst)) {
      ProcessLoad(get_top_var, available.toplevel_vars,
                  get_top_var->GetToplevelReg());
    } else if (auto* set_top_var = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
      Value* reg = set_top_var->GetToplevelReg();
      Value* src = set_top_var->GetSrc();
      auto it = available.toplevel_vars.find(reg);
      if (!is_special_inst && it != available.toplevel_vars.end() &&
          it->second == src) {
        if (enable_elimination) {
          to_remove_.push_back(set_top_var);
          changed = true;
        }
      } else {
        available.toplevel_vars[reg] = src;
      }
    } else if (auto* get_upvalue_inst = llvh::dyn_cast<GetUpvalueInst>(inst)) {
      ProcessLoad(get_upvalue_inst, available.upvalues,
                  get_upvalue_inst->GetIndex());
    } else if (auto* set_upvalue_inst = llvh::dyn_cast<SetUpvalueInst>(inst)) {
      if (LEPUS_UNLIKELY(is_special_inst)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: LSE encountered special instruction attributes on "
            "SetUpvalueInst");
      }
      Value* index = set_upvalue_inst->GetIndex();
      Value* src = set_upvalue_inst->GetSrc();
      auto it = available.upvalues.find(index);
      if (it != available.upvalues.end() && it->second == src) {
        if (enable_elimination) {
          to_remove_.push_back(set_upvalue_inst);
          changed = true;
        }
      } else {
        available.upvalues[index] = src;
      }
    } else if (auto* get_ctx = llvh::dyn_cast<GetContextSlotInst>(inst)) {
      ProcessLoad(get_ctx, available.context_slots,
                  ContextKey{get_ctx->GetDepth(), get_ctx->GetIndex()});
    } else if (auto* set_ctx = llvh::dyn_cast<SetContextSlotInst>(inst)) {
      if (LEPUS_UNLIKELY(is_special_inst)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: LSE encountered special instruction attributes on "
            "SetContextSlotInst");
      }
      ContextKey key{set_ctx->GetDepth(), set_ctx->GetIndex()};
      Value* src = set_ctx->GetToStore();
      auto it = available.context_slots.find(key);
      if (it != available.context_slots.end() && it->second == src) {
        if (enable_elimination) {
          to_remove_.push_back(set_ctx);
          changed = true;
        }
      } else {
        available.context_slots[key] = src;
      }
    } else if (auto* get_ctx_mov =
                   llvh::dyn_cast<GetContextSlotMovInst>(inst)) {
      ProcessLoad(
          get_ctx_mov, available.context_slots,
          ContextKey{get_ctx_mov->GetContext(), get_ctx_mov->GetIndex()});
    } else if (auto* set_ctx_mov =
                   llvh::dyn_cast<SetContextSlotMovInst>(inst)) {
      if (LEPUS_UNLIKELY(is_special_inst)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: LSE encountered special instruction attributes on "
            "SetContextSlotMovInst");
      }
      ContextKey key{set_ctx_mov->GetContext(), set_ctx_mov->GetIndex()};
      Value* src = set_ctx_mov->GetToStore();
      auto it = available.context_slots.find(key);
      if (it != available.context_slots.end() && it->second == src) {
        if (enable_elimination) {
          to_remove_.push_back(set_ctx_mov);
          changed = true;
        }
      } else {
        available.context_slots[key] = src;
      }
    } else if (auto* load_const = llvh::dyn_cast<LoadConstInst>(inst)) {
      ProcessLoad(load_const, available.constants, load_const->GetConst());
    } else if (inst->GetSideEffect().MayReadOrWorse() &&
               !inst->GetSideEffect().IsPure()) {
      if (!(llvh::isa<NewTableInst>(inst) || llvh::isa<NewArrayInst>(inst) ||
            llvh::isa<CreateBlockContextInst>(inst) ||
            llvh::isa<CreateFunctionContextInst>(inst) ||
            llvh::isa<CreateClosureInst>(inst)) &&
          (inst->GetSideEffect().HasSideEffect() ||
           inst->GetSideEffect().GetExecuteJS())) {
        // A conservative rule: calls invalidate all mutable caches.
        // Exception: calling certain known read-only builtins (loaded via
        // GetBuiltinInst) does not mutate heap/frame, so it does not need to
        // clear LSE's mutable caches.
        if (auto* call = llvh::dyn_cast<CallInst>(inst)) {
          if (IsReadonlyCall(call)) {
            continue;
          }
        }
        available.InvalidateMutableCaches();
      }
    }
  }

  return changed;
}

void LoadStoreElimination::InvalidateTables(AvailableValues& available,
                                            Value* written_object,
                                            Value* written_key,
                                            bool written_key_is_const_index) {
  auto IsAlloc = [](Value* v) {
    if (!v) return false;
    ValueKind k = v->GetKind();
    return k == ValueKind::NewTableInstKind ||
           k == ValueKind::NewArrayInstKind ||
           k == ValueKind::CreateBlockContextInstKind ||
           k == ValueKind::CreateFunctionContextInstKind ||
           k == ValueKind::CreateClosureInstKind;
  };

  if (!written_key) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: LoadStoreElimination expected table store to have a "
        "key");
  }

  // Fresh allocation objects cannot alias any previously cached object. At the
  // current program point, `available.tables` only contains entries produced by
  // earlier instructions, so only entries on the same object may be affected.
  if (IsAlloc(written_object)) {
    for (auto it = available.tables.begin(); it != available.tables.end();) {
      if (it->first.object == written_object &&
          KeysMayAlias(it->first.key, it->first.key_is_const_string_index,
                       written_key, written_key_is_const_index)) {
        it = available.tables.erase(it);
      } else {
        ++it;
      }
    }
    return;
  }

  for (auto it = available.tables.begin(); it != available.tables.end();) {
    if (ObjectsMustNotAlias(it->first.object, written_object)) {
      ++it;
      continue;
    }
    if (KeysMayAlias(it->first.key, it->first.key_is_const_string_index,
                     written_key, written_key_is_const_index)) {
      auto to_erase = it++;
      available.tables.erase(to_erase);
    } else {
      ++it;
    }
  }
}

bool LoadStoreElimination::ObjectsMustNotAlias(Value* o1, Value* o2) {
  if (o1 == o2) return false;
  if (!o1 || !o2) return false;

  auto is_alloc = [](Value* v) {
    ValueKind k = v->GetKind();
    return k == ValueKind::NewTableInstKind ||
           k == ValueKind::NewArrayInstKind ||
           k == ValueKind::CreateBlockContextInstKind ||
           k == ValueKind::CreateFunctionContextInstKind ||
           k == ValueKind::CreateClosureInstKind;
  };

  bool a1 = is_alloc(o1);
  bool a2 = is_alloc(o2);
  if (a1 && a2) {
    return true;
  }

  return false;
}

bool LoadStoreElimination::KeysMayAlias(Value* k1, bool k1_is_const_index,
                                        Value* k2, bool k2_is_const_index) {
  if (!k1 || !k2) return true;
  if (k1 == k2) return true;

  // Only interpret LiteralUint32 as const-table index when the producer is
  // SetTableConstStringKeyInst (i.e. k*_is_const_index is true). Treating all
  // LiteralUint32 as const-table indices is UNSOUND: numeric keys like 1 must
  // still alias with string key "1".
  lepus::Value v1 =
      k1_is_const_index ? GetConstTableValueFromIndex(k1) : GetLepusValue(k1);
  lepus::Value v2 =
      k2_is_const_index ? GetConstTableValueFromIndex(k2) : GetLepusValue(k2);

  if (!v1.IsNil() && !v2.IsNil()) {
    if (v1.IsEqual(v2)) return true;
    // Refine number/string aliasing using ToPropertyKey equivalence for
    // *constant* keys:
    //   - 1   aliases  "1"
    //   - 1   does NOT alias "foo"
    // This is safe because the conversion result of a constant number is
    // deterministic.
    if (v1.IsNumber() && v2.IsString()) {
      return ConstNumberStringKeysMayAlias(v1, v2);
    }
    if (v1.IsString() && v2.IsNumber()) {
      return ConstNumberStringKeysMayAlias(v2, v1);
    }
    return false;
  }

  TypeOp* t1 = k1->GetType();
  TypeOp* t2 = k2->GetType();

  // At least one is not a constant.
  if (t1 && t2) {
    if (t1->IsAnyType() || t2->IsAnyType()) return true;

    bool is_num1 = t1->IsNumberType() || t1->GetTypeKind() == TypeOp::Number;
    bool is_num2 = t2->IsNumberType() || t2->GetTypeKind() == TypeOp::Number;
    bool is_str1 = t1->IsStringType();
    bool is_str2 = t2->IsStringType();

    if (t1->GetTypeKind() == t2->GetTypeKind() && !is_num1 && !is_str1)
      return true;

    if (is_num1 && is_num2) return true;
    if (is_str1 && is_str2) return true;

    if (is_num1 && is_str2) return true;
    if (is_str1 && is_num2) return true;

    return false;
  }

  return true;
}

lepus::Value LoadStoreElimination::GetConstTableValueFromIndex(Value* v) {
  auto lepus_func = func_ ? func_->GetLepusFunction() : nullptr;
  if (!lepus_func) return lepus::Value();

  // The const index is typically encoded as LiteralUint32.
  if (auto* u32 = llvh::dyn_cast<LiteralUint32>(v)) {
    uint32_t idx = u32->GetValue();
    if (idx < lepus_func->GetConstValue().size()) {
      return *lepus_func->GetConstValue(idx);
    }
    return lepus::Value();
  }
  // Or encoded as LoadConstInst(<uint32 index>).
  if (auto* load_const_inst = llvh::dyn_cast<LoadConstInst>(v)) {
    if (auto* i = llvh::dyn_cast<LiteralUint32>(load_const_inst->GetConst())) {
      uint32_t idx = i->GetValue();
      if (idx < lepus_func->GetConstValue().size()) {
        return *lepus_func->GetConstValue(idx);
      }
    }
  }
  return lepus::Value();
}

lepus::Value LoadStoreElimination::GetLepusValue(Value* v) {
  if (auto* literal = llvh::dyn_cast<Literal>(v)) {
    switch (literal->GetKind()) {
      case ValueKind::LiteralInt8Kind:
        return lepus::Value(
            static_cast<int32_t>(llvh::cast<LiteralInt8>(literal)->GetValue()));
      case ValueKind::LiteralInt32Kind:
        return lepus::Value(llvh::cast<LiteralInt32>(literal)->GetValue());
      case ValueKind::LiteralUint8Kind:
        return lepus::Value(static_cast<uint32_t>(
            llvh::cast<LiteralUint8>(literal)->GetValue()));
      case ValueKind::LiteralUint32Kind:
        return lepus::Value(llvh::cast<LiteralUint32>(literal)->GetValue());
      case ValueKind::LiteralFloat64Kind:
        return lepus::Value(llvh::cast<LiteralFloat64>(literal)->GetValue());
      case ValueKind::LiteralBoolKind:
        return lepus::Value(llvh::cast<LiteralBool>(literal)->GetValue());
      default:
        break;
    }
  } else if (auto* load_const_inst = llvh::dyn_cast<LoadConstInst>(v)) {
    Literal* lit = load_const_inst->GetConst();
    if (auto* u32 = llvh::dyn_cast<LiteralUint32>(lit)) {
      auto lepus_func = func_->GetLepusFunction();
      if (lepus_func && u32->GetValue() < lepus_func->GetConstValue().size()) {
        return *lepus_func->GetConstValue(u32->GetValue());
      }
    }
    return GetLepusValue(lit);
  }
  return lepus::Value();
}

Pass* CreateLoadStoreElimination(IRContext* ir_ctx) {
  return new LoadStoreElimination(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
