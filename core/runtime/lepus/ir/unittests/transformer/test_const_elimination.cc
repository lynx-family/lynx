// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestConstantsElimination : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestConstantsElimination, testConstantElimination) {
  // we use the existed module operation to finish ut
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());
  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  tmp_builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_function";
  auto* func = tmp_builder.Create<FuncOp>(0, func_name);
  auto func_region = tmp_builder.CreateRegion(func);

  auto block = tmp_builder.CreateBlock(func_region, BlockType::BT_INST, {},
                                       "test_block");
  ASSERT_NE(nullptr, block);

  tmp_builder.SetInsertionPointToEnd(block);

  {
    auto val1 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
    auto val2 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto val3 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(0),
        TypeOp::CreateInt32(&tmp_builder));  // should be eliminated

    auto add_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, ValueKind::BinaryAddInstKind,
        TypeOp::CreateInt32(&tmp_builder));
    auto add_inst2 = tmp_builder.Create<BinaryOperatorInst>(
        0, add_inst, val3, ValueKind::BinaryAddInstKind,
        TypeOp::CreateInt32(&tmp_builder));
    tmp_builder.Create<ReturnInst>(0, add_inst2);
  }

  ASSERT_EQ(6, block->GetOpList().size());

  LoadStoreElimination load_store_elimination(ir_ctx.get());
  load_store_elimination.RunOnFunction(func);

  auto bb = func->GetSingleRegion()->GetEntryBlock();
  ASSERT_NE(nullptr, bb);

  ASSERT_EQ(5, bb->GetOpList().size());
}

TEST_F(LEPUSIRTestConstantsElimination, testGetGlobalElimination) {
  // we use the existed module operation to finish ut
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());
  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  tmp_builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_function";
  auto* func = tmp_builder.Create<FuncOp>(0, func_name);
  auto func_region = tmp_builder.CreateRegion(func);

  auto block = tmp_builder.CreateBlock(func_region, BlockType::BT_INST, {},
                                       "test_block");
  ASSERT_NE(nullptr, block);

  tmp_builder.SetInsertionPointToEnd(block);

  {
    auto val1 = tmp_builder.Create<GetGlobalInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto val2 = tmp_builder.Create<GetGlobalInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto val3 = tmp_builder.Create<GetGlobalInst>(
        0, tmp_builder.GetLiteralUint32(1),
        TypeOp::CreateInt32(&tmp_builder));  // should be eliminated

    auto add_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, ValueKind::BinaryAddInstKind,
        TypeOp::CreateInt32(&tmp_builder));
    auto add_inst2 = tmp_builder.Create<BinaryOperatorInst>(
        0, add_inst, val3, ValueKind::BinaryAddInstKind,
        TypeOp::CreateInt32(&tmp_builder));
    tmp_builder.Create<ReturnInst>(0, add_inst2);
  }

  ASSERT_EQ(6, block->GetOpList().size());

  LoadStoreElimination load_store_elimination(ir_ctx.get());
  load_store_elimination.RunOnFunction(func);

  auto bb = func->GetSingleRegion()->GetEntryBlock();
  ASSERT_NE(nullptr, bb);

  ASSERT_EQ(4, bb->GetOpList().size());
}

TEST_F(LEPUSIRTestConstantsElimination, testGetGlobalElimination2) {
  // we use the existed module operation to finish ut
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());
  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  tmp_builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_function";
  auto* func = tmp_builder.Create<FuncOp>(0, func_name);
  auto func_region = tmp_builder.CreateRegion(func);

  auto block = tmp_builder.CreateBlock(func_region, BlockType::BT_INST, {},
                                       "test_block");
  ASSERT_NE(nullptr, block);
  auto true_bb =
      tmp_builder.CreateBlock(func_region, BlockType::BT_INST, {}, "true_bb");
  auto false_bb =
      tmp_builder.CreateBlock(func_region, BlockType::BT_INST, {}, "false_bb");

  tmp_builder.SetInsertionPointToEnd(block);

  {
    auto val1 = tmp_builder.Create<GetGlobalInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto val2 = tmp_builder.Create<GetGlobalInst>(
        0, tmp_builder.GetLiteralUint32(2), TypeOp::CreateInt32(&tmp_builder));

    auto cmp_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, ValueKind::BinaryStrictlyEqualInstKind,
        TypeOp::CreateBoolean(&tmp_builder));
    ASSERT_EQ(val1, cmp_inst->GetOperand(0));
    ASSERT_EQ(val2, cmp_inst->GetOperand(1));
    tmp_builder.Create<CondBranchInst>(0, cmp_inst, true_bb, false_bb);
  }
  {
    tmp_builder.SetInsertionPointToEnd(true_bb);
    auto val = tmp_builder.Create<GetGlobalInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    tmp_builder.Create<ReturnInst>(0, val);
  }
  {
    tmp_builder.SetInsertionPointToEnd(false_bb);
    auto val = tmp_builder.Create<GetGlobalInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    tmp_builder.Create<ReturnInst>(0, val);
  }

  ASSERT_EQ(4, block->GetOpList().size());
  ASSERT_EQ(2, true_bb->GetOpList().size());
  ASSERT_EQ(2, false_bb->GetOpList().size());

  LoadStoreElimination load_store_elimination(ir_ctx.get());
  load_store_elimination.RunOnFunction(func);

  auto bb = func->GetSingleRegion()->GetEntryBlock();
  ASSERT_NE(nullptr, bb);
  ASSERT_EQ(4, bb->GetOpList().size());

  auto terminator_inst = bb->GetTerminator();
  ASSERT_TRUE(llvh::isa<CondBranchInst>(terminator_inst));
  auto bb1 = llvh::cast<CondBranchInst>(terminator_inst)->GetTrueDest();
  auto bb2 = llvh::cast<CondBranchInst>(terminator_inst)->GetFalseDest();
  ASSERT_EQ(1, bb1->GetOpList().size());
  ASSERT_EQ(1, bb2->GetOpList().size());
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
