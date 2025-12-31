// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/ssa_ir_verify.h"

#include <set>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {
bool SSAIRVerifyPass::RunOnModule(ModuleOp* mod) {
  // verify toplevel variables
  const auto& toplevel_vars = ir_ctx_->GetToplevelVariables();
  for (const auto& item : toplevel_vars) {
    if (item.second->GetToplevelVarReg() != item.first) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: SSAIRVerifyPass detected toplevel variable reg "
          "mismatch");
    }
  }

  // verify each function
  for (auto* func : *mod) {
    for (auto& bb : *func) {
      // each bb should have terminatorInst
      if (!bb.HasTerminalInst()) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: SSAIRVerifyPass detected basic block without "
            "terminator");
      }

      for (auto& inst : bb) {
        // each bb should have only one terminatorInst
        if (llvh::isa<TerminatorInst>(&inst) && bb.GetTerminator() != &inst) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: SSAIRVerifyPass detected multiple terminators "
              "in a basic block");
        }

        // should not have movInst
        if (llvh::isa<MovInst>(&inst)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: SSAIRVerifyPass detected MovInst in SSA IR");
        }

        if (auto* special_inst = llvh::dyn_cast<Instruction>(&inst)) {
          // if inst has toplevelVarReg and closureVarReg, these two attribute
          // should be the same.
          if (special_inst->GetToplevelVarReg() !=
                  constants::kInvalidSignedValue &&
              special_inst->GetClosureVarReg() !=
                  constants::kInvalidSignedValue &&
              special_inst->GetClosureVarReg() !=
                  special_inst->GetToplevelVarReg()) {
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: SSAIRVerifyPass detected mismatched closure "
                "var reg and toplevel var reg");
          }

          // only SetToplevelVarInst or SetToplevelClosureVarInst should have
          // toplevelVarReg or closureVarReg attribute
          if (special_inst->GetToplevelVarReg() !=
                  constants::kInvalidSignedValue ||
              special_inst->GetClosureVarReg() !=
                  constants::kInvalidSignedValue) {
            if (!(llvh::isa<SetToplevelVarInst>(special_inst)) &&
                !llvh::isa<SetToplevelClosureVarInst>(special_inst)) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: SSAIRVerifyPass detected illegal special "
                  "attributes on non-toplevel-var instruction");
            }
          }
        }
      }
    }

    std::set<Value*> defines;
    PostOrderAnalysis PO(func);
    llvh::SmallVector<Block*, 16> order(PO.rbegin(), PO.rend());

    for (int i = 0, e = order.size(); i < e; ++i) {
      Block* bb = order[i];
      for (auto& op : *bb) {
        if (auto* inst = llvh::dyn_cast<Instruction>(&op)) {
          // verify line and column number
          if (inst->GetLocation() == 0) {
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: SSAIRVerifyPass detected instruction with "
                "invalid location");
          }

          // verify ir use after define
          for (auto use : inst->GetUsers()) {
            if (defines.count(use) && !llvh::isa<PhiInst>(use)) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: SSAIRVerifyPass detected use-before-define "
                  "(user already defined)");
            }
          }

          for (size_t op_idx = 0; op_idx < inst->GetNumOperands(); op_idx++) {
            if (llvh::isa<PhiInst>(inst)) {
              break;
            }

            auto operand = inst->GetOperand(op_idx);
            if (operand) {
              if (llvh::isa<Instruction>(operand) && !defines.count(operand)) {
                throw ::lynx::lepus::CompileException(
                    "Lepus IR error: SSAIRVerifyPass detected "
                    "use-before-define (operand not defined yet)");
              }
            }
          }
          defines.insert(inst);

          // each instruction should have type
          if (inst->HasOutput() && inst->GetType()->IsInvalidType()) {
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: SSAIRVerifyPass detected instruction with "
                "NoType");
          }

          // loadconst instruction should not have any type
          if (llvh::isa<LoadConstInst>(inst)) {
            if (inst->GetType()->IsAnyType()) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: SSAIRVerifyPass detected LoadConstInst with "
                  "AnyType");
            }
          }

          for (size_t op_idx = 0; op_idx < inst->GetNumOperands(); op_idx++) {
            auto operand = inst->GetOperand(op_idx);
            if (llvh::isa<Instruction>(operand) &&
                operand->GetType()->IsInvalidType()) {
              // instruction should not have no type
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: SSAIRVerifyPass detected operand "
                  "instruction with NoType");
            }
          }
        }
      }
    }
  }

  return true;
}

Pass* CreateSSAIRVerifyPass(IRContext* ir_ctx) {
  return new SSAIRVerifyPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
