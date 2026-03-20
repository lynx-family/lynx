// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/process_special_mov.h"
#include "core/runtime/lepus/ir/transformer/mir/ssa_ir_verify.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestLoadStoreEliminationAttr : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestLoadStoreEliminationAttr, LoadConstAttributeIsolation) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* lit = builder.GetLiteralInt32(42);
  auto* type = TypeOp::CreateAnyType(&builder);

  // 1. Normal LoadConst
  auto* load1 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);

  // 2. LoadConst that will be stored to toplevel 1
  auto* load2 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);
  builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(1), load2);

  // 3. Another normal LoadConst
  auto* load3 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);

  // 4. LoadConst that will be stored to toplevel 2
  auto* load4 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);
  builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(2), load4);

  builder.Create<ReturnInst>(0, load3);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_load1 = false;
  bool found_load2 = false;
  bool found_load3 = false;
  bool found_load4 = false;

  for (auto* inst : entry->InstRange()) {
    if (inst == load1) found_load1 = true;
    if (inst == load2) found_load2 = true;
    if (inst == load3) found_load3 = true;
    if (inst == load4) found_load4 = true;
  }

  EXPECT_TRUE(found_load1);
  EXPECT_FALSE(found_load2) << "Identical LoadConst should be merged even if "
                               "destined for different variables";
  EXPECT_FALSE(found_load3) << "Normal LoadConst should be merged";
  EXPECT_FALSE(found_load4)
      << "All identical LoadConst should be merged into the first one";

  // Verify that load2, load3, load4 uses are replaced with load1
  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  ASSERT_NE(nullptr, ret);
  EXPECT_EQ(ret->GetOperand(0), load1);

  for (auto* inst : entry->InstRange()) {
    if (auto* set = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
      EXPECT_EQ(set->GetSrc(), load1);
    }
  }
}

TEST_F(LEPUSIRTestLoadStoreEliminationAttr, LoadConstAttributeSourceIsolation) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_source";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* lit = builder.GetLiteralInt32(100);
  auto* type = TypeOp::CreateAnyType(&builder);

  // 1. LoadConst producing value for toplevel 1
  auto* load1 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);
  builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(1), load1);

  // 2. Normal LoadConst
  auto* load2 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);

  builder.Create<ReturnInst>(0, load2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_load1 = false;
  bool found_load2 = false;

  for (auto* inst : entry->InstRange()) {
    if (inst == load1) found_load1 = true;
    if (inst == load2) found_load2 = true;
  }

  EXPECT_TRUE(found_load1);
  EXPECT_FALSE(found_load2) << "Normal LoadConst SHOULD be replaced by "
                               "previous identical value producer";

  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  ASSERT_NE(nullptr, ret);
  EXPECT_EQ(ret->GetOperand(0), load1);
}

TEST_F(LEPUSIRTestLoadStoreEliminationAttr,
       LoadConstMergingWithSetToplevelVar) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_merging";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* lit = builder.GetLiteralInt32(42);
  auto* type = TypeOp::CreateInt32(&builder);

  // 1. LoadConst 1, stored to toplevel 10
  auto* load1 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);
  builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(10), load1);

  // 2. LoadConst 2 (identical), stored to toplevel 11
  auto* load2 = builder.Create<LoadConstInst>(0, (Literal*)lit, type);
  builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(11), load2);

  // 3. GetToplevelVar 10
  auto* get1 =
      builder.Create<GetToplevelVarInst>(0, builder.GetLiteralUint32(10), type);

  builder.Create<ReturnInst>(0, get1);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_load1 = false;
  bool found_load2 = false;
  bool found_get1 = false;

  for (auto* inst : entry->InstRange()) {
    if (inst == load1) found_load1 = true;
    if (inst == load2) found_load2 = true;
    if (inst == get1) found_get1 = true;
  }

  EXPECT_TRUE(found_load1);
  EXPECT_FALSE(found_load2) << "Identical LoadConst should be merged even if "
                               "stored to different toplevel variables";
  EXPECT_FALSE(found_get1) << "GetToplevelVar should be eliminated by "
                              "forwarding from SetToplevelVar";

  // Verify that load2 uses were replaced by load1
  for (auto* inst : entry->InstRange()) {
    if (auto* set = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
      EXPECT_EQ(set->GetSrc(), load1);
    }
  }
}

TEST_F(LEPUSIRTestLoadStoreEliminationAttr, SSAIRAttributeInvariant) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_invariant";
  auto* func = builder.Create<FuncOp>(0, name);
  func->SetTopLevelFunction();  // Required for ProcessSpecialMovPass
  mod->SetRootFunction(func);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* lit = builder.GetLiteralInt32(100);
  auto* type = TypeOp::CreateInt32(&builder);

  // 1. Create LoadConstInst with attributes (simulating initial construction
  // hints)
  auto* load1 = builder.Create<LoadConstInst>(1, (Literal*)lit, type);
  ir_ctx->InsertToplevelValue(load1, 5);
  func->RecordClosureVarRegAndValue(5, load1);
  load1->SetClosureVarReg(5);

  // 2. Create another with closureVarReg only
  auto* load2 = builder.Create<LoadConstInst>(1, (Literal*)lit, type);
  func->RecordClosureVarRegAndValue(10, load2);
  load2->SetClosureVarReg(10);

  builder.Create<ReturnInst>(1, load2);

  // 3. Run Lowering Pass
  ProcessSpecialMovPass lowering(ir_ctx.get());
  lowering.RunOnFunction(func);

  // 4. Verify invariants
  bool found_set_toplevel = false;
  bool found_set_closure = false;

  for (auto* inst : entry->InstRange()) {
    if (inst == load1 || inst == load2) {
      EXPECT_EQ(inst->GetToplevelVarReg(), constants::kInvalidSignedValue)
          << "Original producer should have attribute stripped";
      EXPECT_EQ(inst->GetClosureVarReg(), constants::kInvalidSignedValue)
          << "Original producer should have attribute stripped";
    }

    if (auto* set_top = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
      if (set_top->GetToplevelVarReg() == 5) found_set_toplevel = true;
    }

    if (auto* set_closure = llvh::dyn_cast<SetToplevelClosureVarInst>(inst)) {
      if (set_closure->GetClosureVarReg() == 10) found_set_closure = true;
    }
  }

  EXPECT_TRUE(found_set_toplevel);
  EXPECT_TRUE(found_set_closure);

  // 5. Run SSA IR Verify Pass - this should pass without crashing
  SSAIRVerifyPass verifier(ir_ctx.get());
  EXPECT_TRUE(verifier.RunOnModule(mod));
}

TEST_F(LEPUSIRTestLoadStoreEliminationAttr,
       SetToplevelVarMustUpdateCacheEvenWhenFixInst) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_set_toplevel_fix_inst";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* type = TypeOp::CreateInt32(&builder);
  auto* v1 = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralInt32(1), type);
  auto* set1 =
      builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(10), v1);
  // Mark as "fix inst" in LSE (current LSE uses `toplevel_var_reg !=
  // constants::kInvalidSignedValue`).
  set1->SetToplevelVarReg(10);

  auto* v2 = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralInt32(2), type);
  auto* set2 =
      builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(10), v2);
  set2->SetToplevelVarReg(10);

  auto* get =
      builder.Create<GetToplevelVarInst>(0, builder.GetLiteralUint32(10), type);
  // Simulate `GetToplevelRelatedInstEliminationPass` behavior before LSE.
  get->SetToplevelVarReg(constants::kInvalidSignedValue);
  get->SetClosureVarReg(constants::kInvalidSignedValue);
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // If SetToplevelVarInst doesn't update the cache when it is a fix inst,
  // `get` could be forwarded to v1 incorrectly.
  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  ASSERT_NE(nullptr, ret);
  EXPECT_EQ(ret->GetOperand(0), v2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationAttr,
       SetToplevelClosureVarMustUpdateCacheEvenWhenFixInst) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_set_toplevel_closure_fix_inst";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* type = TypeOp::CreateAnyType(&builder);
  auto* v1 = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralInt32(1), type);
  auto* set1 = builder.Create<SetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(7), v1);
  // Mark as "fix inst" in LSE.
  set1->SetToplevelVarReg(7);

  auto* v2 = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralInt32(2), type);
  auto* set2 = builder.Create<SetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(7), v2);
  set2->SetToplevelVarReg(7);

  auto* get = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(7), type);
  get->SetToplevelVarReg(constants::kInvalidSignedValue);
  get->SetClosureVarReg(constants::kInvalidSignedValue);
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  ASSERT_NE(nullptr, ret);
  EXPECT_EQ(ret->GetOperand(0), v2);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
