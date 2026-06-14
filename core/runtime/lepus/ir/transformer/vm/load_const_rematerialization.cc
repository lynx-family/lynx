// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/load_const_rematerialization.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/literal.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/vm/hvm_register_allocator.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

// Heuristic knobs for the first, more selective phase.
//
// The pass tries to shorten the live range of scalar LoadConst values by
// cloning them closer to aggregate-building users. The gap threshold controls
// how far a user may drift away from the current anchor before we decide to
// insert another rematerialized load in the same block.
constexpr size_t kConstRematerializationGap = 32;
constexpr size_t kConstRematerializationMinUsers = 4;
constexpr size_t kConstRematerializationMinSpan = 64;

// A weaker heuristic for "long tail" cases. These values are used when the
// constant is not hot enough for the primary heuristic, but still lives long
// enough across a small number of aggregate users to be worth splitting.
constexpr size_t kConstRematerializationLongTailMinUsers = 2;
constexpr size_t kConstRematerializationLongTailMinSpan = 256;

// Give hot aggregate actions a stable priority boost so they are consumed
// before long-tail actions when we sort the work list.
constexpr size_t kHotAggregateActionScoreBonus = 32;

// A lightweight wrapper for the preliminary register-allocation result used by
// this pass. We need both the estimated peak register usage and the temporary
// allocation membership so later heuristics can ask questions such as
// "is this LoadConst actually allocated?" without rerunning the same analysis.
struct PreliminaryRAResult {
  llvh::DenseSet<const Instruction*> allocated_insts;
  unsigned max_register_usage{0};
};

// One candidate rewrite.
//
// - load_inst: the original long-lived LoadConst.
// - insertion_user: the user before which we materialize a clone.
// - users: the aggregate users that will be rewired to the clone.
// - score: a greedy priority score; larger score means we expect a bigger live
//   range reduction and therefore a better chance to lower peak pressure.
struct RematerializationAction {
  LoadConstInst* load_inst{nullptr};
  Instruction* insertion_user{nullptr};
  llvh::SmallVector<Instruction*, 4> users;
  size_t score{0};
};

// Only aggregate-building users are interesting for this pass. They are the
// patterns that tend to keep many constants live simultaneously while building
// arrays / tables.
bool IsAggregateHotspotUser(const Instruction* user) {
  return user &&
         (llvh::isa<NewArrayInst>(user) || llvh::isa<SetTableInst>(user) ||
          llvh::isa<SetTableConstStringKeyInst>(user));
}

// A conservative, cheap upper bound for the root function. The register
// allocator can never use more physical registers than:
//   preallocated root prefix registers + all instruction results.
//
// This bound is intentionally loose but extremely cheap. If it already fits in
// the 8-bit VM budget, the root function cannot overflow and we can skip the
// expensive preliminary RA entirely.
unsigned ComputeCheapRootRegisterUpperBound(IRContext* ir_ctx, FuncOp* func) {
  if (LEPUS_UNLIKELY(!func->IsToplevelFunc())) return 0;

  unsigned upper_bound =
      static_cast<unsigned>(ir_ctx->GetToplevelVariables().size());
  for (auto& bb : *func) {
    for (auto& op : bb) {
      if (llvh::isa<Instruction>(&op)) {
        ++upper_bound;
      }
    }
  }
  return upper_bound;
}

unsigned GetShadowCloneRootPrefixCompensation(IRContext* ir_ctx, FuncOp* func) {
  if (!ir_ctx || !func || !func->IsToplevelFunc()) return 0;
  return static_cast<unsigned>(ir_ctx->GetToplevelVariables().size());
}

void CopyShadowCloneInstructionAttrs(const Instruction* source_inst,
                                     Instruction* cloned_inst) {
  if (!source_inst || !cloned_inst) return;
  cloned_inst->CopyAttrs(const_cast<Instruction*>(source_inst));

  // The shadow clone exists only for a temporary register-pressure estimate.
  // Keep RA-relevant attributes, but strip root-function special-register
  // markers so shadow-only Phi lowering cannot mutate the real IRContext maps.
  cloned_inst->SetClosureVarReg(constants::kInvalidSignedValue);
  cloned_inst->SetToplevelVarReg(constants::kInvalidSignedValue);
}

Value* GetOrCreateShadowCloneValue(
    OpBuilder* shadow_builder,
    llvh::DenseMap<const Value*, Value*>& cloned_values, Value* value) {
  if (!value) return nullptr;
  auto it = cloned_values.find(value);
  if (it != cloned_values.end()) return it->second;

  auto* literal = llvh::dyn_cast<Literal>(value);
  if (!literal || !shadow_builder) return value;

  Value* cloned_literal = nullptr;
  if (llvh::isa<LiteralNull>(literal)) {
    cloned_literal = shadow_builder->GetLiteralNull();
  } else if (llvh::isa<LiteralUndefined>(literal)) {
    cloned_literal = shadow_builder->GetLiteralUndefined();
  } else if (auto* lit = llvh::dyn_cast<LiteralInt8>(literal)) {
    cloned_literal = shadow_builder->GetLiteralInt8(lit->GetValue());
  } else if (auto* lit = llvh::dyn_cast<LiteralInt32>(literal)) {
    cloned_literal = shadow_builder->GetLiteralInt32(lit->GetValue());
  } else if (auto* lit = llvh::dyn_cast<LiteralUint8>(literal)) {
    cloned_literal = shadow_builder->GetLiteralUint8(lit->GetValue());
  } else if (auto* lit = llvh::dyn_cast<LiteralUint32>(literal)) {
    cloned_literal = shadow_builder->GetLiteralUint32(lit->GetValue());
  } else if (auto* lit = llvh::dyn_cast<LiteralFloat64>(literal)) {
    cloned_literal = shadow_builder->GetLiteralFloat64(lit->GetValue());
  } else if (auto* lit = llvh::dyn_cast<LiteralBool>(literal)) {
    cloned_literal = shadow_builder->GetLiteralBool(lit->GetValue());
  } else {
    throw ::lynx::lepus::CompileException(
        "Unhandled Literal subclass in GetOrCreateShadowCloneValue");
  }

  if (cloned_literal) {
    cloned_values[value] = cloned_literal;
    return cloned_literal;
  }
  return value;
}

// Run a temporary HVM register allocation on a disposable shadow clone of the
// root function. RegisterAllocator::Allocate() lowers Phi nodes and may insert
// call-function MovInsts, so running it directly on the real IR would pollute
// the pass input even when this optimization later decides to do nothing.
PreliminaryRAResult BuildPreliminaryRAOnShadowClone(IRContext* ir_ctx,
                                                    FuncOp* func) {
  PreliminaryRAResult result;
  OpBuilder shadow_builder;
  auto shadow_mod = std::make_unique<ModuleOp>(
      nullptr, &shadow_builder, 0, ir_ctx, "preliminary_ra_shadow_mod");
  shadow_builder.SetModuleOp(shadow_mod.get());
  shadow_mod->Init();
  shadow_builder.SetInsertionPointToEnd(shadow_mod->GetFunctionBlock());

  std::string shadow_name = func->GetName() + ".preliminary_ra_shadow";
  auto* shadow_func =
      shadow_builder.Create<FuncOp>(func->GetLocation(), shadow_name);
  shadow_builder.CreateRegion(shadow_func);

  llvh::DenseMap<const Value*, Value*> cloned_values;
  llvh::DenseMap<const Instruction*, Instruction*> cloned_insts;

  for (auto* param : func->GetParams()) {
    auto* shadow_param = shadow_func->CreateParam(param->GetParamIndex());
    shadow_param->SetType(param->GetType());
    shadow_param->CopyAttrs(param);
    cloned_values[param] = shadow_param;
  }

  for (auto& bb : *func) {
    auto* shadow_block = shadow_builder.CreateBlock(
        shadow_func->GetSingleRegion(), bb.GetType(), bb.GetLocation());
    cloned_values[&bb] = shadow_block;
  }

  for (auto& bb : *func) {
    auto* shadow_block = llvh::cast<Block>(cloned_values.lookup(&bb));
    shadow_builder.SetInsertionPointToEnd(shadow_block);
    for (auto& op : bb) {
      auto* inst = llvh::dyn_cast<Instruction>(&op);
      if (!inst) continue;

      llvh::SmallVector<Value*, 8> cloned_operands;
      cloned_operands.reserve(inst->GetNumOperands());
      for (unsigned i = 0, e = inst->GetNumOperands(); i < e; ++i) {
        Value* operand = inst->GetOperand(i);
        cloned_operands.push_back(GetOrCreateShadowCloneValue(
            &shadow_builder, cloned_values, operand));
      }

      Instruction* cloned_inst =
          shadow_builder.CloneInst(inst, cloned_operands);
      CopyShadowCloneInstructionAttrs(inst, cloned_inst);
      cloned_values[inst] = cloned_inst;
      cloned_insts[inst] = cloned_inst;
    }
  }

  for (const auto& entry : cloned_insts) {
    const Instruction* source_inst = entry.first;
    Instruction* cloned_inst = entry.second;
    for (unsigned i = 0, e = source_inst->GetNumOperands(); i < e; ++i) {
      Value* operand = source_inst->GetOperand(i);
      auto it = cloned_values.find(operand);
      if (it != cloned_values.end() &&
          cloned_inst->GetOperand(i) != it->second) {
        cloned_inst->SetOperand(it->second, i);
      }
    }
  }

  auto shadow_ra = std::make_unique<HVMRegisterAllocator>(shadow_func);
  PostOrderAnalysis po(shadow_func);
  llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
  shadow_ra->Preallocate();
  shadow_ra->Allocate(order);

  for (const auto& entry : cloned_insts) {
    if (shadow_ra->IsAllocated(entry.second)) {
      result.allocated_insts.insert(entry.first);
    }
  }
  result.max_register_usage =
      shadow_ra->GetMaxRegisterUsage() +
      GetShadowCloneRootPrefixCompensation(ir_ctx, func);

  shadow_ra.reset();

  // Detach all shadow instructions from the use-lists of their operands.
  // The cascade destruction of shadow_mod does NOT call EraseFromParent
  // (which sets operands to nullptr); without this cleanup, any operand
  // referencing an original value (e.g. an external FuncOp callee) would
  // leave a dangling entry in that value's users_ list.
  for (auto& bb : *shadow_func) {
    for (auto& op : bb) {
      if (auto* inst = llvh::dyn_cast<Instruction>(&op)) {
        for (unsigned i = 0, e = inst->GetNumOperands(); i < e; ++i)
          inst->SetOperand(nullptr, i);
      }
    }
  }

  return result;
}

// Check whether a specific user still references a given value. This is used
// after earlier rewrites have happened: an action may become stale if another
// action already rewired all of its target users.
bool InstructionUsesValue(Instruction* user, Value* value) {
  for (unsigned i = 0, e = user->GetNumOperands(); i < e; ++i) {
    if (user->GetOperand(i) == value) return true;
  }
  return false;
}

// If a non-aggregate user still appears after the planned rematerialization
// point, the original LoadConst must stay live across that point. In that case
// rematerializing aggregate users there cannot shorten the original live range
// the way this pass expects, so skip the action entirely.
bool HasBlockingNonAggregateUserAfterOrder(
    const llvh::DenseMap<const LoadConstInst*, size_t>&
        max_blocking_non_aggregate_user_order,
    const LoadConstInst* inst, size_t rematerialization_order) {
  auto it = max_blocking_non_aggregate_user_order.find(inst);
  return it != max_blocking_non_aggregate_user_order.end() &&
         it->second > rematerialization_order;
}

// We only rematerialize cheap scalar constants. Duplicating large structured
// values would increase code size and may not reduce pressure in a predictable
// way. The allocator check also makes sure the instruction participates in the
// current preliminary RA result.
bool IsScalarRematerializationLoad(
    LoadConstInst* inst,
    const llvh::DenseSet<const Instruction*>& allocated_insts) {
  if (allocated_insts.count(inst) == 0) return false;
  auto* ty = inst->GetType();
  if (!ty) return false;
  return ty->IsNumberType() || ty->IsBooleanType() || ty->IsStringType() ||
         ty->IsNullOrUndefinedType();
}

LoadConstRematerializationPass::RematerializationPriority
LoadConstRematerializationPass::GetRematerializationPriority(
    const LoadConstInst* inst,
    const llvh::DenseSet<const Instruction*>& allocated_insts,
    size_t aggregate_user_count, size_t span) {
  // Priority assignment is intentionally conservative. We first prove the
  // value is a scalar constant that is visible to the preliminary allocator,
  // then classify it by how many aggregate users it has and how far its live
  // range spans across the function.
  auto* ty = inst->GetType();
  if (allocated_insts.count(inst) == 0) {
    return RematerializationPriority::kNone;
  }
  if (!(ty->IsNumberType() || ty->IsBooleanType() || ty->IsStringType() ||
        ty->IsNullOrUndefinedType())) {
    return RematerializationPriority::kNone;
  }

  if (aggregate_user_count >= kConstRematerializationMinUsers &&
      span >= kConstRematerializationMinSpan) {
    return RematerializationPriority::kHotAggregate;
  }

  if (aggregate_user_count >= kConstRematerializationLongTailMinUsers &&
      span >= kConstRematerializationLongTailMinSpan) {
    return RematerializationPriority::kLongTailAggregate;
  }

  return RematerializationPriority::kNone;
}

bool LoadConstRematerializationPass::ShouldSkipUsers(
    const LoadConstInst* inst) {
  // Phi users are intentionally excluded. Rewiring a value around Phi nodes
  // requires CFG-aware updates and could silently change SSA shape; this pass
  // is meant to stay local and cheap.
  for (Instruction* user : inst->GetUsers()) {
    if (llvh::isa<PhiInst>(user)) return true;
  }
  return false;
}

void LoadConstRematerializationPass::ReplaceAllOperandUsesInInstruction(
    Instruction* user, Value* from, Value* to) {
  // Only rewrite this one instruction. The pass deliberately does not call a
  // global ReplaceAllUsesWith(), because each rematerialized clone is meant to
  // serve a narrow local region instead of taking over every use of the source.
  if (from == to) return;
  for (unsigned i = 0, e = user->GetNumOperands(); i < e; ++i) {
    if (user->GetOperand(i) == from) {
      user->SetOperand(to, i);
    }
  }
}

bool LoadConstRematerializationPass::RunOnModule(ModuleOp* mod) {
  // Root-only: child/non-toplevel functions are intentionally ignored to avoid
  // paying a preliminary-RA cost for every function in the module.
  auto* func = mod->GetRootFunction();
  if (!func->IsToplevelFunc()) return false;

  // Step 0: cheap pre-filter for the root/toplevel function.
  // If even the conservative structural upper bound already fits in the 8-bit
  // VM budget, the root function cannot overflow and we avoid preliminary RA.
  const unsigned cheap_upper_bound =
      ComputeCheapRootRegisterUpperBound(ir_ctx_, func);
  if (cheap_upper_bound <= Register::kMaxRegistersLimit) {
    return false;
  }

  // Preliminary RA may lower Phi incoming values into MovInsts, which would
  // hide the original Phi use from later user scans. Snapshot such candidates
  // before any temporary RA mutates the IR so we can conservatively skip them.
  llvh::DenseSet<LoadConstInst*> loads_with_original_phi_users;
  for (auto& bb : *func) {
    for (auto& op : bb) {
      auto* load_inst = llvh::dyn_cast<LoadConstInst>(&op);
      if (!load_inst) continue;
      for (Instruction* user : load_inst->GetUsers()) {
        if (user && llvh::isa<PhiInst>(user)) {
          loads_with_original_phi_users.insert(load_inst);
          break;
        }
      }
    }
  }

  // Step 1: run a preliminary allocation over the root function only.
  // If peak usage already fits in the 8-bit VM register budget, the pass is a
  // no-op and we avoid touching the IR entirely.
  PreliminaryRAResult initial_preliminary_ra =
      BuildPreliminaryRAOnShadowClone(ir_ctx_, func);
  const unsigned initial_usage = initial_preliminary_ra.max_register_usage;
  if (initial_usage <= Register::kMaxRegistersLimit) {
    return false;
  }

  OpBuilder* builder = ir_ctx_->GetOpBuilder();
  llvh::SmallVector<LoadConstInst*, 32> candidates;
  // global instruction number
  llvh::DenseMap<Instruction*, size_t> inst_order;
  size_t next_inst_order = 0;

  // Build a deterministic global instruction order for the current function. We
  // use it both for span estimation and for scoring actions that split the
  // live range of a particular LoadConst.
  for (auto& bb : *func) {
    for (auto& op : bb) {
      if (auto* inst = llvh::dyn_cast<Instruction>(&op)) {
        inst_order[inst] = next_inst_order++;
      }
      if (auto* load_inst = llvh::dyn_cast<LoadConstInst>(&op)) {
        candidates.push_back(load_inst);
      }
    }
  }

  llvh::DenseMap<const LoadConstInst*, size_t>
      max_blocking_non_aggregate_user_order;
  for (auto* load_inst : candidates) {
    // Record the last non-aggregate use that still forces the original
    // LoadConst to stay live. Rematerializing beyond that point would not
    // shorten the original live range enough to be worthwhile.
    size_t max_blocking_order = 0;
    bool has_blocking_user = false;
    for (Instruction* user : load_inst->GetUsers()) {
      if (!user || IsAggregateHotspotUser(user) || llvh::isa<PhiInst>(user)) {
        continue;
      }
      auto it = inst_order.find(user);
      if (it == inst_order.end()) {
        max_blocking_order = std::numeric_limits<size_t>::max();
        has_blocking_user = true;
        break;
      }
      max_blocking_order = has_blocking_user
                               ? std::max(max_blocking_order, it->second)
                               : it->second;
      has_blocking_user = true;
    }
    if (has_blocking_user) {
      max_blocking_non_aggregate_user_order[load_inst] = max_blocking_order;
    }
  }

  auto get_order_or_max = [&](Instruction* inst) {
    if (!inst) return std::numeric_limits<size_t>::max();
    auto it = inst_order.find(inst);
    return it == inst_order.end() ? std::numeric_limits<size_t>::max()
                                  : it->second;
  };

  llvh::SmallVector<RematerializationAction, 64> actions;
  for (auto* load_inst : candidates) {
    // Collect only aggregate users first. Non-aggregate users do not contribute
    // to the "many constants live at once while building an object/array"
    // pattern that this pass is trying to relieve.
    const size_t load_order = inst_order.lookup(load_inst);
    size_t min_order = load_order;
    size_t max_order = min_order;
    size_t aggregate_user_count = 0;
    llvh::SmallVector<Instruction*, 16> aggregate_users;
    aggregate_users.reserve(load_inst->GetNumUsers());
    for (Instruction* user : load_inst->GetUsers()) {
      if (!IsAggregateHotspotUser(user)) continue;
      auto it = inst_order.find(user);
      if (it == inst_order.end()) continue;
      aggregate_users.push_back(user);
      ++aggregate_user_count;
      min_order = std::min(min_order, it->second);
      max_order = std::max(max_order, it->second);
    }
    if (aggregate_users.empty()) continue;

    const size_t span = max_order - min_order;
    const RematerializationPriority priority = GetRematerializationPriority(
        load_inst, initial_preliminary_ra.allocated_insts, aggregate_user_count,
        span);
    if (priority == RematerializationPriority::kNone ||
        loads_with_original_phi_users.count(load_inst) != 0 ||
        ShouldSkipUsers(load_inst)) {
      continue;
    }

    // Group users by block because rematerialization is block-local. Inside one
    // block we may need multiple clones if users are spread far apart; across
    // different blocks we always start a fresh local plan.
    llvh::SmallDenseMap<Block*, llvh::SmallVector<Instruction*, 8>, 8>
        users_by_block;
    for (Instruction* user : aggregate_users) {
      users_by_block[user->GetBlock()].push_back(user);
    }

    for (auto& entry : users_by_block) {
      Block* block = entry.first;
      auto& block_users = entry.second;
      std::sort(block_users.begin(), block_users.end(),
                [&](Instruction* lhs, Instruction* rhs) {
                  return inst_order.lookup(lhs) < inst_order.lookup(rhs);
                });

      const bool same_block = block == load_inst->GetBlock();
      // Cross-block users always start from a fresh local anchor because this
      // pass only rematerializes within the user's own block.
      const size_t original_order =
          same_block ? inst_order.lookup(load_inst) : 0;
      size_t current_anchor_order = original_order;
      bool current_uses_original = true;

      for (Instruction* user : block_users) {
        // Decide whether this user is still "close enough" to the current
        // anchor (either the original LoadConst or the last rematerialized
        // clone). If it is too far away, create a new action and let later
        // users in the same local segment share that new clone.
        const size_t user_order = inst_order.lookup(user);
        bool need_rematerialization = false;
        if (current_uses_original) {
          if (!same_block || user_order <= original_order ||
              user_order - original_order > kConstRematerializationGap) {
            need_rematerialization = true;
          }
        } else if (user_order >
                   current_anchor_order + kConstRematerializationGap) {
          need_rematerialization = true;
        }

        if (need_rematerialization) {
          if (HasBlockingNonAggregateUserAfterOrder(
                  max_blocking_non_aggregate_user_order, load_inst,
                  user_order)) {
            // Keep using the original constant when a later blocking user still
            // depends on it being live up to this point.
            current_uses_original = true;
            current_anchor_order = original_order;
            continue;
          }
          actions.push_back({load_inst, user, {}, 0});
          current_uses_original = false;
          current_anchor_order = user_order;
          actions.back().score =
              (user_order > load_order ? user_order - load_order : 1);
          if (priority == RematerializationPriority::kHotAggregate) {
            actions.back().score += kHotAggregateActionScoreBonus;
          }
        }

        // Once a new local anchor exists, subsequent users in the same segment
        // will be rewired to that clone.
        if (!current_uses_original) {
          actions.back().users.push_back(user);
          actions.back().score += 8;
        }
      }
    }
  }

  bool changed = false;
  auto apply_rematerialization_action =
      [&](const RematerializationAction& action) {
        if (!action.load_inst || !action.insertion_user ||
            action.users.empty()) {
          return false;
        }

        bool has_pending_user = false;
        for (Instruction* user : action.users) {
          if (InstructionUsesValue(user, action.load_inst)) {
            has_pending_user = true;
            break;
          }
        }
        if (!has_pending_user) return false;

        builder->SetInsertionPoint(action.insertion_user);
        auto* rematerialization = builder->Create<LoadConstInst>(
            action.load_inst->GetLocation(), action.load_inst->GetConst(),
            action.load_inst->GetType());
        for (Instruction* user : action.users) {
          ReplaceAllOperandUsesInInstruction(user, action.load_inst,
                                             rematerialization);
        }
        changed = true;
        return true;
      };
  if (!actions.empty()) {
    // Greedy order: handle the most promising live-range splits first. We plan
    // once from the initial preliminary RA and then apply all legal actions in
    // one shot without rerunning preliminary RA inside this pass.
    std::sort(actions.begin(), actions.end(),
              [&](const RematerializationAction& lhs,
                  const RematerializationAction& rhs) {
                if (lhs.score != rhs.score) return lhs.score > rhs.score;
                if (lhs.users.size() != rhs.users.size()) {
                  return lhs.users.size() > rhs.users.size();
                }
                const size_t lhs_load_order = get_order_or_max(lhs.load_inst);
                const size_t rhs_load_order = get_order_or_max(rhs.load_inst);
                if (lhs_load_order != rhs_load_order) {
                  return lhs_load_order < rhs_load_order;
                }
                return get_order_or_max(lhs.insertion_user) <
                       get_order_or_max(rhs.insertion_user);
              });

    for (size_t i = 0; i < actions.size(); ++i) {
      auto& action = actions[i];
      if (!action.load_inst || !action.insertion_user || action.users.empty()) {
        continue;
      }

      // Another earlier action may already have consumed every targeted use.
      // Skip stale actions instead of creating redundant clones.
      if (!apply_rematerialization_action(action)) continue;

      // Cross-block rematerialization is only fully effective when every
      // aggregate-only user block of the same LoadConst gets its own local
      // clone. Otherwise the original value may stay live in the source block,
      // and because planning no longer iterates with intermediate RA runs, we
      // can end up with partial, iteration-order-dependent rewrites. Finish the
      // remaining cross-block siblings for the same load immediately.
      if (action.insertion_user->GetBlock() != action.load_inst->GetBlock()) {
        for (size_t j = i + 1; j < actions.size(); ++j) {
          auto& sibling = actions[j];
          if (sibling.load_inst != action.load_inst ||
              !sibling.insertion_user ||
              sibling.insertion_user->GetBlock() ==
                  action.load_inst->GetBlock()) {
            continue;
          }
          apply_rematerialization_action(sibling);
        }
      }
    }
  }

  {
    // Phase 2 fallback:
    //
    // After the selective heuristic, switch to a more aggressive mode. Here we
    // allow one-user actions so we can keep shaving the remaining aggregate
    // live ranges, still without re-planning.
    llvh::SmallVector<RematerializationAction, 128> overflow_actions;
    overflow_actions.reserve(candidates.size());
    for (auto* load_inst : candidates) {
      if (!IsScalarRematerializationLoad(
              load_inst, initial_preliminary_ra.allocated_insts) ||
          loads_with_original_phi_users.count(load_inst) != 0 ||
          ShouldSkipUsers(load_inst)) {
        continue;
      }

      const size_t load_order = inst_order.lookup(load_inst);
      for (Instruction* user : load_inst->GetUsers()) {
        if (!IsAggregateHotspotUser(user) ||
            !InstructionUsesValue(user, load_inst)) {
          continue;
        }
        auto user_it = inst_order.find(user);
        const size_t user_order =
            user_it == inst_order.end() ? load_order : user_it->second;
        if (HasBlockingNonAggregateUserAfterOrder(
                max_blocking_non_aggregate_user_order, load_inst, user_order)) {
          continue;
        }
        if (user->GetBlock() == load_inst->GetBlock() &&
            user_order > load_order && user_order - load_order <= 1) {
          continue;
        }

        // Score by how much live range we expect to cut. Cross-block users get
        // an extra boost because they often keep the source value live across a
        // larger control-flow region.
        size_t score = user_order > load_order ? user_order - load_order : 1;
        if (user->GetBlock() != load_inst->GetBlock()) {
          score += kConstRematerializationLongTailMinSpan;
        }
        overflow_actions.push_back({load_inst, user, {user}, score});
      }
    }

    std::sort(overflow_actions.begin(), overflow_actions.end(),
              [&](const RematerializationAction& lhs,
                  const RematerializationAction& rhs) {
                if (lhs.score != rhs.score) return lhs.score > rhs.score;
                const size_t lhs_load_order = get_order_or_max(lhs.load_inst);
                const size_t rhs_load_order = get_order_or_max(rhs.load_inst);
                if (lhs_load_order != rhs_load_order) {
                  return lhs_load_order < rhs_load_order;
                }
                return get_order_or_max(lhs.insertion_user) <
                       get_order_or_max(rhs.insertion_user);
              });

    for (size_t i = 0; i < overflow_actions.size(); ++i) {
      auto& action = overflow_actions[i];
      if (!action.load_inst || !action.insertion_user || action.users.empty()) {
        continue;
      }

      // Re-check that the selected user still uses the original LoadConst. This
      // keeps the fallback idempotent even after earlier remats have changed
      // the IR shape.
      if (!InstructionUsesValue(action.insertion_user, action.load_inst)) {
        continue;
      }

      if (!apply_rematerialization_action(action)) continue;

      if (action.insertion_user->GetBlock() != action.load_inst->GetBlock()) {
        for (size_t j = i + 1; j < overflow_actions.size(); ++j) {
          auto& sibling = overflow_actions[j];
          if (sibling.load_inst != action.load_inst ||
              !sibling.insertion_user ||
              sibling.insertion_user->GetBlock() ==
                  action.load_inst->GetBlock() ||
              !InstructionUsesValue(sibling.insertion_user,
                                    sibling.load_inst)) {
            continue;
          }
          apply_rematerialization_action(sibling);
        }
      }
    }
  }

  if (!changed) return false;

  // Remove dead originals left behind after rewiring. Each source LoadConst is
  // kept until the end so action discovery can still iterate over the original
  // candidate list safely.
  for (auto* load_inst : candidates) {
    if (load_inst->GetNumUsers() == 0 && load_inst->GetBlock()) {
      load_inst->GetBlock()->Erase(load_inst);
    }
  }

  return true;
}

Pass* CreateLoadConstRematerializationPass(IRContext* ir_ctx) {
  return new LoadConstRematerializationPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
