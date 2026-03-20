// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instruction.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"
#include "core/runtime/lepus/ir/transformer/mir/simplify_cfg.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestOptIndirectJmp : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestOptIndirectJmp, testOptIndirectJmp) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  auto block = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                       0, "test_block");
  auto true_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                         0, "true_bb");
  auto false_bb = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "false_bb");
  auto phi_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                        0, "phi_bb");
  auto true_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "true_bb1");
  auto false_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                           BlockType::BT_INST, 0, "false_bb1");
  ASSERT_NE(nullptr, block);
  ASSERT_EQ(mod->GetIRRegion(), block->GetParent());
  tmp_builder.SetInsertionPointToEnd(block);

  auto var_1 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  auto var_2 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  ASSERT_NE(nullptr, var_1);
  ASSERT_NE(nullptr, var_2);

  auto* cond = tmp_builder.Create<BinaryOperatorInst>(
      0, var_1, var_2, ValueKind::BinaryStrictlyEqualInstKind,
      TypeOp::CreateBoolean(&tmp_builder));

  auto* test_inst =
      tmp_builder.Create<CondBranchInst>(0, cond, true_bb, false_bb);

  tmp_builder.SetInsertionPointToEnd(true_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(phi_bb);
  auto* phi = tmp_builder.Create<PhiInst>(0, PhiInst::ValueListType(),
                                          PhiInst::BlockListType());

  phi->AddEntry(tmp_builder.GetLiteralBool(true), true_bb);
  phi->AddEntry(tmp_builder.GetLiteralBool(false), false_bb);
  phi->SetType(TypeOp::CreateBoolean(&tmp_builder));

  tmp_builder.Create<CondBranchInst>(0, phi, true_bb1, false_bb1);
  tmp_builder.SetInsertionPointToEnd(true_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0));
  tmp_builder.SetInsertionPointToEnd(false_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0));

  ASSERT_TRUE(OptIndirectJmp(test_inst));
}

TEST_F(LEPUSIRTestOptIndirectJmp, testOptIndirectJmp2) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  auto block = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                       0, "test_block");
  auto true_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                         0, "true_bb");
  auto false_bb = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "false_bb");
  auto phi_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                        0, "phi_bb");
  auto true_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "true_bb1");
  auto false_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                           BlockType::BT_INST, 0, "false_bb1");
  ASSERT_NE(nullptr, block);
  ASSERT_EQ(mod->GetIRRegion(), block->GetParent());
  tmp_builder.SetInsertionPointToEnd(block);

  auto var_1 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  auto var_2 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  ASSERT_NE(nullptr, var_1);
  ASSERT_NE(nullptr, var_2);

  auto* cond = tmp_builder.Create<BinaryOperatorInst>(
      0, var_1, var_2, ValueKind::BinaryStrictlyEqualInstKind,
      TypeOp::CreateBoolean(&tmp_builder));

  auto* val1 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  auto* val2 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));

  auto* test_inst =
      tmp_builder.Create<CondBranchInst>(0, cond, true_bb, false_bb);

  tmp_builder.SetInsertionPointToEnd(true_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(phi_bb);
  auto* phi = tmp_builder.Create<PhiInst>(0, PhiInst::ValueListType(),
                                          PhiInst::BlockListType());

  phi->AddEntry(val1, true_bb);
  phi->AddEntry(val2, false_bb);
  phi->SetType(TypeOp::CreateBoolean(&tmp_builder));

  tmp_builder.Create<CondBranchInst>(0, phi, true_bb1, false_bb1);
  tmp_builder.SetInsertionPointToEnd(true_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0));
  tmp_builder.SetInsertionPointToEnd(false_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(1));

  ASSERT_FALSE(OptIndirectJmp(test_inst));
}

TEST_F(LEPUSIRTestOptIndirectJmp, testOptIndirectJmp3) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  auto block = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                       0, "test_block");
  auto true_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                         0, "true_bb");
  auto false_bb = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "false_bb");
  auto phi_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                        0, "phi_bb");
  auto true_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "true_bb1");
  auto false_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                           BlockType::BT_INST, 0, "false_bb1");
  ASSERT_NE(nullptr, block);
  ASSERT_EQ(mod->GetIRRegion(), block->GetParent());
  tmp_builder.SetInsertionPointToEnd(block);

  auto var_1 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  auto var_2 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  auto* cond = tmp_builder.Create<BinaryOperatorInst>(
      0, var_1, var_2, ValueKind::BinaryStrictlyEqualInstKind,
      TypeOp::CreateBoolean(&tmp_builder));

  auto* test_inst =
      tmp_builder.Create<CondBranchInst>(0, cond, true_bb, false_bb);

  tmp_builder.SetInsertionPointToEnd(true_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(phi_bb);
  auto* phi = tmp_builder.Create<PhiInst>(0, PhiInst::ValueListType(),
                                          PhiInst::BlockListType());

  phi->AddEntry(tmp_builder.GetLiteralBool(true), true_bb);
  phi->AddEntry(cond, block);
  phi->SetType(TypeOp::CreateBoolean(&tmp_builder));

  tmp_builder.Create<CondBranchInst>(0, phi, true_bb1, false_bb1);
  tmp_builder.SetInsertionPointToEnd(true_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0));
  tmp_builder.SetInsertionPointToEnd(false_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(1));

  ASSERT_TRUE(OptIndirectJmp(test_inst));
}

TEST_F(LEPUSIRTestOptIndirectJmp, testOptIndirectJmp4) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  auto block = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                       0, "test_block");
  auto true_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                         0, "true_bb");
  auto false_bb = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "false_bb");
  auto phi_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                        0, "phi_bb");
  auto true_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "true_bb1");
  auto false_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                           BlockType::BT_INST, 0, "false_bb1");
  auto other_bb = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "other_bb");
  ASSERT_NE(nullptr, block);
  ASSERT_EQ(mod->GetIRRegion(), block->GetParent());
  tmp_builder.SetInsertionPointToEnd(block);

  auto var_1 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  auto var_2 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));

  auto* cond = tmp_builder.Create<BinaryOperatorInst>(
      0, var_1, var_2, ValueKind::BinaryStrictlyEqualInstKind,
      TypeOp::CreateBoolean(&tmp_builder));

  auto* test_inst =
      tmp_builder.Create<CondBranchInst>(0, cond, true_bb, false_bb);

  tmp_builder.SetInsertionPointToEnd(true_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(phi_bb);
  auto* phi = tmp_builder.Create<PhiInst>(0, PhiInst::ValueListType(),
                                          PhiInst::BlockListType());

  phi->AddEntry(tmp_builder.GetLiteralBool(true), other_bb);
  phi->AddEntry(cond, block);
  phi->SetType(TypeOp::CreateBoolean(&tmp_builder));

  tmp_builder.Create<CondBranchInst>(0, phi, true_bb1, false_bb1);
  tmp_builder.SetInsertionPointToEnd(true_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralBool(true));
  tmp_builder.SetInsertionPointToEnd(false_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralBool(false));

  ASSERT_FALSE(OptIndirectJmp(test_inst));
}

TEST_F(LEPUSIRTestOptIndirectJmp, testOptIndirectJmp5) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  auto block = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                       0, "test_block");
  auto phi_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                        0, "true_bb");
  auto false_bb = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "false_bb");

  auto true_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, 0, "true_bb1");
  auto false_bb1 = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                           BlockType::BT_INST, 0, "false_bb1");
  ASSERT_NE(nullptr, block);
  ASSERT_EQ(mod->GetIRRegion(), block->GetParent());
  tmp_builder.SetInsertionPointToEnd(block);

  auto var_1 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
  auto var_2 = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));

  auto* cond = tmp_builder.Create<BinaryOperatorInst>(
      0, var_1, var_2, ValueKind::BinaryStrictlyEqualInstKind,
      TypeOp::CreateBoolean(&tmp_builder));

  auto* test_inst =
      tmp_builder.Create<CondBranchInst>(0, cond, phi_bb, false_bb);

  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<BranchInst>(0, phi_bb);

  tmp_builder.SetInsertionPointToEnd(phi_bb);
  auto* phi = tmp_builder.Create<PhiInst>(0, PhiInst::ValueListType(),
                                          PhiInst::BlockListType());

  phi->AddEntry(cond, block);
  phi->AddEntry(tmp_builder.GetLiteralBool(false), false_bb);
  phi->SetType(TypeOp::CreateBoolean(&tmp_builder));

  tmp_builder.Create<CondBranchInst>(0, phi, true_bb1, false_bb1);
  tmp_builder.SetInsertionPointToEnd(true_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralBool(true));
  tmp_builder.SetInsertionPointToEnd(false_bb1);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralBool(false));

  ASSERT_TRUE(OptIndirectJmp(test_inst));
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
