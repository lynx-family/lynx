// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_STORE_ELIMINATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_STORE_ELIMINATION_H_

#include <unordered_map>

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

  // =========================
  // Exposed for unit tests
  // =========================
  // NOTE: These helpers are intentionally public so unit tests can validate
  // correctness/stability properties without using `#define private public`.
  // They are NOT meant to be used by production passes outside LSE.
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
  };

  // For unit tests: simulate internal replacement bookkeeping.
  void SetReplacementForTest(Value* from, Value* to) {
    replaced_by_[from] = to;
  }
  Value* GetReplacementForTest(Value* from) {
    auto it = replaced_by_.find(from);
    return it == replaced_by_.end() ? nullptr : it->second;
  }

  // Internal helpers exposed for unit tests.
  bool ProcessBlock(Block* bb, AvailableValues& available,
                    bool enable_elimination);
  bool KeysMayAlias(Value* k1, bool k1_is_const_index, Value* k2,
                    bool k2_is_const_index);
  Value* ResolveReplacement(Value* v);

 private:
  void InvalidateTables(AvailableValues& available, Value* written_object,
                        Value* written_key, bool written_key_is_const_index);
  bool ObjectsMustNotAlias(Value* o1, Value* o2);
  lepus::Value GetLepusValue(Value* v);
  lepus::Value GetConstTableValueFromIndex(Value* v);
  bool IsReadonlyCall(CallInst* call);

  FuncOp* func_ = nullptr;
  llvh::SmallVector<Instruction*, 16> to_remove_;
  llvh::DenseMap<Value*, Value*> replaced_by_;
};

Pass* CreateLoadStoreElimination(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_STORE_ELIMINATION_H_
