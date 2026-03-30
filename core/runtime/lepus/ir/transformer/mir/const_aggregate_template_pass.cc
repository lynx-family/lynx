// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <functional>

#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/literal.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {
class ConstAggregateTemplatePass : public FunctionPass {
 public:
  explicit ConstAggregateTemplatePass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "const-aggregate-template") {}

  bool RunOnFunction(FuncOp* func) override {
    auto lepus_func = func->GetLepusFunction();
    if (!lepus_func) return false;

    auto* builder = ir_ctx_->GetOpBuilder();
    if (!builder) return false;

    bool changed = false;

    for (auto& block : *func) {
      llvh::SmallVector<Instruction*, 16> to_remove;
      for (auto it = block.InstBegin(); it != block.InstEnd(); ++it) {
        auto* inst = *it;
        if (auto* new_array = llvh::dyn_cast<NewArrayInst>(inst)) {
          llvh::SmallVector<Instruction*, 8> remove_local;
          if (auto* repl = TryRewriteArray(ir_ctx_, *builder, lepus_func.get(),
                                           new_array, &remove_local)) {
            inst->ReplaceAllUsesWith(repl);
            to_remove.push_back(inst);
            to_remove.append(remove_local.begin(), remove_local.end());
            changed = true;
            continue;
          }
        } else if (auto* new_table = llvh::dyn_cast<NewTableInst>(inst)) {
          llvh::SmallVector<Instruction*, 8> remove_local;
          if (auto* repl = TryRewriteTable(ir_ctx_, *builder, lepus_func.get(),
                                           new_table, &remove_local)) {
            inst->ReplaceAllUsesWith(repl);
            to_remove.push_back(inst);
            to_remove.append(remove_local.begin(), remove_local.end());
            changed = true;
            continue;
          }
        }
      }
      llvh::for_each(to_remove,
                     [](Instruction* inst) { inst->EraseFromParent(); });
    }
    return changed;
  }

 private:
  static bool IsReadonlySetStyleObjectSecondArg(IRContext* ir_ctx,
                                                Value* value) {
    if (!value) return false;

    bool has_user = false;
    for (auto* user : value->GetUsers()) {
      auto* call = llvh::dyn_cast<CallInst>(user);
      if (!call) return false;
      if (call->GetBuiltinFuncName() != tasm::kCFunctionSetStyleObject) {
        return false;
      }
      if (call->GetNumArguments() < 2 || call->GetArgument(1) != value) {
        return false;
      }
      has_user = true;
    }
    return has_user;
  }

  enum class LoadPolicy {
    kBorrowConst,
    kMaterialize,
  };

  static inline bool IsPureConstValue(const lepus::Value& v) {
    if (v.IsBool() || v.IsNumber() || v.IsString()) {
      return true;
    }
    return false;
  }

  static bool TryExtractConstValueFromIRValue(lepus::Function* lepus_func,
                                              Value* v,
                                              lepus::Value* out_value) {
    if (!lepus_func || !v || !out_value) return false;

    if (auto* load = llvh::dyn_cast<LoadConstInst>(v)) {
      auto* lit = llvh::dyn_cast<LiteralUint32>(load->GetConst());
      if (!lit) return false;
      auto* cv = lepus_func->GetConstValue(lit->GetValue());
      if (!cv || !IsPureConstValue(*cv)) return false;
      *out_value = *cv;
      return true;
    }

    // Allow a subset of literal forms.
    if (auto* b = llvh::dyn_cast<LiteralBool>(v)) {
      *out_value = lepus::Value(b->GetValue());
      return true;
    }
    if (auto* i32 = llvh::dyn_cast<LiteralInt32>(v)) {
      *out_value = lepus::Value(static_cast<int32_t>(i32->GetValue()));
      return true;
    }
    if (auto* u32 = llvh::dyn_cast<LiteralUint32>(v)) {
      *out_value = lepus::Value(static_cast<uint32_t>(u32->GetValue()));
      return true;
    }
    if (auto* f64 = llvh::dyn_cast<LiteralFloat64>(v)) {
      *out_value = lepus::Value(f64->GetValue());
      return true;
    }

    return false;
  }

  static LoadPolicy DecideLoadPolicy(
      Value* aggregate,
      const llvh::SmallPtrSetImpl<Instruction*>& init_writes) {
    // Conservative policy:
    // - Borrow const only when we can prove the aggregate never escapes and is
    //   only used in known-readonly contexts.
    // - Otherwise materialize a fresh clone to preserve identity/mutation.
    llvh::SmallPtrSet<Value*, 16> visited;
    llvh::SmallVector<Value*, 16> work_list;
    work_list.push_back(aggregate);

    while (!work_list.empty()) {
      Value* v = work_list.pop_back_val();
      if (!visited.insert(v).second) continue;

      for (auto* user : v->GetUsers()) {
        auto* inst = llvh::dyn_cast<Instruction>(user);
        if (!inst) {
          return LoadPolicy::kMaterialize;
        }
        if (init_writes.count(inst)) continue;

        if (auto* phi = llvh::dyn_cast<PhiInst>(inst)) {
          work_list.push_back(phi);
          continue;
        }

        // Escaping/identity-observable or unknown contexts.
        if (llvh::isa<ReturnInst>(inst) ||
            llvh::isa<SetToplevelVarInst>(inst) ||
            llvh::isa<SetToplevelClosureVarInst>(inst) ||
            llvh::isa<SetUpvalueInst>(inst) ||
            llvh::isa<SetContextSlotInst>(inst) ||
            llvh::isa<SetContextSlotMovInst>(inst)) {
          return LoadPolicy::kMaterialize;
        }

        if (auto* call = llvh::dyn_cast<CallInst>(inst)) {
          // Only allow the known readonly style setter second-arg case.
          if (call->GetBuiltinFuncName() == tasm::kCFunctionSetStyleObject &&
              call->GetNumArguments() >= 2 && call->GetArgument(1) == v) {
            continue;
          }
          return LoadPolicy::kMaterialize;
        }

        // Readonly property reads are ok.
        if (llvh::isa<GetTableInst>(inst) ||
            llvh::isa<GetTableConstStringKeyInst>(inst)) {
          continue;
        }

        // Anything else is treated as potentially escaping or observing
        // identity (e.g. ToString, comparisons, unknown arithmetic, etc.).
        return LoadPolicy::kMaterialize;
      }
    }

    return LoadPolicy::kBorrowConst;
  }

  static bool BuildPureConstArray(lepus::Function* lepus_func,
                                  NewArrayInst* array,
                                  lepus::Value* out_value) {
    if (!lepus_func || !array || !out_value || array->GetArraySize() == 0) {
      return false;
    }

    auto ary = lepus::CArray::Create();
    ary->reserve(array->GetArraySize());
    for (auto i = 0; i < array->GetArraySize(); ++i) {
      auto* item = array->GetOperand(i);
      lepus::Value elem;
      if (!TryExtractConstValueFromIRValue(lepus_func, item, &elem)) {
        return false;
      }
      ary->push_back(std::move(elem));
    }

    lepus::Value result(std::move(ary));
    if (!result.MarkConst()) return false;
    *out_value = std::move(result);
    return true;
  }

  static Instruction* TryRewriteArray(
      IRContext* ir_ctx, OpBuilder& builder, lepus::Function* lepus_func,
      NewArrayInst* array, llvh::SmallVectorImpl<Instruction*>* out_to_remove) {
    if (!array || array->GetArraySize() <= 1) return nullptr;

    lepus::Value aggregate;
    if (!BuildPureConstArray(lepus_func, array, &aggregate)) return nullptr;

    llvh::SmallPtrSet<Instruction*, 4> init_writes;
    const auto policy = DecideLoadPolicy(array, init_writes);

    const uint32_t idx =
        static_cast<uint32_t>(lepus_func->AddConstValue(aggregate));
    if (idx > 0xFFFF) return nullptr;

    builder.SetInsertionPointAfter(array);
    if (policy == LoadPolicy::kBorrowConst) {
      return builder.Create<LoadConstInst>(array->GetLocation(),
                                           builder.GetLiteralUint32(idx),
                                           array->GetType());
    }
    return builder.Create<LoadConstMaterializeInst>(
        array->GetLocation(), builder.GetLiteralUint32(idx), array->GetType());
  }

  static bool BuildPureConstTable(
      lepus::Function* lepus_func,
      const llvh::SmallVectorImpl<Instruction*>& init_sets,
      lepus::Value* out_value) {
    if (!lepus_func || init_sets.empty() || !out_value) return false;

    auto dict = lepus::Dictionary::Create();
    dict->reserve(init_sets.size());

    for (auto* inst : init_sets) {
      base::String key_str;
      lepus::Value val;

      if (auto* set_cs = llvh::dyn_cast<SetTableConstStringKeyInst>(inst)) {
        auto* lit = llvh::dyn_cast<LiteralUint32>(set_cs->GetConstIndex());
        if (!lit) return false;
        auto* k = lepus_func->GetConstValue(lit->GetValue());
        if (!k || !k->IsString()) return false;
        key_str = k->String();
        if (!TryExtractConstValueFromIRValue(lepus_func, set_cs->GetStoreVal(),
                                             &val)) {
          return false;
        }
      } else if (auto* set = llvh::dyn_cast<SetTableInst>(inst)) {
        lepus::Value key;
        if (!TryExtractConstValueFromIRValue(lepus_func, set->GetProp(),
                                             &key)) {
          return false;
        }
        if (!key.IsString()) return false;
        key_str = key.String();
        if (!TryExtractConstValueFromIRValue(lepus_func, set->GetStoreVal(),
                                             &val)) {
          return false;
        }
      } else {
        return false;
      }

      // Overwrite semantics match JS object literal (last wins).
      dict->SetValue(key_str, std::move(val));
    }

    lepus::Value result(std::move(dict));
    if (!result.MarkConst()) return false;
    *out_value = std::move(result);
    return true;
  }

  static bool CollectConstTableInit(
      IRContext* ir_ctx, lepus::Function* lepus_func, NewTableInst* table,
      llvh::SmallVectorImpl<Instruction*>& out_init_sets,
      llvh::SmallPtrSetImpl<Instruction*>& out_init_set_set,
      Instruction** out_last_init) {
    if (!table || !lepus_func || !out_last_init) return false;
    Block* block = table->GetParent();
    if (!block) return false;

    // Only support the common straight-line initialization pattern within the
    // same block.
    for (auto* user : table->GetUsers()) {
      auto* user_inst = llvh::dyn_cast<Instruction>(user);
      if (!user_inst || user_inst->GetParent() != block) {
        return false;
      }
    }

    bool seen_non_init_use = false;
    Instruction* last_init = nullptr;
    for (auto it = block->InstBegin(); it != block->InstEnd(); ++it) {
      auto* inst = *it;
      if (inst == table) {
        // Start scanning after NewTable.
        continue;
      }
      if (inst->GetParent() != block) continue;
      bool uses_table = false;
      for (unsigned oi = 0; oi < inst->GetNumOperands(); ++oi) {
        if (inst->GetOperand(oi) == table) {
          uses_table = true;
          break;
        }
      }
      if (!uses_table) continue;

      // Allow binding the fresh object to toplevel variable before finishing
      // constant initialization. This is a common lowering pattern for large
      // object literals.
      if (auto* set_tv = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
        if (set_tv->GetSrc() == table) {
          continue;
        }
      }
      if (auto* set_tc = llvh::dyn_cast<SetToplevelClosureVarInst>(inst)) {
        if (set_tc->GetSrc() == table) {
          continue;
        }
      }

      // Initialization writes must happen before any non-init use.
      bool is_init = false;
      if (auto* set_cs = llvh::dyn_cast<SetTableConstStringKeyInst>(inst)) {
        is_init = (set_cs->GetObject() == table);
      } else if (auto* set = llvh::dyn_cast<SetTableInst>(inst)) {
        is_init = (set->GetObject() == table);
      }

      if (is_init && !seen_non_init_use) {
        // Only accept if key/value are compile-time constants we can encode.
        if (auto* set_cs = llvh::dyn_cast<SetTableConstStringKeyInst>(inst)) {
          auto* lit = llvh::dyn_cast<LiteralUint32>(set_cs->GetConstIndex());
          if (!lit) return false;
          auto* k = lepus_func->GetConstValue(lit->GetValue());
          if (!k || !k->IsString()) return false;
          lepus::Value tmp;
          if (!TryExtractConstValueFromIRValue(lepus_func,
                                               set_cs->GetStoreVal(), &tmp)) {
            return false;
          }
        } else if (auto* set = llvh::dyn_cast<SetTableInst>(inst)) {
          lepus::Value tmp;
          if (!TryExtractConstValueFromIRValue(lepus_func, set->GetProp(),
                                               &tmp) ||
              !tmp.IsString()) {
            return false;
          }
          if (!TryExtractConstValueFromIRValue(lepus_func, set->GetStoreVal(),
                                               &tmp)) {
            return false;
          }
        } else {
          return false;
        }

        out_init_sets.push_back(inst);
        out_init_set_set.insert(inst);
        last_init = inst;
        continue;
      }

      // Any other use marks the end of initialization.
      seen_non_init_use = true;
      // If we see further writes after a non-init use, bail out.
      if (is_init) return false;
    }

    if (out_init_sets.empty() || !last_init) return false;
    *out_last_init = last_init;
    return true;
  }

  static Instruction* TryRewriteTable(
      IRContext* ir_ctx, OpBuilder& builder, lepus::Function* lepus_func,
      NewTableInst* table, llvh::SmallVectorImpl<Instruction*>* out_to_remove) {
    if (!table || !out_to_remove) return nullptr;

    llvh::SmallVector<Instruction*, 16> init_sets;
    llvh::SmallPtrSet<Instruction*, 16> init_set_set;
    Instruction* last_init = nullptr;
    if (!CollectConstTableInit(ir_ctx, lepus_func, table, init_sets,
                               init_set_set, &last_init)) {
      return nullptr;
    }

    // Skip tiny aggregates to avoid growing const pool for low/negative ROI.
    if (init_sets.size() <= 1) return nullptr;

    lepus::Value aggregate;
    if (!BuildPureConstTable(lepus_func, init_sets, &aggregate)) return nullptr;

    const uint32_t idx =
        static_cast<uint32_t>(lepus_func->AddConstValue(aggregate));
    if (idx > 0xFFFF) return nullptr;

    const auto policy = DecideLoadPolicy(table, init_set_set);

    // Insert immediately after the NewTable so the replacement dominates all
    // uses (including early SetToplevelVar binds).
    builder.SetInsertionPointAfter(table);
    Instruction* repl = nullptr;
    if (policy == LoadPolicy::kBorrowConst) {
      repl = builder.Create<LoadConstInst>(table->GetLocation(),
                                           builder.GetLiteralUint32(idx),
                                           table->GetType());
    } else {
      repl = builder.Create<LoadConstMaterializeInst>(
          table->GetLocation(), builder.GetLiteralUint32(idx),
          table->GetType());
    }

    // Drop the initialization writes; DCE will clean up now-dead const loads.
    out_to_remove->append(init_sets.begin(), init_sets.end());
    return repl;
  }
};

Pass* CreateConstAggregateTemplatePass(IRContext* ir_ctx) {
  return new ConstAggregateTemplatePass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
