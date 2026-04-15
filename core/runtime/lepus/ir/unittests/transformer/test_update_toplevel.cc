// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/transformer/mir/dce.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"
#include "core/runtime/lepus/ir/transformer/vm/toplevel_store_optimization.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestUpdateToplevel : public IRTestBase {
 public:
  void SetUp() override {
    vm_ctx = std::make_unique<VMContext>();
    vm_ctx->Initialize();
    ir_ctx = std::make_unique<IRContext>(vm_ctx.get());
    std::unique_ptr<TargetContext> target_ctx =
        std::make_unique<TargetContext>();
    ir_ctx->SetTargetContext(target_ctx);
    mod = ir_ctx->GetMainMod();
  }

  std::unique_ptr<VMContext> vm_ctx;
};

TEST_F(LEPUSIRTestUpdateToplevel, BasicUpdate) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_func";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  // 1. Set up a toplevel variable in VMContext
  // name "a", original reg 10
  vm_ctx->UpdateToplevelVarReg("a", 10);

  // 2. Create an instruction representing 'a'
  auto* v1 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(100),
                                           TypeOp::CreateInt32(&builder));
  v1->SetToplevelVarReg(10);
  ir_ctx->InsertToplevelValue(v1, 10);

  // 3. Run RA
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  // 4. Run UpdateToplevelVarRegPass
  auto* update_pass_raw = CreateUpdateToplevelVarRegPass(ir_ctx.get());
  auto* update_pass = llvh::cast<ModulePass>(update_pass_raw);
  update_pass->RunOnModule(mod);

  // 5. Verify VMContext is updated
  const auto& top_vars = vm_ctx->GetToplevelVariables();
  auto it = top_vars.find("a");
  ASSERT_NE(it, top_vars.end());

  // The new register should be what RA assigned to v1
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  unsigned new_reg = ra->GetRegister(v1).GetIndex();
  EXPECT_EQ(it->second, (long)new_reg);

  delete update_pass_raw;
}

TEST_F(LEPUSIRTestUpdateToplevel, MultiUpdate) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_func";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  // 1. Set up multiple toplevel variables
  vm_ctx->UpdateToplevelVarReg("a", 10);
  vm_ctx->UpdateToplevelVarReg("b", 11);
  vm_ctx->UpdateToplevelVarReg("c", 12);

  auto* v1 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(100),
                                           TypeOp::CreateInt32(&builder));
  v1->SetToplevelVarReg(10);
  ir_ctx->InsertToplevelValue(v1, 10);

  auto* v2 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(200),
                                           TypeOp::CreateInt32(&builder));
  v2->SetToplevelVarReg(11);
  ir_ctx->InsertToplevelValue(v2, 11);

  auto* v3 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(300),
                                           TypeOp::CreateInt32(&builder));
  v3->SetToplevelVarReg(12);
  ir_ctx->InsertToplevelValue(v3, 12);

  // 2. Run RA
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  // 3. Run UpdateToplevelVarRegPass
  auto* update_pass_raw = CreateUpdateToplevelVarRegPass(ir_ctx.get());
  auto* update_pass = llvh::cast<ModulePass>(update_pass_raw);
  update_pass->RunOnModule(mod);

  // 4. Verify all are updated
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);

  EXPECT_EQ(vm_ctx->GetToplevelVariables().at("a"),
            (long)ra->GetRegister(v1).GetIndex());
  EXPECT_EQ(vm_ctx->GetToplevelVariables().at("b"),
            (long)ra->GetRegister(v2).GetIndex());
  EXPECT_EQ(vm_ctx->GetToplevelVariables().at("c"),
            (long)ra->GetRegister(v3).GetIndex());

  delete update_pass_raw;
}

TEST_F(LEPUSIRTestUpdateToplevel, ProtectionDCE) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_func";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  // Create a toplevel variable instruction with no users
  auto* v1 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(100),
                                           TypeOp::CreateInt32(&builder));
  v1->SetToplevelVarReg(0);

  // Run DCE
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  // Verify v1 is NOT deleted
  bool found = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == v1) found = true;
  }
  EXPECT_TRUE(found);
}

TEST_F(LEPUSIRTestUpdateToplevel, UpdateToplevelVarPropagation) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_func";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  // 1. Create original instruction for toplevel var "a" (reg 10)
  auto* v1 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(100),
                                           TypeOp::CreateInt32(&builder));
  v1->SetToplevelVarReg(10);
  ir_ctx->InsertToplevelValue(v1, 10);

  // 2. Create new instruction that will replace v1
  auto* v2 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(200),
                                           TypeOp::CreateInt32(&builder));

  // 3. Update toplevel var: replace v1 with v2
  ir_ctx->UpdateToplevelVar(v1, v2);

  // 4. Verify v2 has the attribute and is in the map
  EXPECT_EQ(v2->GetToplevelVarReg(), 10);

  const auto& toplevel_vars = ir_ctx->GetToplevelVariables();
  auto it = toplevel_vars.find(10);
  ASSERT_NE(it, toplevel_vars.end());
  EXPECT_EQ(it->second, v2);
}

TEST_F(LEPUSIRTestUpdateToplevel,
       UpdateAfterRedundantSetEliminatesToplevelAnchor) {
  // This test protects a subtle integration hazard:
  // - IRContext::toplevel_variables_ may (incorrectly but practically) point to
  //   a SetToplevelVarInst (store) as the toplevel anchor value.
  // - the set-toplevel-elimination pass can delete that Set.
  // If we don't rewrite the side-table to the producer, later passes that
  // consult toplevel_variables_ and call ra->GetRegister(value) may crash.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_toplevel_anchor_set_elimination";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);
  Block* entry = &*func->begin();

  // Clear default return.
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder.SetInsertionPointToStart(entry);

  // VMContext toplevel variable "a" lives in original reg 10.
  vm_ctx->UpdateToplevelVarReg("a", 10);

  // Create a producer and a SetToplevelVarInst.
  auto* src = builder.Create<LoadConstInst>(1, builder.GetLiteralInt32(123),
                                            TypeOp::CreateInt32(&builder));
  auto* set_inst =
      builder.Create<SetToplevelVarInst>(2, builder.GetLiteralUint32(10), src);

  // (Intentionally) treat the Set as the toplevel anchor in IRContext.
  set_inst->SetToplevelVarReg(10);
  ir_ctx->InsertToplevelValue(set_inst, 10);
  builder.Create<ReturnInst>(3, builder.GetLiteralInt32(0));

  // Setup and run RA.
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);

  // Run UpdateToplevelVarRegPass first (as in pipeline).
  long vm_ctx_reg_after_update = -1;
  {
    auto* update_pass_raw = CreateUpdateToplevelVarRegPass(ir_ctx.get());
    auto* update_pass = llvh::cast<ModulePass>(update_pass_raw);
    update_pass->RunOnModule(mod);
    auto it = vm_ctx->GetToplevelVariables().find("a");
    ASSERT_NE(it, vm_ctx->GetToplevelVariables().end());
    vm_ctx_reg_after_update = it->second;
    delete update_pass_raw;
  }

  // Now run redundant Set elimination.
  {
    ToplevelStoreOptimizationPass pass(ir_ctx.get());
    pass.RunOnModule(mod);
  }

  // Verify: the IRContext side-table anchor is rewritten to the producer.
  auto& toplevel_vars = ir_ctx->GetToplevelVariables();
  ASSERT_NE(toplevel_vars.find(10), toplevel_vars.end());
  EXPECT_EQ(toplevel_vars[10], src);
  EXPECT_TRUE(ra->IsAllocated(toplevel_vars[10]));

  // Verify: Set is deleted.
  bool set_exists = false;
  for (auto& op : *entry) {
    if (&op == set_inst) set_exists = true;
  }
  EXPECT_FALSE(set_exists);

  // VMContext mapping should stay consistent after elimination.
  {
    auto it = vm_ctx->GetToplevelVariables().find("a");
    ASSERT_NE(it, vm_ctx->GetToplevelVariables().end());
    EXPECT_EQ(it->second, vm_ctx_reg_after_update);
    EXPECT_EQ(it->second, (long)ra->GetRegister(src).GetIndex());
  }
}

TEST_F(LEPUSIRTestUpdateToplevel,
       RedundantSetEliminationIgnoresCurrentRegValue) {
  // Regression test:
  // The IRContext toplevel anchor is inserted only once during bytecode->IR
  // building. Later SetToplevelVarInst may define new values in that same
  // toplevel register, but IRContext::toplevel_variables_ may still point to
  // the initial anchor.
  //
  // The set-toplevel-elimination pass should ignore the value that is
  // actually live in the target register at the store point (the overwritten
  // value), not just the initial anchor.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_ignore_overwritten";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);

  Block* entry = &*func->begin();
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder.SetInsertionPointToStart(entry);

  // Anchor toplevel reg 0.
  auto* anchor0 = builder.Create<LoadConstInst>(1, builder.GetLiteralInt32(0),
                                                TypeOp::CreateInt32(&builder));
  ir_ctx->InsertToplevelValue(anchor0, 0);

  // First store defines a new value in toplevel reg 0 but is NOT eligible for
  // coalescing (src has multiple uses).
  auto* src1 = builder.Create<LoadConstInst>(2, builder.GetLiteralInt32(1),
                                             TypeOp::CreateInt32(&builder));
  [[maybe_unused]] auto* keep_src1_live = builder.Create<MovInst>(3, src1);
  [[maybe_unused]] auto* set1 =
      builder.Create<SetToplevelVarInst>(4, builder.GetLiteralUint32(0), src1);

  // Second store should be coalesced: src2 has exactly one use (the Set).
  auto* src2 = builder.Create<LoadConstInst>(5, builder.GetLiteralInt32(2),
                                             TypeOp::CreateInt32(&builder));
  auto* set2 =
      builder.Create<SetToplevelVarInst>(6, builder.GetLiteralUint32(0), src2);
  builder.Create<ReturnInst>(7, builder.GetLiteralInt32(0));

  // Setup and run RA.
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);

  // Run redundant Set elimination.
  {
    ToplevelStoreOptimizationPass pass(ir_ctx.get());
    pass.RunOnModule(mod);
  }

  // Verify: set2 is deleted and src2 is forced into toplevel reg 0.
  bool set2_exists = false;
  for (auto& op : *entry) {
    if (&op == set2) set2_exists = true;
  }
  EXPECT_FALSE(set2_exists);
  EXPECT_TRUE(src2->IsFixReg());
  EXPECT_TRUE(ra->IsAllocated(src2));
  EXPECT_EQ(ra->GetRegister(src2).GetIndex(), 0u);
}

TEST_F(LEPUSIRTestUpdateToplevel,
       RedundantSetEliminationMultiUseWhenAlreadyInTargetReg) {
  // New behavior:
  // If `src` is already allocated to the target physical register, the
  // SetToplevelVarInst will lower to a redundant Move and can be removed even
  // when `src` has multiple users (and even if it is a fix-reg toplevel
  // variable).

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_redundant_multi_use";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);

  Block* entry = &*func->begin();
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder.SetInsertionPointToStart(entry);

  // Make `src` a toplevel variable anchor at original reg 0 so RA preallocates
  // it to physical reg 0 and marks it fix-reg.
  auto* src = builder.Create<LoadConstInst>(1, builder.GetLiteralInt32(7),
                                            TypeOp::CreateInt32(&builder));
  ir_ctx->InsertToplevelValue(src, 0);

  // Add another user to ensure src has multiple users.
  [[maybe_unused]] auto* extra_use = builder.Create<MovInst>(2, src);

  auto* set_inst =
      builder.Create<SetToplevelVarInst>(3, builder.GetLiteralUint32(0), src);
  builder.Create<ReturnInst>(4, builder.GetLiteralInt32(0));

  // Run RA.
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);

  // Sanity: src is fix-reg in reg 0.
  ASSERT_TRUE(ra->IsAllocated(src));
  EXPECT_TRUE(src->IsFixReg());
  EXPECT_EQ(ra->GetRegister(src).GetIndex(), 0u);

  // Run redundant Set elimination.
  {
    ToplevelStoreOptimizationPass pass(ir_ctx.get());
    pass.RunOnModule(mod);
  }

  // Verify: the Set is removed.
  bool set_exists = false;
  for (auto& op : *entry) {
    if (&op == set_inst) set_exists = true;
  }
  EXPECT_FALSE(set_exists);
}

TEST_F(LEPUSIRTestUpdateToplevel,
       RedundantSetEliminationDoesNotCoalesceWhenOperandInTargetReg) {
  // Safety test:
  // TryCoalesceProducerIntoTargetReg contains a conservative check: if any
  // operand of
  // the producer already occupies the target physical register, we do NOT
  // Coalesce the producer into that register.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_operand_in_target_reg";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);

  Block* entry = &*func->begin();
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder.SetInsertionPointToStart(entry);

  // Force `op0` to be fixed in physical reg 0 by making it a toplevel anchor.
  auto* op0 = builder.Create<LoadConstInst>(1, builder.GetLiteralInt32(11),
                                            TypeOp::CreateInt32(&builder));
  ir_ctx->InsertToplevelValue(op0, 0);

  auto* one = builder.Create<LoadConstInst>(2, builder.GetLiteralInt32(1),
                                            TypeOp::CreateInt32(&builder));
  auto* add = builder.Create<BinaryOperatorInst>(
      3, op0, one, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(&builder));
  auto* set_inst =
      builder.Create<SetToplevelVarInst>(4, builder.GetLiteralUint32(0), add);
  builder.Create<ReturnInst>(5, builder.GetLiteralInt32(0));

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(op0));
  EXPECT_EQ(ra->GetRegister(op0).GetIndex(), 0u);

  // Run redundant Set elimination.
  {
    ToplevelStoreOptimizationPass pass(ir_ctx.get());
    pass.RunOnModule(mod);
  }

  // Verify: the Set is NOT removed.
  bool set_exists = false;
  for (auto& op : *entry) {
    if (&op == set_inst) set_exists = true;
  }
  EXPECT_TRUE(set_exists);
  EXPECT_FALSE(add->IsFixReg());
}

TEST_F(LEPUSIRTestUpdateToplevel,
       RedundantSetEliminationDoesNotEliminatePhiSource) {
  // Phi is explicitly excluded from both the "already-in-target-register"
  // elimination path and the producer-coalescing path.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_phi_source";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);

  Block* entry = &*func->begin();
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }

  Block* then_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* else_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* merge_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry
  builder.SetInsertionPointToStart(entry);
  auto* v_then = builder.Create<LoadConstInst>(1, builder.GetLiteralInt32(1),
                                               TypeOp::CreateInt32(&builder));
  auto* v_else = builder.Create<LoadConstInst>(2, builder.GetLiteralInt32(2),
                                               TypeOp::CreateInt32(&builder));
  builder.Create<CondBranchInst>(3, builder.GetLiteralBool(true), then_bb,
                                 else_bb);

  // then_bb
  builder.SetInsertionPointToStart(then_bb);
  builder.Create<BranchInst>(4, merge_bb);

  // else_bb
  builder.SetInsertionPointToStart(else_bb);
  builder.Create<BranchInst>(5, merge_bb);

  // merge_bb
  builder.SetInsertionPointToStart(merge_bb);
  PhiInst::ValueListType phi_vals = {v_then, v_else};
  PhiInst::BlockListType phi_blks = {then_bb, else_bb};
  auto* phi = builder.Create<PhiInst>(6, phi_vals, phi_blks);
  phi->SetType(TypeOp::CreateInt32(&builder));
  auto* set_inst =
      builder.Create<SetToplevelVarInst>(7, builder.GetLiteralUint32(0), phi);
  builder.Create<ReturnInst>(8, builder.GetLiteralInt32(0));

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  // Run redundant Set elimination.
  {
    ToplevelStoreOptimizationPass pass(ir_ctx.get());
    pass.RunOnModule(mod);
  }

  // Verify: the Set is NOT removed.
  bool set_exists = false;
  for (auto& op : *merge_bb) {
    if (&op == set_inst) set_exists = true;
  }
  EXPECT_TRUE(set_exists);
}

TEST_F(LEPUSIRTestUpdateToplevel,
       RedundantSetEliminationClosureSingleUseProducer) {
  // Coverage test for SetToplevelClosureVarInst path:
  // If `src` has a single use (the SetToplevelClosureVarInst) and is safe to
  // move, it should be forced into the same physical register chosen for the
  // closure anchor value.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_closure_single_use";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);

  Block* entry = &*func->begin();
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder.SetInsertionPointToStart(entry);

  // Create a closure anchor value and record it.
  constexpr uint32_t kOldClosureReg = 20;
  auto* anchor_src = builder.Create<LoadConstInst>(
      1, builder.GetLiteralInt32(0), TypeOp::CreateInt32(&builder));
  auto* closure_anchor = builder.Create<MovInst>(2, anchor_src);
  closure_anchor->SetClosureVarReg(kOldClosureReg);
  func->RecordClosureVarRegAndValue(kOldClosureReg, closure_anchor);

  // Single-use producer stored into the closure variable.
  auto* src = builder.Create<LoadConstInst>(3, builder.GetLiteralInt32(42),
                                            TypeOp::CreateInt32(&builder));
  auto* set_closure = builder.Create<SetToplevelClosureVarInst>(
      4, builder.GetLiteralUint32(kOldClosureReg), src);
  builder.Create<ReturnInst>(5, closure_anchor);

  // Run RA.
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(closure_anchor));
  unsigned target_reg = ra->GetRegister(closure_anchor).GetIndex();

  // Run redundant Set elimination.
  {
    ToplevelStoreOptimizationPass pass(ir_ctx.get());
    pass.RunOnModule(mod);
  }

  // Verify: the Set is removed and src is forced into target_reg.
  bool set_exists = false;
  for (auto& op : *entry) {
    if (&op == set_closure) set_exists = true;
  }
  EXPECT_FALSE(set_exists);
  EXPECT_TRUE(src->IsFixReg());
  EXPECT_TRUE(ra->IsAllocated(src));
  EXPECT_EQ(ra->GetRegister(src).GetIndex(), target_reg);
}

TEST_F(LEPUSIRTestUpdateToplevel, ReplaceToplevelVarWithSyncInsts) {
  auto* builder = ir_ctx->GetOpBuilder();
  builder->SetModuleOp(mod);

  std::string name = "test_func";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  func->SetTopLevelFunction();
  Block* entry = &*func->begin();
  // Remove default return created by CreateTestFuncOp
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder->SetInsertionPointToStart(entry);

  // 1. Create a MovInst with ToplevelVarReg
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  auto* mov_inst = builder->Create<MovInst>(0, src);
  mov_inst->SetToplevelVarReg(10);
  ir_ctx->InsertToplevelValue(mov_inst, 10);

  // Use the mov_inst so it's not removed by preliminary cleanup
  builder->Create<ReturnInst>(0, mov_inst);

  // 2. Run ProcessSpecialMovPass
  auto* replace_pass_raw = CreateProcessSpecialMovPass(ir_ctx.get());
  auto* replace_pass = llvh::cast<FunctionPass>(replace_pass_raw);
  replace_pass->RunOnFunction(func);

  // 3. Verify MovInst is replaced by SetToplevelVarInst and GetToplevelVarInst
  bool found_mov = false;
  SetToplevelVarInst* found_set = nullptr;
  GetToplevelVarInst* found_get = nullptr;

  for (auto* inst : entry->InstRange()) {
    if (inst == mov_inst) found_mov = true;
    if (auto* s = llvh::dyn_cast<SetToplevelVarInst>(inst)) found_set = s;
    if (auto* g = llvh::dyn_cast<GetToplevelVarInst>(inst)) found_get = g;
  }

  EXPECT_FALSE(found_mov);
  ASSERT_NE(found_set, nullptr);
  ASSERT_NE(found_get, nullptr);

  // Verify SetToplevelVarInst properties
  EXPECT_EQ(llvh::cast<LiteralUint32>(found_set->GetToplevelReg())->GetValue(),
            10u);
  EXPECT_EQ(found_set->GetSrc(), src);

  // Verify GetToplevelVarInst properties
  EXPECT_EQ(llvh::cast<LiteralUint32>(found_get->GetToplevelReg())->GetValue(),
            10u);

  // Verify uses are updated
  auto* ret = llvh::cast<ReturnInst>(entry->GetTerminator());
  EXPECT_EQ(ret->GetValue(), found_get);

  // Verify IRContext map is updated
  const auto& toplevel_vars = ir_ctx->GetToplevelVariables();
  auto it = toplevel_vars.find(10);
  ASSERT_NE(it, toplevel_vars.end());
  EXPECT_EQ(it->second, found_set);

  delete replace_pass_raw;
}

TEST_F(LEPUSIRTestUpdateToplevel, UpdateToplevelVarInLoop) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_loop";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  func->SetTopLevelFunction();

  Block* entry = &*func->begin();
  Block* loop_header =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* loop_body =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* exit =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry
  builder.SetInsertionPointToStart(entry);
  auto* zero = builder.GetLiteralInt32(0);
  builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(10), zero);
  auto* get_initial = builder.Create<GetToplevelVarInst>(
      0, builder.GetLiteralUint32(10), TypeOp::CreateAnyType(&builder));
  ir_ctx->InsertToplevelValue(get_initial, 10);
  builder.Create<BranchInst>(0, loop_header);

  // loop_header
  builder.SetInsertionPointToStart(loop_header);
  PhiInst::ValueListType phi_vals = {
      get_initial};  // simplified, just one incoming for now
  PhiInst::BlockListType phi_blks = {entry};
  auto* phi = builder.Create<PhiInst>(0, phi_vals, phi_blks);
  phi->SetType(TypeOp::CreateInt32(&builder));
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), loop_body,
                                 exit);

  // loop_body
  builder.SetInsertionPointToStart(loop_body);
  auto* one = builder.GetLiteralInt32(1);
  auto* inc = builder.Create<BinaryOperatorInst>(
      0, phi, one, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(&builder));
  builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(10), inc);
  auto* get_loop = builder.Create<GetToplevelVarInst>(
      0, builder.GetLiteralUint32(10), TypeOp::CreateAnyType(&builder));
  ir_ctx->UpdateToplevelVar(get_initial,
                            get_loop);  // Update the toplevel value mapping
  phi->AddEntry(get_loop, loop_body);
  builder.Create<BranchInst>(0, loop_header);

  // exit
  builder.SetInsertionPointToStart(exit);
  builder.Create<ReturnInst>(0, phi);

  // 1. Run RA
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  // 2. Set up VMContext
  vm_ctx->UpdateToplevelVarReg("a", 10);

  // 3. Run UpdateToplevelVarRegPass
  auto* update_pass_raw = CreateUpdateToplevelVarRegPass(ir_ctx.get());
  auto* update_pass = llvh::cast<ModulePass>(update_pass_raw);
  update_pass->RunOnModule(mod);

  // 4. Verify VMContext is updated to the register of get_loop
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  unsigned expected_reg = ra->GetRegister(get_loop).GetIndex();
  EXPECT_EQ(vm_ctx->GetToplevelVariables().at("a"), (long)expected_reg);

  delete update_pass_raw;
}

TEST_F(LEPUSIRTestUpdateToplevel, SSAIRVerifyAfterLowering) {
  auto* builder = ir_ctx->GetOpBuilder();
  builder->SetModuleOp(mod);

  std::string name = "test_func";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  func->SetTopLevelFunction();
  Block* entry = &*func->begin();
  // Remove default return created by CreateTestFuncOp
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder->SetInsertionPointToStart(entry);

  // 1. Create a MovInst with ToplevelVarReg
  auto* src = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  auto* mov_inst = builder->Create<MovInst>(1, src);
  mov_inst->SetToplevelVarReg(10);
  ir_ctx->InsertToplevelValue(mov_inst, 10);
  builder->Create<ReturnInst>(1, mov_inst);

  // 2. Run ProcessSpecialMovPass
  auto* replace_pass_raw = CreateProcessSpecialMovPass(ir_ctx.get());
  auto* replace_pass = llvh::cast<FunctionPass>(replace_pass_raw);
  replace_pass->RunOnFunction(func);

  // 3. Run GetToplevelRelatedInstEliminationPass
  auto* remove_pass_raw =
      CreateGetToplevelRelatedInstEliminationPass(ir_ctx.get());
  auto* remove_pass = llvh::cast<ModulePass>(remove_pass_raw);
  remove_pass->RunOnModule(mod);

  // 4. Run SSAIRVerifyPass - should PASS
  auto* verify_pass_raw = CreateSSAIRVerifyPass(ir_ctx.get());
  auto* verify_pass = llvh::cast<ModulePass>(verify_pass_raw);
  EXPECT_TRUE(verify_pass->RunOnModule(mod));

  delete replace_pass_raw;
  delete remove_pass_raw;
  delete verify_pass_raw;
}

TEST_F(LEPUSIRTestUpdateToplevel, ClosureOldRegLowering) {
  auto* builder = ir_ctx->GetOpBuilder();
  builder->SetModuleOp(mod);

  std::string name = "test_func";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  mod->SetRootFunction(func);
  func->SetTopLevelFunction();
  Block* entry = &*func->begin();
  // Remove default return created by CreateTestFuncOp
  while (!entry->empty()) {
    entry->Erase(entry->Front());
  }
  builder->SetInsertionPointToStart(entry);

  // 1. Create a MovInst with ClosureVarReg
  auto* src = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  auto* mov_inst = builder->Create<MovInst>(1, src);
  mov_inst->SetClosureVarReg(20);
  func->RecordClosureVarRegAndValue(20, mov_inst);
  builder->Create<ReturnInst>(1, mov_inst);

  // 2. Run ProcessSpecialMovPass
  auto* replace_pass_raw = CreateProcessSpecialMovPass(ir_ctx.get());
  auto* replace_pass = llvh::cast<FunctionPass>(replace_pass_raw);
  replace_pass->RunOnFunction(func);

  // 3. Verify mapping is updated to GetToplevelClosureVarInst
  auto* closure_val = func->GetClosureVarGivenReg(20);
  ASSERT_NE(closure_val, nullptr);
  EXPECT_TRUE(llvh::isa<GetToplevelClosureVarInst>(closure_val));
  EXPECT_EQ(closure_val->GetClosureVarReg(), 20);

  // 4. Run SSAIRVerifyPass - should PASS
  // Need to run GetToplevelRelatedInstEliminationPass first to clear Get's
  // attribute
  auto* remove_pass_raw =
      CreateGetToplevelRelatedInstEliminationPass(ir_ctx.get());
  auto* remove_pass = llvh::cast<ModulePass>(remove_pass_raw);
  remove_pass->RunOnModule(mod);

  auto* verify_pass_raw = CreateSSAIRVerifyPass(ir_ctx.get());
  auto* verify_pass = llvh::cast<ModulePass>(verify_pass_raw);
  EXPECT_TRUE(verify_pass->RunOnModule(mod));

  delete replace_pass_raw;
  delete remove_pass_raw;
  delete verify_pass_raw;
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
