// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"
#include "core/runtime/lepus/ir/target_context.h"
#include "core/runtime/lepus/ir/transformer/mir/change_special_attribute.h"
#include "core/runtime/lepus/ir/transformer/mir/construct_ssa_ir.h"
#include "core/runtime/lepus/ir/transformer/mir/get_toplevel_related_inst_elimination.h"

namespace lynx {
namespace lepus {
namespace ir {

//===----------------------------------------------------------------------===//
// Pipeline contract / IR invariants (structured)
//
// Purpose
//   - Document stage invariants (Requires / Guarantees).
//   - Make pass ordering and dependencies explicit.
//   - Clarify which passes depend on and/or mutate toplevel/closure
//   side-tables.
//
// Key side-tables (out-of-band state consumed by later passes / lowering)
//   - IRContext::toplevel_variables_
//       Maps:  toplevel slot (logical reg) -> anchor(Value/Instruction)
//       Used by: UpdateToplevelVarRegPass, SetToplevelEliminationPass, ISel
//
//   - FuncOp(root)::closure var side-table
//       Maps:  closure_var_reg -> anchor(Value/Instruction)
//       Used by: UpdateToplevelClosureVarPass, SetToplevelEliminationPass
//
//   - FuncOp(root)::toplevel_closure_var_regs_
//       Data:  set of closure regs that must be exposed to descendants
//       Produced by: CollectToplevelClosureRegPass
//
//   - FuncOp::upvalue_index_to_toplevel_reg_
//       Data:  mapping (upvalue index -> toplevel physical reg)
//       Produced by: UpdateToplevelClosureVarPass
//       Used by: instruction selection, FunctionToBuiltInPass
//
// Global rule
//   - After side-table synchronization, any pass that changes physical
//     registers or replaces anchor instructions MUST update corresponding
//     side-tables, otherwise later lowering may read stale anchors / regs.
//
// Stage overview (order is significant and matches AddMIRPasses /
// AddTargetPasses below)
//   Stage A (SM_MIR): side-table precompute
//     Pass: CollectToplevelClosureRegPass
//       Requires: root function + LepusFunction child/upvalue metadata
//       Updates:  FuncOp(root)::toplevel_closure_var_regs_
//       Guarantees: closure regs set is complete for descendants
//
//   Stage B (SM_MIR): SSA construction + normalization
//     Passes: ConstructSSAIRPass -> NormalizePhiPass
//       Requires: MIR is well-formed (CFG traversable; def-use closed)
//       Guarantees: SSA built; phi normalized for downstream passes
//
//   Stage C (SM_MIR): toplevel/closure attribute normalization
//     Passes: ProcessSpecialMovPass -> GetToplevelRelatedInstEliminationPass
//       Requires: SSA form ready
//       Guarantees:
//         - MovInst rewritten into Get/SetToplevel* where applicable
//         - GetToplevel* with no uses may be removed
//         - toplevelVarReg/closureVarReg stripped from GetToplevel*
//           (attributes should remain only on SetToplevel*)
//
//   Stage D (SM_MIR): SSA verification
//     Pass: SSAIRVerifyPass
//       Requires: SSA invariants hold
//       Guarantees: no mutation; diagnostics/assertions only
//
//   Stage E (SM_MIR): attribute cleanup + type info refresh
//     Passes: ChangeSpecialAttributePass -> TypeSpecificationPass ->
//     NormalizePhiPass
//       Notes:
//         - ChangeSpecialAttributePass may clear redundant ClosureVarReg when
//         it
//           matches ToplevelVarReg; UpdateToplevelClosureVarPass tolerates
//           this.
//       Guarantees: type info attached; phi kept consistent with constraints
//
//   Stage F (SM_MIR): generic SSA-based optimizations and late constant
//   materialization
//     Passes (exact order):
//       SimplifyCFG -> LoadNullEliminationPass -> InstCombinePass ->
//       DCE -> CSE -> LoadStoreElimination -> DCE -> LoadStoreElimination ->
//       SimplifyCFG -> InstCombinePass -> DCE -> ConstAggregateTemplatePass ->
//       SimplifyCFG -> DCE
//       Requires: SSA validity; CFG/def-use available
//       Guarantees:
//         - canonical CFG / instruction shapes for downstream RA
//         - redundant loads / stores / dead code removed as much as possible
//         - const aggregate templates are formed only after the main MIR
//           cleanup pipeline settles, and then we run a final SimplifyCFG to
//           canonicalize CFG changes introduced by aggregation
//
//   Stage G (SM_REG_ALLOC): register allocation
//     Passes: RegisterAllocationPass -> VerifyCallRegisterPass ->
//             MovEliminationPass -> RegisterCompactionPass
//       Notes:
//         - VerifyCallRegisterPass after RegisterCompactionPass is currently
//           disabled in this pipeline; the final call-register verification is
//           performed after post-RA toplevel/closure rewrites.
//       Requires: MIR/SSA stable enough for RA; TargetContext available
//       Guarantees: RA analysis valid; values assigned to physical registers
//
//   Stage H (SM_REG_ALLOC): post-RA side-table synchronization and late
//   toplevel/closure lowering
//     Pass: UpdateToplevelVarRegPass
//       Requires: IRContext::toplevel_variables_ + VMContext toplevel mapping
//       Updates:  VMContext reg indices; Get/SetToplevelVar literal regs
//
//     Pass: UpdateToplevelClosureVarPass
//       Requires: closure side-table consistency + RA analysis
//       Updates:  FuncOp::upvalue_index_to_toplevel_reg_ mapping
//
//     Pass: SetToplevelEliminationPass (ToplevelStoreOptimizationPass)
//       Requires: side-tables already synced to physical regs
//       Updates:  may delete/merge SetToplevel*; MUST update anchor side-tables
//
//     Pass: FunctionToBuiltInPass
//       Requires: upvalue_index -> toplevel physical reg mapping ready
//       Requires: anchors/regs stabilized after SetToplevelEliminationPass
//
//     Pass: VerifyCallRegisterPass
//       Requires: post-RA rewrites preserve call register constraints
//       Guarantees: later VM lowering still sees a valid physical-register
//       layout
//
//   Stage I (SM_VM_TARGET): instruction selection
//     Passes: DumpOpcodeBeforeOptPass -> InstructionSelectionPass
//       Requires: side-tables synced; RA stabilized
//       Guarantees: VM target instruction stream is produced
//===----------------------------------------------------------------------===//
static void AddMIRPasses(PassManager& pm) {
  pm.SetMode(StageMode::SM_MIR);
  pm.AddCollectToplevelClosureRegPass();
  pm.AddConstructSSAIRPass();
  pm.AddNormalizePhiPass();

  pm.AddProcessSpecialMovPass();
  pm.AddGetToplevelRelatedInstEliminationPass();

  pm.AddSSAIRVerifyPass();
  pm.AddChangeSpecialAttributePass();
  pm.AddTypeSpecificationPass();
  pm.AddNormalizePhiPass();

  pm.AddSimplifyCFG();
  pm.AddLoadNullEliminationPass();
  pm.AddInstCombinePass();
  pm.AddDCE();
  pm.AddCSE();
  pm.AddLoadStoreElimination();
  pm.AddDCE();
  pm.AddLoadStoreElimination();
  pm.AddSimplifyCFG();
  pm.AddInstCombinePass();
  pm.AddDCE();
  pm.AddConstAggregateTemplatePass();
  pm.AddSimplifyCFG();
  pm.AddDCE();
}

static void AddTargetPasses(PassManager& pm) {
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  pm.GetIRCtx()->SetTargetContext(target_ctx);

  pm.SetMode(StageMode::SM_REG_ALLOC);
  pm.AddRegisterAllocationPass();
  pm.AddVerifyCallRegisterPass();
  pm.AddMovEliminationPass();
  pm.AddRegisterCompactionPass();
  // pm.AddVerifyCallRegisterPass();

  pm.AddUpdateToplevelVarRegPass();
  pm.AddUpdateToplevelClosureVarPass();
  pm.AddSetToplevelEliminationPass();
  pm.AddFunctionToBuiltInPass();
  pm.AddVerifyCallRegisterPass();

  pm.SetMode(StageMode::SM_VM_TARGET);
  pm.AddDumpOpcodeBeforeOptPass();
  pm.AddInstructionSelectionPass();
}

void RunO1OptimizationPasses(ModuleOp& mod, const char* ir_dump_path) {
  PassManager pm(mod.GetIRCtx());
  pm.SetIRDumpPath(ir_dump_path);
  AddMIRPasses(pm);
  AddTargetPasses(pm);

  pm.Run(&mod);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
