// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/transformer/mir/normalize_phi.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestNormalizePhi : public IRTestBase {
 public:
  virtual void SetUp(void) { IRTestBase::SetUp(); }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestNormalizePhi, EliminateTrivialPhi) {
  // Case: %phi = Phi(%val, block1, %val, block2)
  // Expected: %phi is replaced by %val and removed.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_eliminate_trivial_phi";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);

  Block* entry = &*func->begin();
  Block* bb1 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb2 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* target =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* val = builder.GetLiteralInt32(42);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), bb1, bb2);

  builder.SetInsertionPointToStart(bb1);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(bb2);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(target);
  PhiInst::ValueListType vals = {val, val};
  PhiInst::BlockListType blks = {bb1, bb2};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  phi->SetType(TypeOp::CreateInt32(&builder));

  auto* ret = builder.Create<ReturnInst>(0, phi);

  NormalizePhiPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // 1. Phi should be removed
  bool found_phi = false;
  for (auto& inst : *target) {
    if (&inst == phi) {
      found_phi = true;
      break;
    }
  }
  EXPECT_FALSE(found_phi);

  // 2. ReturnInst should use 'val' instead of 'phi'
  EXPECT_EQ(ret->GetValue(), val);
}

TEST_F(LEPUSIRTestNormalizePhi, EliminateSingleEntryPhi) {
  // Case: %phi = Phi(%val, block1)
  // Expected: %phi is replaced by %val and removed.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_eliminate_single_entry_phi";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);

  Block* entry = &*func->begin();
  Block* target =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* val = builder.GetLiteralInt32(100);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(target);
  PhiInst::ValueListType vals = {val};
  PhiInst::BlockListType blks = {entry};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  phi->SetType(TypeOp::CreateInt32(&builder));

  auto* ret = builder.Create<ReturnInst>(0, phi);

  NormalizePhiPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_phi = false;
  for (auto& inst : *target) {
    if (&inst == phi) {
      found_phi = true;
      break;
    }
  }
  EXPECT_FALSE(found_phi);
  EXPECT_EQ(ret->GetValue(), val);
}

TEST_F(LEPUSIRTestNormalizePhi, EliminateSelfReferencingTrivialPhi) {
  // Case: %phi = Phi(%val, block1, %phi, block2)
  // Expected: %phi is replaced by %val and removed.

  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test_eliminate_self_ref_phi";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);

  Block* entry = &*func->begin();
  Block* bb1 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* loop =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* exit =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* val = builder.GetLiteralInt32(99);
  builder.Create<BranchInst>(0, bb1);

  builder.SetInsertionPointToStart(bb1);
  builder.Create<BranchInst>(0, loop);

  builder.SetInsertionPointToStart(loop);
  // Create Phi first
  PhiInst::ValueListType empty_vals;
  PhiInst::BlockListType empty_blks;
  auto* phi = builder.Create<PhiInst>(0, empty_vals, empty_blks);
  phi->SetType(TypeOp::CreateInt32(&builder));

  phi->AddEntry(val, bb1);
  phi->AddEntry(phi, loop);  // Self reference

  // Use phi
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), loop, exit);

  builder.SetInsertionPointToStart(exit);
  auto* ret = builder.Create<ReturnInst>(0, phi);

  NormalizePhiPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_phi = false;
  for (auto& inst : *loop) {
    if (&inst == phi) {
      found_phi = true;
      break;
    }
  }
  EXPECT_FALSE(found_phi);
  EXPECT_EQ(ret->GetValue(), val);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
