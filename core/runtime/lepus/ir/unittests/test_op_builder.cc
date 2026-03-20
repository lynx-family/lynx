// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <limits>

#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/ir/utils/eval.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestOpBuilder : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestOpBuilder, testBasic) {
  // 1. test size
  ASSERT_EQ(24, sizeof(OpBuilder));

  // 2. test constructor
  OpBuilder tmp_builder;
  ASSERT_EQ(nullptr, tmp_builder.GetMod());
  ASSERT_EQ(nullptr, tmp_builder.GetBlock());
}

TEST_F(LEPUSIRTestOpBuilder, testCreateRegion) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = tmp_builder.CreateRegion(mod, "test");
  ASSERT_NE(nullptr, region);
  ASSERT_EQ(2, mod->GetRegionSize());
  ASSERT_EQ(mod, region->GetParent());
  ASSERT_NE(mod->GetRegions().end(),
            llvh::find_if(mod->GetRegions(), [&](std::unique_ptr<Region>& reg) {
              return reg.get() == region;
            }));
}

TEST_F(LEPUSIRTestOpBuilder, testCreateBlock) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());
  ASSERT_EQ(3, mod->GetIRRegion()->GetBlockSize());

  auto block = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                       {}, "test_block");
  ASSERT_NE(nullptr, block);
  ASSERT_EQ(mod->GetIRRegion(), block->GetParent());

  auto iter = llvh::find_if(mod->GetIRRegion()->GetBlocks(),
                            [&](Block& b) { return &b == block; });
  ASSERT_NE(mod->GetIRRegion()->GetBlocks().end(), iter);
}

TEST_F(LEPUSIRTestOpBuilder, testEvalToBooleanForLiterals) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);

  // null/undefined -> false
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralNull());
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralUndefined());
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }

  // bool
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralBool(true));
    ASSERT_NE(nullptr, b);
    EXPECT_TRUE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralBool(false));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }

  // signed ints
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralInt8(0));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralInt8(-1));
    ASSERT_NE(nullptr, b);
    EXPECT_TRUE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralInt32(0));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralInt32(1));
    ASSERT_NE(nullptr, b);
    EXPECT_TRUE(b->GetValue());
  }
  // unsigned ints
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralUint8(0));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralUint8(255));
    ASSERT_NE(nullptr, b);
    EXPECT_TRUE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralUint32(0));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralUint32(123));
    ASSERT_NE(nullptr, b);
    EXPECT_TRUE(b->GetValue());
  }

  // float64
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralFloat64(0.0));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralFloat64(-0.0));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(&builder, builder.GetLiteralFloat64(1.25));
    ASSERT_NE(nullptr, b);
    EXPECT_TRUE(b->GetValue());
  }
  {
    auto* b = EvalToBoolean(
        &builder,
        builder.GetLiteralFloat64(std::numeric_limits<double>::quiet_NaN()));
    ASSERT_NE(nullptr, b);
    EXPECT_FALSE(b->GetValue());
  }
}

TEST_F(LEPUSIRTestOpBuilder, testEvalToBooleanForTableValue) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);

  auto* region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);
  auto* block =
      builder.CreateBlock(region, BlockType::BT_INST, 0, "eval_block");
  ASSERT_NE(nullptr, block);
  builder.SetInsertionPointToEnd(block);

  auto* table = builder.Create<NewTableInst>(0);
  ASSERT_NE(nullptr, table);

  auto* b = EvalToBoolean(&builder, static_cast<Value*>(table));
  ASSERT_NE(nullptr, b);
  EXPECT_TRUE(b->GetValue());
}

TEST_F(LEPUSIRTestOpBuilder, testCloneInstAndDestroyer) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);

  auto* region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);
  auto* block =
      builder.CreateBlock(region, BlockType::BT_INST, 0, "clone_block");
  ASSERT_NE(nullptr, block);
  builder.SetInsertionPointToEnd(block);

  auto* v0 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(7),
                                           TypeOp::CreateInt32(&builder));
  ASSERT_NE(nullptr, v0);
  auto* m0 = builder.Create<MovInst>(0, v0);
  ASSERT_NE(nullptr, m0);
  auto* r0 = builder.Create<ReturnInst>(0, m0);
  ASSERT_NE(nullptr, r0);
  const size_t size_before_clone = block->size();

  // Keep terminator as the last instruction in the block.
  builder.SetInsertionPoint(r0);
  auto* cloned = builder.CloneInst(m0);
  ASSERT_NE(nullptr, cloned);
  EXPECT_EQ(size_before_clone + 1, block->size());
  EXPECT_EQ(m0->GetKind(), cloned->GetKind());
  EXPECT_EQ(m0->GetNumOperands(), cloned->GetNumOperands());
  EXPECT_EQ(m0->GetOperand(0), cloned->GetOperand(0));

  {
    InstructionDestroyer destroyer;
    destroyer.Add(cloned);
  }
  EXPECT_EQ(size_before_clone, block->size());

  // Cover createEmptyRegion / cloneBlock / createTmpBlock (smoke).
  auto region_cnt_before = mod->GetRegionSize();
  auto* empty_region = builder.CreateEmptyRegion(mod, "empty_region");
  ASSERT_NE(nullptr, empty_region);
  EXPECT_EQ(region_cnt_before + 1, mod->GetRegionSize());

  auto* cloned_block = builder.CloneBlock(region, block, "cloned_block");
  ASSERT_NE(nullptr, cloned_block);
  EXPECT_EQ(region, cloned_block->GetParent());

  auto* tmp_block = builder.CreateTmpBlock(ir_ctx.get());
  ASSERT_NE(nullptr, tmp_block);
  delete tmp_block;
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
