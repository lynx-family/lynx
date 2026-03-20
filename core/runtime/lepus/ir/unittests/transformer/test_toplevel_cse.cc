// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/transformer/mir/cse.h"
#include "core/runtime/lepus/ir/transformer/mir/get_toplevel_related_inst_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/process_special_mov.h"
#include "core/runtime/lepus/ir/transformer/mir/ssa_ir_verify.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestToplevelCSE : public IRTestBase {
 public:
  virtual void SetUp(void) { IRTestBase::SetUp(); }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestToplevelCSE, ToplevelDecoupledCSE) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "toplevel";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();  // Mark as toplevel

  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  // 1. Load constant 10
  auto* c1 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  // 2. Assign to toplevel reg 0 (using decoupled MovInst)
  auto* m1 = builder.Create<MovInst>(0, c1);
  m1->SetToplevelVarReg(0);
  ir_ctx->InsertToplevelValue(m1, 0);

  // 3. Load constant 10 again
  auto* c2 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  // 4. Assign to toplevel reg 1 (using decoupled MovInst)
  auto* m2 = builder.Create<MovInst>(0, c2);
  m2->SetToplevelVarReg(1);
  ir_ctx->InsertToplevelValue(m2, 1);

  // Before CSE: 2 LoadConst, 2 Mov
  int lc_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadConstInst>(inst)) lc_count++;
  }
  EXPECT_EQ(lc_count, 2);

  // Run CSE
  CSE pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // After CSE: 1 LoadConst, 2 Mov
  lc_count = 0;
  int mov_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadConstInst>(inst)) lc_count++;
    if (llvh::isa<MovInst>(inst)) {
      mov_count++;
      // Verify both moves now use the SAME LoadConst
      EXPECT_EQ(inst->GetOperand(0), c1);
    }
  }
  EXPECT_EQ(lc_count, 1);
  EXPECT_EQ(mov_count, 2);

  // Verify that the moves are NOT merged because they have different
  // toplevelVarReg (They are still in the entry block)
  bool m1_found = false;
  bool m2_found = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == m1) m1_found = true;
    if (inst == m2) m2_found = true;
  }
  EXPECT_TRUE(m1_found);
  EXPECT_TRUE(m2_found);
}

TEST_F(LEPUSIRTestToplevelCSE, ToplevelDecoupledFullPipeline) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "toplevel";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();

  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  auto* c1 = builder.Create<LoadConstInst>(1, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  auto* m1 = builder.Create<MovInst>(1, c1);
  m1->SetToplevelVarReg(0);
  ir_ctx->InsertToplevelValue(m1, 0);

  auto* c2 = builder.Create<LoadConstInst>(1, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  auto* m2 = builder.Create<MovInst>(1, c2);
  m2->SetToplevelVarReg(1);
  ir_ctx->InsertToplevelValue(m2, 1);

  // Add a user for m1 and m2 so that GetToplevelVarInst is not removed
  builder.Create<BinaryOperatorInst>(1, m1, m2, ValueKind::BinaryAddInstKind,
                                     TypeOp::CreateInt32(&builder));

  // 1. Run CSE
  CSE cse_pass(ir_ctx.get());
  cse_pass.RunOnFunction(func);

  // 2. Run ProcessSpecialMovPass
  ProcessSpecialMovPass spec_pass(ir_ctx.get());
  spec_pass.RunOnFunction(func);

  // 3. Run GetToplevelRelatedInstEliminationPass to clear Get's attributes
  GetToplevelRelatedInstEliminationPass remove_pass(ir_ctx.get());
  remove_pass.RunOnModule(mod);

  // 4. Run SSAIRVerifyPass
  SSAIRVerifyPass verify_pass(ir_ctx.get());
  EXPECT_TRUE(verify_pass.RunOnModule(mod));

  // Verify results:
  // - 1 LoadConst
  // - 2 SetToplevelVarInst
  // - 0 GetToplevelVarInst (GetToplevelRelatedInstEliminationPass eliminates
  // redundant Gets)
  int lc_count = 0;
  int set_count = 0;
  int get_count = 0;
  BinaryOperatorInst* found_bin = nullptr;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadConstInst>(inst)) {
      lc_count++;
      // Verify no toplevelVarReg here
      EXPECT_EQ(inst->GetToplevelVarReg(), constants::kInvalidSignedValue);
    }
    if (auto* s = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
      set_count++;
      // SetToplevelVarInst SHOULD have toplevelVarReg
      EXPECT_NE(inst->GetToplevelVarReg(), constants::kInvalidSignedValue);
      // Src should be c1 (due to CSE)
      EXPECT_EQ(s->GetSrc(), c1);
    }
    if (llvh::isa<GetToplevelVarInst>(inst)) {
      get_count++;
      // GetToplevelVarInst SHOULD NOT have toplevelVarReg anymore
      EXPECT_EQ(inst->GetToplevelVarReg(), constants::kInvalidSignedValue);
    }
    if (auto* b = llvh::dyn_cast<BinaryOperatorInst>(inst)) {
      found_bin = b;
    }
  }
  EXPECT_EQ(lc_count, 1);
  EXPECT_EQ(set_count, 2);
  EXPECT_EQ(get_count, 0);
  ASSERT_NE(found_bin, nullptr);
  // BinaryOp should now use c1 directly (Store-to-Load forwarding)
  EXPECT_EQ(found_bin->GetOperand(0), c1);
  EXPECT_EQ(found_bin->GetOperand(1), c1);
}

TEST_F(LEPUSIRTestToplevelCSE, LoadConstWithExplicitSentinelAttrsStillCSE) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "toplevel";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);

  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  auto* c1 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  auto* c2 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));

  // Explicitly set attrs to sentinel and ensure CSE still treats them as unset.
  c1->SetToplevelVarReg(constants::kInvalidSignedValue);
  c1->SetClosureVarReg(constants::kInvalidSignedValue);
  c2->SetToplevelVarReg(constants::kInvalidSignedValue);
  c2->SetClosureVarReg(constants::kInvalidSignedValue);

  builder.Create<ReturnInst>(0, c2);

  CSE pass(ir_ctx.get());
  pass.RunOnFunction(func);

  int lc_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadConstInst>(inst)) lc_count++;
  }
  EXPECT_EQ(lc_count, 1);
}

TEST_F(LEPUSIRTestToplevelCSE, ClosureDecoupledCSE) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "toplevel";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  func->SetTopLevelFunction();  // Mark as toplevel

  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  // 1. Load constant 10
  auto* c1 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  // 2. Mark as captured by closure (original reg 0)
  auto* m1 = builder.Create<MovInst>(0, c1);
  m1->SetClosureVarReg(0);

  // 3. Load constant 10 again
  auto* c2 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  // 4. Mark as captured by closure (original reg 1)
  auto* m2 = builder.Create<MovInst>(0, c2);
  m2->SetClosureVarReg(1);

  // Run CSE
  CSE pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // After CSE: 1 LoadConst, 2 Mov
  int lc_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadConstInst>(inst)) lc_count++;
  }
  EXPECT_EQ(lc_count, 1);

  // Both MovInst remain
  bool m1_found = false;
  bool m2_found = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == m1) m1_found = true;
    if (inst == m2) m2_found = true;
  }
  EXPECT_TRUE(m1_found);
  EXPECT_TRUE(m2_found);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
