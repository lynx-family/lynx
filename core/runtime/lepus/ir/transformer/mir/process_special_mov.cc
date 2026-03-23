// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/process_special_mov.h"

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Casting.h"
#include "core/runtime/lepus/ir/op_builder.h"

namespace lynx {
namespace lepus {
namespace ir {

bool ProcessSpecialMovPass::IsSpecialMovInst(MovInst* mov_inst) {
  // if mov_inst has toplevelVarReg or ClosureVarReg, or the src of the mov_inst
  // has toplevelVarReg or ClosureVarReg, then this mov_inst is special.

  bool res = mov_inst->GetToplevelVarReg() != constants::kInvalidSignedValue ||
             mov_inst->GetClosureVarReg() != constants::kInvalidSignedValue;

  auto src = mov_inst->GetSingleOperand();
  if (llvh::isa<Instruction>(src)) {
    res |= (llvh::dyn_cast<Instruction>(src)->GetClosureVarReg() !=
                constants::kInvalidSignedValue ||
            llvh::dyn_cast<Instruction>(src)->GetToplevelVarReg() !=
                constants::kInvalidSignedValue);
  }

  return res;
}

void ProcessSpecialMovPass::RemoveNormalUselessMovInst(FuncOp* func) {
  to_removed_.clear();
  for (auto& block : *func) {
    for (auto& op : block) {
      if (auto* mov_inst = llvh::dyn_cast<MovInst>(&op)) {
        // skip special mov_inst for now.
        if (IsSpecialMovInst(mov_inst)) continue;

        // replace mov_inst with src
        mov_inst->ReplaceAllUsesWith(mov_inst->GetSingleOperand());
        to_removed_.push_back(mov_inst);
      }
    }
  }

  llvh::for_each(to_removed_,
                 [](MovInst* mov_inst) { mov_inst->EraseFromParent(); });
}

void ProcessSpecialMovPass::UpdateClosureAttributes(IRContext* ir_ctx,
                                                    MovInst* mov_inst) {
  auto closure_var_reg = mov_inst->GetClosureVarReg();
  if (LEPUS_UNLIKELY(closure_var_reg == constants::kInvalidSignedValue)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateClosureAttributes requires valid closure var "
        "reg");
  }
  [[maybe_unused]] auto dst_toplevel_reg = mov_inst->GetToplevelVarReg();
  if (LEPUS_UNLIKELY(closure_var_reg != dst_toplevel_reg)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateClosureAttributes requires closure var reg to "
        "equal dst toplevel reg");
  }

  auto root_func = ir_ctx->GetMainMod()->GetRootFunction();
  auto& toplevel_vars = ir_ctx->GetToplevelVariables();
  auto iter = toplevel_vars.find(closure_var_reg);
  if (LEPUS_UNLIKELY(iter == toplevel_vars.end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: UpdateClosureAttributes cannot find toplevel var for "
        "closure var reg");
  }
  root_func->UpdateClosureVar(mov_inst, iter->second);
}

void ProcessSpecialMovPass::ProcessMovInstForToplevelVar(
    IRContext* ir_ctx, MovInst* mov_inst, unsigned dst_toplevel_reg,
    unsigned src_toplevel_reg) {
  auto* builder = ir_ctx->GetOpBuilder();
  auto loc = mov_inst->GetLocation();
  Value* src_val = mov_inst->GetSingleOperand();

  if (dst_toplevel_reg != constants::kInvalidSignedValue) {
    // If source is also a top-level variable, get source value first
    if (src_toplevel_reg != constants::kInvalidSignedValue) {
      src_val = builder->Create<GetToplevelVarInst>(
          loc, builder->GetLiteralUint32(src_toplevel_reg),
          mov_inst->GetType());
    }

    // Create Set/Get instruction pair
    auto* set_inst = builder->Create<SetToplevelVarInst>(
        loc, builder->GetLiteralUint32(dst_toplevel_reg), src_val);
    auto* get_inst = builder->Create<GetToplevelVarInst>(
        loc, builder->GetLiteralUint32(dst_toplevel_reg), mov_inst->GetType());

    mov_inst->ReplaceAllUsesWith(get_inst);
    ir_ctx->UpdateToplevelVar(mov_inst, set_inst);
    // Still tag the store as toplevel-related so later passes can treat it
    // conservatively.
    set_inst->SetToplevelVarReg(dst_toplevel_reg);
    get_inst->SetToplevelVarReg(dst_toplevel_reg);

    // Process closure attributes if exists
    if (mov_inst->GetClosureVarReg() != constants::kInvalidSignedValue) {
      auto closure_var_reg = mov_inst->GetClosureVarReg();
      if (LEPUS_UNLIKELY(closure_var_reg != dst_toplevel_reg)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ProcessMovInstForToplevelVar detected mismatched "
            "closure var reg and dst toplevel reg");
      }
      /// if mov_inst->GetClosureVarReg != constants::kInvalidSignedValue, it
      /// means that this toplevel var is also a toplevel closure var. if we
      /// delete this movInst, we need to update the closure_reg_to_value_
      /// table.
      UpdateClosureAttributes(ir_ctx, mov_inst);
      // this moveInst may be the source of the other movInst.
      // So we need to update the ClosureVarReg of get_inst.
      get_inst->SetClosureVarReg(closure_var_reg);
      set_inst->SetClosureVarReg(closure_var_reg);
    }
    mov_inst->EraseFromParent();
  } else if (src_toplevel_reg != constants::kInvalidSignedValue) {
    // only src has toplevelVarReg
    auto* get_inst = builder->Create<GetToplevelVarInst>(
        loc, builder->GetLiteralUint32(src_toplevel_reg), mov_inst->GetType());

    //// need to check the mov_inst GetClosureVarReg
    if (mov_inst->GetClosureVarReg() != constants::kInvalidSignedValue) {
      // it means the closure variable is not a toplevel variable
      auto dst_closure_reg = mov_inst->GetClosureVarReg();
      builder->Create<SetToplevelClosureVarInst>(
          loc, builder->GetLiteralUint32(dst_closure_reg), get_inst);
      auto* get_inst_closure = builder->Create<GetToplevelClosureVarInst>(
          loc, builder->GetLiteralUint32(dst_closure_reg), mov_inst->GetType());
      get_inst_closure->SetClosureVarReg(dst_closure_reg);
      mov_inst->ReplaceAllUsesWith(get_inst_closure);
      ir_ctx_->UpdateSpecialAttribute(mov_inst, get_inst_closure);
    } else {
      // Replace mov_inst with get_inst
      mov_inst->ReplaceAllUsesWith(get_inst);
      ir_ctx_->UpdateSpecialAttribute(mov_inst, get_inst);
    }
    mov_inst->EraseFromParent();
  }
}

void ProcessSpecialMovPass::ProcessMovInstForClosureVar(IRContext* ir_ctx,
                                                        MovInst* mov_inst) {
  Value* src_val = mov_inst->GetSingleOperand();
  auto* builder = ir_ctx->GetOpBuilder();
  auto loc = mov_inst->GetLocation();

  // 2. process movInst with closureVarReg attribute
  auto src_closure_reg = src_val->GetClosureVarReg();
  auto dst_closure_reg = mov_inst->GetClosureVarReg();
  Value* replacement = nullptr;

  if (dst_closure_reg != constants::kInvalidSignedValue) {
    if (src_closure_reg != constants::kInvalidSignedValue) {
      // Both have ClosureVarReg - replace with Get + Set + Get
      src_val = builder->Create<GetToplevelClosureVarInst>(
          loc, builder->GetLiteralUint32(src_closure_reg), mov_inst->GetType());
    }

    builder->Create<SetToplevelClosureVarInst>(
        loc, builder->GetLiteralUint32(dst_closure_reg), src_val);
    auto* get_inst = builder->Create<GetToplevelClosureVarInst>(
        loc, builder->GetLiteralUint32(dst_closure_reg), mov_inst->GetType());

    get_inst->SetClosureVarReg(dst_closure_reg);
    replacement = get_inst;
  } else if (src_closure_reg != constants::kInvalidSignedValue) {
    // Only src has ClosureVarReg - replace with Get
    auto* get_inst = builder->Create<GetToplevelClosureVarInst>(
        loc, builder->GetLiteralUint32(src_closure_reg), mov_inst->GetType());
    replacement = get_inst;
  }
  if (LEPUS_UNLIKELY(!replacement)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: ProcessMovInstForClosureVar produced null replacement"
        " (expected valid ClosureVarReg on src or dst)");
  }
  // Replace mov_inst with set_inst
  mov_inst->ReplaceAllUsesWith(replacement);
  ir_ctx_->UpdateSpecialAttribute(mov_inst, replacement);
  mov_inst->EraseFromParent();
}

void ProcessSpecialMovPass::ProcessNonMovInstForSpecialAttribute(
    IRContext* ir_ctx, Instruction* inst) {
  auto* builder = ir_ctx->GetOpBuilder();

  if (inst->GetToplevelVarReg() != constants::kInvalidSignedValue) {
    /// insert setToplevelVarInst
    auto* set_toplevel_var = builder->Create<SetToplevelVarInst>(
        inst->GetLocation(),
        builder->GetLiteralUint32(inst->GetToplevelVarReg()), inst);
    ir_ctx_->UpdateSpecialAttribute(inst, set_toplevel_var);
  } else {
    if (LEPUS_UNLIKELY(inst->GetClosureVarReg() ==
                       constants::kInvalidSignedValue)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: ProcessNonMovInstForSpecialAttribute expects valid "
          "closure var reg when no toplevel var reg");
    }
    /// insert setToplevelClosureVarInst
    auto* set_closure_var = builder->Create<SetToplevelClosureVarInst>(
        inst->GetLocation(),
        builder->GetLiteralUint32(inst->GetClosureVarReg()), inst);
    ir_ctx_->UpdateSpecialAttribute(inst, set_closure_var);
  }

  inst->SetToplevelVarReg(constants::kInvalidSignedValue);
  inst->SetClosureVarReg(constants::kInvalidSignedValue);
}

bool ProcessSpecialMovPass::RunOnFunction(FuncOp* func) {
  // Only process toplevel function
  if (!func->IsToplevelFunc()) {
    return false;
  }

  IRContext* ir_ctx = func->GetIRCtx();
  auto* builder = ir_ctx->GetOpBuilder();

  // 1. remove all normal MovInst
  RemoveNormalUselessMovInst(func);

  // 2. collect all special MovInst
  llvh::SmallVector<MovInst*, 16> mov_insts;
  for (auto& block : *func->GetSingleRegion()) {
    for (auto& inst : block) {
      if (auto* mov_inst = llvh::dyn_cast<MovInst>(&inst)) {
        if (LEPUS_UNLIKELY(!IsSpecialMovInst(mov_inst))) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: ProcessSpecialMovPass expected collected "
              "MovInst to be special");
        }
        mov_insts.push_back(mov_inst);
      }
    }
  }

  // 3. process special movInst
  for (auto* mov_inst : mov_insts) {
    if (LEPUS_UNLIKELY(!IsSpecialMovInst(mov_inst))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: ProcessSpecialMovPass encountered non-special "
          "MovInst in special list");
    }
    builder->SetInsertionPoint(mov_inst);

    auto* src_val = mov_inst->GetSingleOperand();

    const int dst_toplevel_reg = mov_inst->GetToplevelVarReg();
    const int src_toplevel_reg =
        llvh::isa<Instruction>(src_val)
            ? llvh::dyn_cast<Instruction>(src_val)->GetToplevelVarReg()
            : -1;

    if (dst_toplevel_reg != constants::kInvalidSignedValue ||
        src_toplevel_reg != constants::kInvalidSignedValue) {
      // 3.1. process movInst with toplevelVarReg attribute
      ProcessMovInstForToplevelVar(ir_ctx, mov_inst, dst_toplevel_reg,
                                   src_toplevel_reg);
    } else {
      // 3.1. process movInst with closureVarReg attribute
      ProcessMovInstForClosureVar(ir_ctx, mov_inst);
    }
  }

  // 4. For inst instead of movInst with toplevelVarReg or closureVarReg
  // attribute, we insert a setToplevelVar or setToplevelClosureVar inst after
  // it. so the attribute in the inst will be deleted.
  for (auto& bb : *func) {
    for (auto& op : bb) {
      if (auto* inst = llvh::dyn_cast<Instruction>(&op)) {
        if ((inst->GetToplevelVarReg() != constants::kInvalidSignedValue ||
             inst->GetClosureVarReg() != constants::kInvalidSignedValue)) {
          // skip Get + Set
          if (llvh::isa<SetToplevelVarInst>(inst) ||
              llvh::isa<SetToplevelClosureVarInst>(inst) ||
              llvh::isa<GetToplevelVarInst>(inst) ||
              llvh::isa<GetToplevelClosureVarInst>(inst))
            continue;

          builder->SetInsertionPointAfter(inst);
          ///// insert setToplevelVar or setToplevelClosureVar
          ProcessNonMovInstForSpecialAttribute(ir_ctx, inst);
        }
      }
    }
  }

  return true;
}

Pass* CreateProcessSpecialMovPass(IRContext* ir_ctx) {
  return new ProcessSpecialMovPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
