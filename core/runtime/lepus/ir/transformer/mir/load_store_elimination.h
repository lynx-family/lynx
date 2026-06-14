// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_STORE_ELIMINATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_STORE_ELIMINATION_H_

#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class LoadStoreElimination : public FunctionPass {
 public:
  explicit LoadStoreElimination(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "lse") {}
  ~LoadStoreElimination() override = default;

  bool RunOnFunction(FuncOp* f) override;
  void Reset(FuncOp* f);

  struct TableKey {
    Value* object;
    Value* key;
    // True iff `key` is a const-table string index used by
    // SetTableConstStringKeyInst / GetTableConstStringKeyInst.
    bool key_is_const_string_index;

    struct NormalizedKey {
      enum Kind : uint8_t {
        // Fallback: compare by Value* identity.
        Ptr,
        // Const-table index (uint32).
        ConstIndex,
        // GetGlobalInst with a constant global index.
        GlobalIndex,
        // GetBuiltinInst with a constant builtin index.
        BuiltinIndex,
      } kind;
      uint64_t payload;
    };

    static bool ExtractInt32(Value* v, int32_t* out);
    static bool TryGetConstIndexFromLoadConst(Value* v, uint32_t* out);
    static bool TryGetConstIndex(Value* v,
                                 bool treat_literal_u32_as_const_index,
                                 uint32_t* out);
    static NormalizedKey Normalize(Value* v,
                                   bool treat_literal_u32_as_const_index);
    bool operator==(const TableKey& other) const;
  };

  struct TableKeyHash {
    std::size_t operator()(const TableKey& k) const {
      auto mix = [](uint64_t x) {
        // A simple 64-bit mix (splitmix64-ish).
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
      };

      TableKey::NormalizedKey o = TableKey::Normalize(
          k.object, /*treat_literal_u32_as_const_index*/ false);
      TableKey::NormalizedKey p =
          TableKey::Normalize(k.key, k.key_is_const_string_index);

      uint64_t h = mix(static_cast<uint64_t>(o.kind));
      h ^= mix(o.payload);
      h ^= mix(static_cast<uint64_t>(p.kind));
      h ^= mix(p.payload);
      return static_cast<std::size_t>(h);
    }
  };

  struct ContextKey {
    Value* context;
    Value* index;
    bool operator==(const ContextKey& other) const {
      return context == other.context && index == other.index;
    }
  };

  struct ContextKeyHash {
    std::size_t operator()(const ContextKey& k) const {
      return std::hash<Value*>()(k.context) ^ std::hash<Value*>()(k.index);
    }
  };

  struct AvailableValues {
    // GetTableInst, SetTableInst
    std::unordered_map<TableKey, Value*, TableKeyHash> tables;
    // GetGlobalInst
    std::unordered_map<Value*, Value*>
        globals;  // Name Index (Literal*) -> Value
    // GetBuiltinInst
    std::unordered_map<Value*, Value*> builtins;  // Index (Literal*) -> Value
    // GetToplevelClosureInst, SetToplevelClosureInst
    std::unordered_map<Value*, Value*>
        toplevel_closures;  // Register Index (Literal*) -> Value
    // GetToplevelVarInst, SetToplevelVarInst
    std::unordered_map<Value*, Value*>
        toplevel_vars;  // Register Index (Literal*) -> Value
    // LoadConstInst
    std::unordered_map<Value*, Value*>
        constants;  // Constant (Literal*) -> Value
    // GetUpValueInst, SetUpValueInst
    std::unordered_map<Value*, Value*> upvalues;  // Index (Literal*) -> Value
    // GetContextSlotInst, SetContextSlotInst
    std::unordered_map<ContextKey, Value*, ContextKeyHash> context_slots;

    void InvalidateMutableCaches() {
      tables.clear();
      toplevel_closures.clear();
      toplevel_vars.clear();
      upvalues.clear();
      context_slots.clear();
    }

    /// Keep only table entries whose receiver is a "safe" kind: property
    /// access results, parameters, or local allocations. Remove all others.
    void FilterTablesKeepingSafeReceivers() {
      for (auto it = tables.begin(); it != tables.end();) {
        Value* receiver = it->first.object;
        if (!receiver) {
          it = tables.erase(it);
          continue;
        }
        ValueKind k = receiver->GetKind();
        if (k == ValueKind::GetTableInstKind ||
            k == ValueKind::GetTableConstStringKeyInstKind ||
            k == ValueKind::ParameterKind || k == ValueKind::NewTableInstKind ||
            k == ValueKind::NewArrayInstKind) {
          ++it;
        } else {
          it = tables.erase(it);
        }
      }
    }

    /// Selectively clear upvalue entries, preserving those whose index is
    /// in the never-written set. If the set is empty, behavior depends on
    /// `clear_if_empty`: true = clear all, false = preserve all.
    void SelectivelyPreserveUpvalues(
        const std::set<uint8_t>& never_written_upvalues,
        bool clear_if_empty = true) {
      if (!never_written_upvalues.empty()) {
        for (auto it = upvalues.begin(); it != upvalues.end();) {
          auto* idx_lit = llvh::dyn_cast<LiteralUint8>(it->first);
          if (idx_lit &&
              never_written_upvalues.count(idx_lit->GetValue()) > 0) {
            ++it;
          } else {
            it = upvalues.erase(it);
          }
        }
      } else if (clear_if_empty) {
        upvalues.clear();
      }
      // else: preserve all (empty set + !clear_if_empty)
    }

    /// Selectively clear toplevel closure entries, preserving those whose
    /// register is in the never-written set.
    void SelectivelyPreserveToplevelClosures(
        const std::set<uint32_t>& never_written_closures) {
      if (!never_written_closures.empty()) {
        for (auto it = toplevel_closures.begin();
             it != toplevel_closures.end();) {
          auto* reg_lit = llvh::dyn_cast<LiteralUint32>(it->first);
          if (reg_lit &&
              never_written_closures.count(reg_lit->GetValue()) > 0) {
            ++it;
          } else {
            it = toplevel_closures.erase(it);
          }
        }
      } else {
        toplevel_closures.clear();
      }
    }

    /// Invalidation for native C++ renderer calls (IsLocalTableSafeCall).
    /// These are known C++ functions (__SetAttribute, __AppendElement, etc.)
    /// that cannot access or modify JS closure scopes, upvalue slots, context
    /// slots, or toplevel variables. We preserve ALL non-table caches except
    /// upvalue slots that are potentially written by other JS functions.
    void InvalidateForNativeRendererCall(
        const std::set<uint8_t>& never_written_upvalues) {
      // Native C++ functions have no access to JS closure/upvalue/context
      // state. Selectively clear upvalue entries for potentially-written slots.
      // If never_written_upvalues is empty, preserve all (no analysis info
      // available, default to safe for native renderer calls).
      SelectivelyPreserveUpvalues(never_written_upvalues,
                                  /*clear_if_empty=*/false);
      // Table cache: keep safe receivers only.
      FilterTablesKeepingSafeReceivers();
    }

    /// Selective invalidation for "local-table-safe" calls: the callee only
    /// writes to direct upvalue/closure table slots, not to parameter-derived
    /// or nested-property receivers. We can keep table entries whose receiver
    /// is from GetTableInst, Parameters, or local allocations.
    ///
    /// Non-table caches are selectively preserved:
    /// - Upvalue slots whose index is in `never_written_upvalues` survive.
    /// - Toplevel closure regs in `never_written_toplevel_closures` survive.
    /// - All other non-table caches (toplevel_vars, context_slots) are cleared.
    ///
    /// SAFETY INVARIANT (two-sided guarantee):
    /// 1. Analysis side (FunctionOwnWritesAreLocalTableSafe): rejects any
    ///    function that writes to GetTable-on-upvalue chains (e.g.,
    ///    upval.sub.x = v). This ensures the callee cannot modify objects
    ///    reachable via the caller's GetTableInst results.
    /// 2. Invalidation side (here): clears table entries whose receiver is
    ///    GetUpvalueInst/GetToplevelVarInst etc. This ensures that if the
    ///    callee writes directly to a shared upvalue table (e.g., shared.x =
    ///    v), the caller's cache for that upvalue-receiver entry is
    ///    invalidated.
    void InvalidateForLocalTableSafeCall(
        const std::set<uint8_t>& never_written_upvalues,
        const std::set<uint32_t>& never_written_toplevel_closures) {
      SelectivelyPreserveToplevelClosures(never_written_toplevel_closures);
      toplevel_vars.clear();
      context_slots.clear();
      SelectivelyPreserveUpvalues(never_written_upvalues);
      FilterTablesKeepingSafeReceivers();
    }
  };

 private:
  bool ProcessBlock(Block* bb, AvailableValues& available,
                    bool enable_elimination);
  bool KeysMayAlias(Value* k1, bool k1_is_const_index, Value* k2,
                    bool k2_is_const_index);
  Value* ResolveReplacement(Value* v);

  enum class PropertyReceiverKind : uint8_t {
    kUnknown,
    kTable,
    kArray,
  };

  PropertyReceiverKind GetPropertyReceiverKind(Value* object);
  bool CanTrackTableStore(Value* object, Value* key,
                          bool key_is_const_string_index);
  bool KeyMayBeString(Value* key, bool key_is_const_string_index);
  bool KeyMayBeNumber(Value* key, bool key_is_const_string_index);
  void InvalidateTables(AvailableValues& available, Value* written_object,
                        Value* written_key, bool written_key_is_const_index);
  bool ObjectsMustNotAlias(Value* o1, Value* o2);
  lepus::Value GetLepusValue(Value* v);
  lepus::Value GetConstTableValueFromIndex(Value* v);
  bool IsReadonlyCall(CallInst* call);
  void ComputeNeverWrittenUpvalues();
  void ComputeNeverWrittenToplevelClosures();
  void ComputeParamWriteProperties();

  FuncOp* func_ = nullptr;
  llvh::SmallVector<Instruction*, 16> to_remove_;
  llvh::DenseMap<Value*, Value*> replaced_by_;
  // Set of upvalue indices (uint8 values) for the current function whose
  // corresponding variable is never written (via SetUpvalueInst) anywhere
  // in the module.  These can be preserved across non-readonly calls.
  std::set<uint8_t> never_written_upvalue_indices_;
  // Set of toplevel closure register values that are never written (via
  // SetToplevelClosureVarInst/SetToplevelVarInst) anywhere in the module.
  bool toplevel_closure_computed_ = false;
  std::set<uint32_t> never_written_toplevel_closure_regs_;
  // Set of toplevel closure registers that ARE written somewhere in the module.
  // Used by table-safety analysis to reject name-based callee resolution when
  // the captured variable may have been reassigned.
  std::set<uint32_t> written_toplevel_closure_regs_;
  // Module-level analysis results (computed once, cached across functions).
  bool param_properties_computed_ = false;
  // True if no function in the module writes to a receiver that is directly
  // a Parameter (e.g., param.x = v). When true, table entries with a
  // Parameter receiver can be preserved across non-TP calls.
  bool param_properties_never_written_ = false;
  // True if no function in the module writes to a receiver that traces back
  // to a Parameter through GetTable chains (e.g., getTable(param, k).x = v).
  // When true, table entries with GetTable-on-param receivers are also safe.
  bool param_gettable_properties_never_written_ = false;

  // Interprocedural table-safety analysis (computed once, cached).
  bool table_safe_computed_ = false;
  // Set of function names whose calls are guaranteed not to modify any table
  // visible to the caller (no SetTable/SetTableConstStringKey transitively).
  std::set<std::string> table_safe_func_names_;
  void ComputeTableSafeFunctions();
  bool IsCallToTableSafeFunction(CallInst* call);

  // Result of callee resolution through GetUpvalue/GetToplevelClosureVar.
  struct ResolvedCallee {
    FuncOp* func_op = nullptr;  // Resolved FuncOp (GetToplevelClosureVar path)
    std::string name;           // Resolved name (GetUpvalue path)
    explicit operator bool() const { return func_op || !name.empty(); }
  };
  ResolvedCallee ResolveCallee(CallInst* call);
};

Pass* CreateLoadStoreElimination(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_STORE_ELIMINATION_H_
