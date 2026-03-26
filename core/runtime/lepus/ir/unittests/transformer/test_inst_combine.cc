// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "base/include/value/base_value.h"
#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/dce.h"
#include "core/runtime/lepus/ir/transformer/mir/inst_combine/inst_combine.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestInstCombineOp : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestInstCombineOp, testInstCombineCmpJmp) {
  // we use the existed module operation to finish ut
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);
  ASSERT_EQ(mod, tmp_builder.GetMod());
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetIRRegion();
  ASSERT_NE(nullptr, region);

  auto block = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                       {}, "test_block");
  auto true_bb = tmp_builder.CreateBlock(mod->GetIRRegion(), BlockType::BT_INST,
                                         {}, "true_bb");
  auto false_bb = tmp_builder.CreateBlock(mod->GetIRRegion(),
                                          BlockType::BT_INST, {}, "false_bb");
  ASSERT_NE(nullptr, block);
  ASSERT_EQ(mod->GetIRRegion(), block->GetParent());

  tmp_builder.SetInsertionPointToEnd(true_bb);
  { tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0)); }
  tmp_builder.SetInsertionPointToEnd(false_bb);
  { tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(1)); }

  tmp_builder.SetInsertionPointToEnd(block);

  {
    auto val1 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
    auto val2 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto cmp_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, ValueKind::BinaryStrictlyEqualInstKind,
        TypeOp::CreateBoolean(&tmp_builder));
    ASSERT_EQ(val1, cmp_inst->GetOperand(0));
    ASSERT_EQ(val2, cmp_inst->GetOperand(1));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, cmp_inst, true_bb, false_bb);
    cond_branch_inst->SetSmallJmp(true);

    auto* res = CombineCompareAndJmp(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res != nullptr);
    ASSERT_TRUE(llvh::isa<EqCondBranchInst>(res));
  }

  {
    auto val1 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
    auto val2 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto cmp_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, ValueKind::BinaryStrictlyNotEqualInstKind,
        TypeOp::CreateBoolean(&tmp_builder));
    ASSERT_EQ(val1, cmp_inst->GetOperand(0));
    ASSERT_EQ(val2, cmp_inst->GetOperand(1));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, cmp_inst, true_bb, false_bb);
    cond_branch_inst->SetSmallJmp(true);

    auto* res = CombineCompareAndJmp(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res != nullptr);
    ASSERT_TRUE(llvh::isa<NeqCondBranchInst>(res));
  }

  {
    auto val1 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
    auto val2 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto cmp_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, ValueKind::BinaryStrictlyEqualInstKind,
        TypeOp::CreateBoolean(&tmp_builder));
    ASSERT_EQ(val1, cmp_inst->GetOperand(0));
    ASSERT_EQ(val2, cmp_inst->GetOperand(1));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, cmp_inst, true_bb, false_bb);
    cond_branch_inst->SetSmallJmp(false);

    // for large jmp, we do not combine them
    auto* res = CombineCompareAndJmp(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res == nullptr);
  }

  {
    auto val1 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(0), TypeOp::CreateInt32(&tmp_builder));
    auto val2 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(1), TypeOp::CreateInt32(&tmp_builder));
    auto cmp_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, ValueKind::BinaryStrictlyNotEqualInstKind,
        TypeOp::CreateBoolean(&tmp_builder));
    ASSERT_EQ(val1, cmp_inst->GetOperand(0));
    ASSERT_EQ(val2, cmp_inst->GetOperand(1));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, cmp_inst, true_bb, false_bb);
    cond_branch_inst->SetSmallJmp(false);

    // for large jmp, we do not combine them
    auto* res = CombineCompareAndJmp(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res == nullptr);
  }
}

TEST_F(LEPUSIRTestInstCombineOp, testInstCombineCondBranch) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_func";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();
  auto true_bb =
      tmp_builder.CreateBlock(region, BlockType::BT_INST, {}, "true_bb");
  auto false_bb =
      tmp_builder.CreateBlock(region, BlockType::BT_INST, {}, "false_bb");

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  uint32_t true_idx = lepus_func->AddConstBoolean(true);
  uint32_t false_idx = lepus_func->AddConstBoolean(false);
  uint32_t zero_idx = lepus_func->AddConstNumber(0);
  uint32_t nonzero_idx = lepus_func->AddConstNumber(123);

  tmp_builder.SetInsertionPointToEnd(true_bb);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0));
  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(1));

  tmp_builder.SetInsertionPointToEnd(block);

  {
    // Test true constant
    auto val = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(true_idx),
        TypeOp::CreateBoolean(&tmp_builder));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, val, true_bb, false_bb);

    auto* res = CombineCondBranch(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res != nullptr);
    ASSERT_TRUE(llvh::isa<BranchInst>(res));
    ASSERT_EQ(llvh::cast<BranchInst>(res)->GetBranchDest(), true_bb);
  }

  {
    // Test false constant
    auto val = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(false_idx),
        TypeOp::CreateBoolean(&tmp_builder));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, val, true_bb, false_bb);

    auto* res = CombineCondBranch(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res != nullptr);
    ASSERT_TRUE(llvh::isa<BranchInst>(res));
    ASSERT_EQ(llvh::cast<BranchInst>(res)->GetBranchDest(), false_bb);
  }

  {
    // Test 0 (false)
    auto val = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(zero_idx),
        TypeOp::CreateInt32(&tmp_builder));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, val, true_bb, false_bb);

    auto* res = CombineCondBranch(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res != nullptr);
    ASSERT_TRUE(llvh::isa<BranchInst>(res));
    ASSERT_EQ(llvh::cast<BranchInst>(res)->GetBranchDest(), false_bb);
  }

  {
    // Test non-zero (true)
    auto val = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(nonzero_idx),
        TypeOp::CreateInt32(&tmp_builder));
    auto cond_branch_inst =
        tmp_builder.Create<CondBranchInst>(0, val, true_bb, false_bb);

    auto* res = CombineCondBranch(&tmp_builder, cond_branch_inst);
    ASSERT_TRUE(res != nullptr);
    ASSERT_TRUE(llvh::isa<BranchInst>(res));
    ASSERT_EQ(llvh::cast<BranchInst>(res)->GetBranchDest(), true_bb);
  }
}

TEST_F(LEPUSIRTestInstCombineOp, testConstantFoldBinaryOps) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_func";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  tmp_builder.SetInsertionPointToEnd(block);

  auto check_fold = [&](ValueKind kind, uint32_t left_idx, uint32_t right_idx,
                        bool expected_val) {
    auto val1 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(left_idx),
        TypeOp::CreateAnyType(&tmp_builder));
    auto val2 = tmp_builder.Create<LoadConstInst>(
        0, tmp_builder.GetLiteralUint32(right_idx),
        TypeOp::CreateAnyType(&tmp_builder));
    auto binary_inst = tmp_builder.Create<BinaryOperatorInst>(
        0, val1, val2, kind, TypeOp::CreateBoolean(&tmp_builder));

    auto* res = ConstantFold(&tmp_builder, binary_inst);
    ASSERT_TRUE(res != nullptr);
    ASSERT_TRUE(llvh::isa<LoadConstInst>(res));
    auto res_const_idx =
        llvh::cast<LiteralUint32>(llvh::cast<LoadConstInst>(res)->GetConst())
            ->GetValue();
    auto* res_val = lepus_func->GetConstValue(res_const_idx);
    ASSERT_TRUE(res_val->IsBool());
    EXPECT_EQ(res_val->Bool(), expected_val);
  };

  // 1. Equality
  uint32_t n10_idx = lepus_func->AddConstNumber(10);
  uint32_t n20_idx = lepus_func->AddConstNumber(20);
  uint32_t n10_copy_idx = lepus_func->AddConstNumber(10);

  check_fold(ValueKind::BinaryStrictlyEqualInstKind, n10_idx, n10_copy_idx,
             true);
  check_fold(ValueKind::BinaryStrictlyEqualInstKind, n10_idx, n20_idx, false);
  check_fold(ValueKind::BinaryStrictlyNotEqualInstKind, n10_idx, n20_idx, true);
  check_fold(ValueKind::BinaryStrictlyNotEqualInstKind, n10_idx, n10_copy_idx,
             false);

  // 2. Number Comparisons
  check_fold(ValueKind::BinaryGreaterThanInstKind, n20_idx, n10_idx, true);
  check_fold(ValueKind::BinaryGreaterThanInstKind, n10_idx, n20_idx, false);
  check_fold(ValueKind::BinaryGreaterThanOrEqualInstKind, n20_idx, n10_idx,
             true);
  check_fold(ValueKind::BinaryGreaterThanOrEqualInstKind, n10_idx, n10_copy_idx,
             true);
  check_fold(ValueKind::BinaryLessThanInstKind, n10_idx, n20_idx, true);
  check_fold(ValueKind::BinaryLessThanOrEqualInstKind, n10_idx, n20_idx, true);
  check_fold(ValueKind::BinaryLessThanOrEqualInstKind, n10_idx, n10_copy_idx,
             true);

  // 3. String Comparisons
  uint32_t sa_idx = lepus_func->AddConstString("a");
  uint32_t sb_idx = lepus_func->AddConstString("b");
  uint32_t sa_copy_idx = lepus_func->AddConstString("a");

  check_fold(ValueKind::BinaryGreaterThanInstKind, sb_idx, sa_idx, true);
  check_fold(ValueKind::BinaryGreaterThanInstKind, sa_idx, sb_idx, false);
  check_fold(ValueKind::BinaryGreaterThanOrEqualInstKind, sb_idx, sa_idx, true);
  check_fold(ValueKind::BinaryGreaterThanOrEqualInstKind, sa_idx, sa_copy_idx,
             true);
  check_fold(ValueKind::BinaryLessThanInstKind, sa_idx, sb_idx, true);
  check_fold(ValueKind::BinaryLessThanOrEqualInstKind, sa_idx, sb_idx, true);
  check_fold(ValueKind::BinaryLessThanOrEqualInstKind, sa_idx, sa_copy_idx,
             true);
}

TEST_F(LEPUSIRTestInstCombineOp, testConstantFoldUnaryNegInt64) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_fold_unary_neg_int64";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  uint32_t one_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(1));

  tmp_builder.SetInsertionPointToEnd(block);
  auto* one = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(one_idx),
      TypeOp::CreateInt64(&tmp_builder));
  auto* neg = tmp_builder.Create<UnaryOperatorInst>(
      0, one, ValueKind::UnaryNegInstKind);

  auto* res = ConstantFold(&tmp_builder, neg);
  ASSERT_NE(nullptr, res);
  ASSERT_TRUE(llvh::isa<LoadConstInst>(res));
  auto idx =
      llvh::cast<LiteralUint32>(llvh::cast<LoadConstInst>(res)->GetConst())
          ->GetValue();
  auto* v = lepus_func->GetConstValue(idx);
  ASSERT_NE(nullptr, v);
  ASSERT_TRUE(v->IsInt64());
  EXPECT_EQ(v->Int64(), -1);
}

TEST_F(LEPUSIRTestInstCombineOp, testConstantFoldUnaryNegDouble) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_fold_unary_neg_double";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  uint32_t idx = static_cast<uint32_t>(lepus_func->AddConstNumber(2.5));

  tmp_builder.SetInsertionPointToEnd(block);
  auto* c =
      tmp_builder.Create<LoadConstInst>(0, tmp_builder.GetLiteralUint32(idx),
                                        TypeOp::CreateFloat64(&tmp_builder));
  auto* neg =
      tmp_builder.Create<UnaryOperatorInst>(0, c, ValueKind::UnaryNegInstKind);

  auto* res = ConstantFold(&tmp_builder, neg);
  ASSERT_NE(nullptr, res);
  ASSERT_TRUE(llvh::isa<LoadConstInst>(res));
  auto out_idx =
      llvh::cast<LiteralUint32>(llvh::cast<LoadConstInst>(res)->GetConst())
          ->GetValue();
  auto* v = lepus_func->GetConstValue(out_idx);
  ASSERT_NE(nullptr, v);
  ASSERT_TRUE(v->IsNumber());
  EXPECT_DOUBLE_EQ(v->Number(), -2.5);
}

TEST_F(LEPUSIRTestInstCombineOp, testConstantFoldUnaryNegSkipDoubleZero) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_fold_unary_neg_skip_double_zero";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  // Force a double 0.0 in const table (not int64) to guard against -0
  // semantics changes.
  lepus::Value z;
  z.SetNumber(0.0);
  uint32_t idx = static_cast<uint32_t>(lepus_func->AddConstValue(z));

  tmp_builder.SetInsertionPointToEnd(block);
  auto* c =
      tmp_builder.Create<LoadConstInst>(0, tmp_builder.GetLiteralUint32(idx),
                                        TypeOp::CreateFloat64(&tmp_builder));
  auto* neg =
      tmp_builder.Create<UnaryOperatorInst>(0, c, ValueKind::UnaryNegInstKind);

  auto* res = ConstantFold(&tmp_builder, neg);
  EXPECT_EQ(nullptr, res);
}

TEST_F(LEPUSIRTestInstCombineOp, testConstantFoldUnaryNegSkipInt64Min) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_fold_unary_neg_skip_int64_min";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  lepus::Value v;
  v.SetNumber(std::numeric_limits<int64_t>::min());
  uint32_t idx = static_cast<uint32_t>(lepus_func->AddConstValue(v));

  tmp_builder.SetInsertionPointToEnd(block);
  auto* c = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(idx), TypeOp::CreateInt64(&tmp_builder));
  auto* neg =
      tmp_builder.Create<UnaryOperatorInst>(0, c, ValueKind::UnaryNegInstKind);

  auto* res = ConstantFold(&tmp_builder, neg);
  EXPECT_EQ(nullptr, res);
}

TEST_F(LEPUSIRTestInstCombineOp, testConstantFoldUnaryNegOperandHasMultiUsers) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_fold_unary_neg_operand_multi_users";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  uint32_t idx = static_cast<uint32_t>(lepus_func->AddConstNumber(7));

  tmp_builder.SetInsertionPointToEnd(block);
  auto* c = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(idx), TypeOp::CreateInt64(&tmp_builder));
  // Make `c` have another use besides UnaryNeg.
  tmp_builder.Create<MovInst>(0, c);

  auto* neg =
      tmp_builder.Create<UnaryOperatorInst>(0, c, ValueKind::UnaryNegInstKind);
  auto* res = ConstantFold(&tmp_builder, neg);
  ASSERT_NE(nullptr, res);
  ASSERT_TRUE(llvh::isa<LoadConstInst>(res));
}

TEST_F(LEPUSIRTestInstCombineOp, testCombineSetTableConstStringKey) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_set_table_const_string_key";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Build: key = LoadConst("foo"); obj[ key ] = 1
  auto* obj = builder.Create<NewTableInst>(0);
  ASSERT_TRUE(obj->GetType()->IsTableType());

  uint32_t foo_idx = lepus_func->AddConstString("foo");
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  ASSERT_TRUE(key->GetType()->IsStringType());

  auto* store = builder.GetLiteralInt32(1);
  store->SetType(TypeOp::CreateInt32(&builder));
  auto* set_table = builder.Create<SetTableInst>(0, obj, key, store);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  // InstCombine is followed by DCE in the real optimization pipeline.
  // Run it here to drop the now-dead LoadConstInst.
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  bool found_combined = false;
  bool found_original = false;
  bool found_key = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == set_table) found_original = true;
    if (inst == key) found_key = true;
    if (llvh::isa<SetTableConstStringKeyInst>(inst)) {
      found_combined = true;
    }
  }
  EXPECT_TRUE(found_combined);
  EXPECT_FALSE(found_original);
  EXPECT_FALSE(found_key);
}

TEST_F(LEPUSIRTestInstCombineOp, testCombineGetTableConstStringKeyThroughPhi) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_get_table_const_string_key_phi";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  // CFG:
  //   entry -> (then, else) -> join
  // In both then/else, we materialize the same const-string key, and join via
  // PhiInst. InstCombine should still recognize it as a const-string key and
  // rewrite GetTableInst -> GetTableConstStringKeyInst.
  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* then_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* else_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});

  uint32_t foo_idx = lepus_func->AddConstString("foo");

  builder.SetInsertionPointToStart(entry);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), then_bb,
                                 else_bb);

  builder.SetInsertionPointToStart(then_bb);
  auto* key_then = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  builder.Create<BranchInst>(0, join_bb);

  builder.SetInsertionPointToStart(else_bb);
  auto* key_else = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  builder.Create<BranchInst>(0, join_bb);

  builder.SetInsertionPointToStart(join_bb);
  auto* obj = builder.Create<NewTableInst>(0);
  ASSERT_TRUE(obj->GetType()->IsTableType());

  PhiInst::ValueListType values{key_then, key_else};
  PhiInst::BlockListType blocks{then_bb, else_bb};
  auto* key_phi = builder.Create<PhiInst>(0, values, blocks);

  auto* get_table = builder.Create<GetTableInst>(0, obj, key_phi);
  builder.Create<ReturnInst>(0, get_table);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  // Run DCE to remove now-dead LoadConstInst / PhiInst if possible.
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  bool found_original = false;
  bool found_combined = false;
  uint32_t combined_idx = 0;
  for (auto* inst : join_bb->InstRange()) {
    if (inst == get_table) found_original = true;
    if (auto* g = llvh::dyn_cast<GetTableConstStringKeyInst>(inst)) {
      found_combined = true;
      ASSERT_TRUE(llvh::isa<LiteralUint32>(g->GetConstIndex()));
      combined_idx = llvh::cast<LiteralUint32>(g->GetConstIndex())->GetValue();
    }
  }

  EXPECT_TRUE(found_combined);
  EXPECT_FALSE(found_original);
  EXPECT_EQ(combined_idx, foo_idx);
}

TEST_F(LEPUSIRTestInstCombineOp,
       testCombineGetTableConstStringKeyPhiRejectsDifferentIncomingKeys) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_get_table_const_string_key_phi_mismatch";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* then_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* else_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});

  uint32_t foo_idx = lepus_func->AddConstString("foo");
  uint32_t bar_idx = lepus_func->AddConstString("bar");

  builder.SetInsertionPointToStart(entry);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), then_bb,
                                 else_bb);

  builder.SetInsertionPointToStart(then_bb);
  auto* key_then = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  builder.Create<BranchInst>(0, join_bb);

  builder.SetInsertionPointToStart(else_bb);
  auto* key_else = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(bar_idx), TypeOp::CreateString(&builder));
  builder.Create<BranchInst>(0, join_bb);

  builder.SetInsertionPointToStart(join_bb);
  auto* obj = builder.Create<NewTableInst>(0);
  ASSERT_TRUE(obj->GetType()->IsTableType());

  PhiInst::ValueListType values{key_then, key_else};
  PhiInst::BlockListType blocks{then_bb, else_bb};
  auto* key_phi = builder.Create<PhiInst>(0, values, blocks);
  auto* get_table = builder.Create<GetTableInst>(0, obj, key_phi);
  builder.Create<ReturnInst>(0, get_table);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  bool found_original = false;
  bool found_combined = false;
  for (auto* inst : join_bb->InstRange()) {
    if (inst == get_table) found_original = true;
    if (llvh::isa<GetTableConstStringKeyInst>(inst)) found_combined = true;
  }

  // Different incoming keys cannot be proven to the same const string key.
  EXPECT_TRUE(found_original);
  EXPECT_FALSE(found_combined);
}

TEST_F(LEPUSIRTestInstCombineOp,
       testCombineGetTableConstStringKeyRejectsNonStringConst) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_get_table_const_string_key_reject_non_string";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  ASSERT_TRUE(obj->GetType()->IsTableType());

  // key is a LoadConstInst, but its const table value is NOT a string.
  uint32_t num_idx = lepus_func->AddConstNumber(3.14);
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(num_idx), TypeOp::CreateInt64(&builder));
  auto* get_table = builder.Create<GetTableInst>(0, obj, key);
  builder.Create<ReturnInst>(0, get_table);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  bool found_original = false;
  bool found_combined = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_table) found_original = true;
    if (llvh::isa<GetTableConstStringKeyInst>(inst)) found_combined = true;
  }
  EXPECT_TRUE(found_original);
  EXPECT_FALSE(found_combined);
}

TEST_F(LEPUSIRTestInstCombineOp,
       testCombineGetTableConstStringKeyRejectsIndexOverflow) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_get_table_const_string_key_reject_big_index";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  ASSERT_TRUE(obj->GetType()->IsTableType());

  // Create a const string index > 255; must NOT be rewritten.
  uint32_t large_idx = 0;
  for (int i = 0; i < 260; i++) {
    large_idx =
        lepus_func->AddConstString(std::string("k") + std::to_string(i));
  }
  ASSERT_GT(large_idx, 255u);

  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(large_idx), TypeOp::CreateString(&builder));
  auto* get_table = builder.Create<GetTableInst>(0, obj, key);
  builder.Create<ReturnInst>(0, get_table);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  bool found_original = false;
  bool found_combined = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_table) found_original = true;
    if (llvh::isa<GetTableConstStringKeyInst>(inst)) found_combined = true;
  }
  EXPECT_TRUE(found_original);
  EXPECT_FALSE(found_combined);
}

TEST_F(LEPUSIRTestInstCombineOp, testCombineSetTableConstStringKeyThroughPhi) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_set_table_const_string_key_phi";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* then_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* else_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join_bb =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});

  uint32_t foo_idx = lepus_func->AddConstString("foo");

  builder.SetInsertionPointToStart(entry);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), then_bb,
                                 else_bb);

  builder.SetInsertionPointToStart(then_bb);
  auto* key_then = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  builder.Create<BranchInst>(0, join_bb);

  builder.SetInsertionPointToStart(else_bb);
  auto* key_else = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  builder.Create<BranchInst>(0, join_bb);

  builder.SetInsertionPointToStart(join_bb);
  auto* obj = builder.Create<NewTableInst>(0);
  ASSERT_TRUE(obj->GetType()->IsTableType());

  PhiInst::ValueListType values{key_then, key_else};
  PhiInst::BlockListType blocks{then_bb, else_bb};
  auto* key_phi = builder.Create<PhiInst>(0, values, blocks);

  auto* store = builder.GetLiteralInt32(1);
  store->SetType(TypeOp::CreateInt32(&builder));
  auto* set_table = builder.Create<SetTableInst>(0, obj, key_phi, store);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  bool found_original = false;
  bool found_combined = false;
  uint32_t combined_idx = 0;
  for (auto* inst : join_bb->InstRange()) {
    if (inst == set_table) found_original = true;
    if (auto* s = llvh::dyn_cast<SetTableConstStringKeyInst>(inst)) {
      found_combined = true;
      ASSERT_TRUE(llvh::isa<LiteralUint32>(s->GetConstIndex()));
      combined_idx = llvh::cast<LiteralUint32>(s->GetConstIndex())->GetValue();
    }
  }
  EXPECT_FALSE(found_original);
  EXPECT_TRUE(found_combined);
  EXPECT_EQ(combined_idx, foo_idx);
}

TEST_F(LEPUSIRTestInstCombineOp, testAddEmptyStringToToString) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_add_empty_string_to_to_string";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  uint32_t num_idx = lepus_func->AddConstNumber(123);
  auto* num = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(num_idx), TypeOp::CreateInt64(&builder));

  uint32_t empty_idx = lepus_func->AddConstString("");
  auto* empty = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(empty_idx), TypeOp::CreateString(&builder));

  // Build both directions: x + "" and "" + x.
  auto* add_rhs_empty = builder.Create<BinaryOperatorInst>(
      0, num, empty, ValueKind::BinaryAddInstKind,
      TypeOp::CreateString(&builder));
  auto* add_lhs_empty = builder.Create<BinaryOperatorInst>(
      0, empty, num, ValueKind::BinaryAddInstKind,
      TypeOp::CreateString(&builder));

  ArgList arr_items;
  arr_items.push_back(add_rhs_empty);
  arr_items.push_back(add_lhs_empty);
  auto* arr = builder.Create<NewArrayInst>(0, arr_items);
  builder.Create<ReturnInst>(0, arr);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  int to_string_cnt = 0;
  int add_cnt = 0;
  bool all_src_match_num = true;
  for (auto* inst : entry->InstRange()) {
    if (auto* b = llvh::dyn_cast<BinaryOperatorInst>(inst)) {
      if (b->GetKind() == ValueKind::BinaryAddInstKind) {
        add_cnt++;
      }
    }
    if (auto* ts = llvh::dyn_cast<ToStringInst>(inst)) {
      to_string_cnt++;
      all_src_match_num &= (ts->GetSrc() == num);
    }
  }

  EXPECT_EQ(add_cnt, 0);
  EXPECT_GE(to_string_cnt, 2);
  EXPECT_TRUE(all_src_match_num);
}

TEST_F(LEPUSIRTestInstCombineOp, testAddEmptyStringToToStringBool) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_add_empty_string_to_to_string_bool";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  uint32_t bool_idx = lepus_func->AddConstBoolean(true);
  auto* bval = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(bool_idx), TypeOp::CreateBoolean(&builder));

  uint32_t empty_idx = lepus_func->AddConstString("");
  auto* empty = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(empty_idx), TypeOp::CreateString(&builder));

  auto* add = builder.Create<BinaryOperatorInst>(
      0, bval, empty, ValueKind::BinaryAddInstKind,
      TypeOp::CreateString(&builder));
  builder.Create<ReturnInst>(0, add);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  bool found_to_string = false;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<BinaryOperatorInst>(inst) &&
        llvh::cast<BinaryOperatorInst>(inst)->GetKind() ==
            ValueKind::BinaryAddInstKind) {
      FAIL() << "BinaryAddInst should be rewritten";
    }
    if (auto* ts = llvh::dyn_cast<ToStringInst>(inst)) {
      found_to_string = true;
      EXPECT_EQ(ts->GetSrc(), bval);
    }
  }
  EXPECT_TRUE(found_to_string);
}

TEST_F(LEPUSIRTestInstCombineOp, testSimplifyTripleUnaryNot) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_triple_not";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  uint32_t one_idx = lepus_func->AddConstNumber(1);
  tmp_builder.SetInsertionPointToEnd(block);
  auto one = tmp_builder.Create<LoadConstInst>(
      0, tmp_builder.GetLiteralUint32(one_idx),
      TypeOp::CreateInt64(&tmp_builder));

  auto* not1 = tmp_builder.Create<UnaryOperatorInst>(
      0, one, ValueKind::UnaryNotInstKind);
  auto* not2 = tmp_builder.Create<UnaryOperatorInst>(
      0, not1, ValueKind::UnaryNotInstKind);
  auto* not3 = tmp_builder.Create<UnaryOperatorInst>(
      0, not2, ValueKind::UnaryNotInstKind);

  auto* folded = ConstantFold(&tmp_builder, not3);
  ASSERT_TRUE(folded != nullptr);
  // !!!x -> !x
  EXPECT_EQ(folded, not1);
}

TEST_F(LEPUSIRTestInstCombineOp, testSimplifyDoubleUnaryNotInCondBranch) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_double_not_cond";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();
  auto true_bb =
      tmp_builder.CreateBlock(region, BlockType::BT_INST, {}, "true_bb");
  auto false_bb =
      tmp_builder.CreateBlock(region, BlockType::BT_INST, {}, "false_bb");

  tmp_builder.SetInsertionPointToEnd(true_bb);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0));
  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(1));

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  uint32_t n_idx = lepus_func->AddConstNumber(123);
  tmp_builder.SetInsertionPointToEnd(block);
  auto n =
      tmp_builder.Create<LoadConstInst>(0, tmp_builder.GetLiteralUint32(n_idx),
                                        TypeOp::CreateInt64(&tmp_builder));

  auto* not1 =
      tmp_builder.Create<UnaryOperatorInst>(0, n, ValueKind::UnaryNotInstKind);
  auto* not2 = tmp_builder.Create<UnaryOperatorInst>(
      0, not1, ValueKind::UnaryNotInstKind);
  tmp_builder.Create<CondBranchInst>(0, not2, true_bb, false_bb);

  auto* folded = ConstantFold(&tmp_builder, not2);
  ASSERT_TRUE(folded != nullptr);
  // !!x used only by cond branch -> x
  EXPECT_EQ(folded, n);
}

TEST_F(LEPUSIRTestInstCombineOp, testInvertCondBranchUnaryNot) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_invert_cond_branch_unary_not";
  tmp_builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  auto* func_op = tmp_builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);
  auto* block =
      tmp_builder.CreateBlock(region, BlockType::BT_INST, {}, "entry");
  ASSERT_NE(nullptr, block);
  auto true_bb =
      tmp_builder.CreateBlock(region, BlockType::BT_INST, {}, "true_bb");
  auto false_bb =
      tmp_builder.CreateBlock(region, BlockType::BT_INST, {}, "false_bb");

  tmp_builder.SetInsertionPointToEnd(true_bb);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(1));
  tmp_builder.SetInsertionPointToEnd(false_bb);
  tmp_builder.Create<ReturnInst>(0, tmp_builder.GetLiteralInt32(0));

  tmp_builder.SetInsertionPointToEnd(block);
  auto* param = func_op->CreateParam(0);
  param->SetType(TypeOp::CreateAnyType(&tmp_builder));

  auto* not1 = tmp_builder.Create<UnaryOperatorInst>(
      0, param, ValueKind::UnaryNotInstKind);
  auto* cond_br =
      tmp_builder.Create<CondBranchInst>(0, not1, true_bb, false_bb);
  cond_br->SetSmallJmp(true);

  InstCombinePass pass(ir_ctx.get());
  try {
    pass.RunOnFunction(func_op);
  } catch (const ::lynx::lepus::CompileException& e) {
    FAIL() << e.message();
  } catch (...) {
    FAIL() << "unknown exception";
  }

  // DCE should remove the now-dead UnaryNotInst.
  DCE dce_pass(ir_ctx.get());
  try {
    dce_pass.RunOnModule(mod);
  } catch (const ::lynx::lepus::CompileException& e) {
    FAIL() << e.message();
  } catch (...) {
    FAIL() << "unknown exception";
  }

  // The cond branch should be inverted in-place.
  EXPECT_EQ(cond_br->GetCondition(), param);
  EXPECT_EQ(cond_br->GetTrueDest(), false_bb);
  EXPECT_EQ(cond_br->GetFalseDest(), true_bb);
  EXPECT_TRUE(cond_br->IsSmallJmp());

  // DCE should remove the now-dead UnaryNotInst.
  bool found_not = false;
  for (auto* inst : block->InstRange()) {
    if (inst == not1) found_not = true;
  }
  EXPECT_FALSE(found_not);
}

TEST_F(LEPUSIRTestInstCombineOp, testSimplifyDoubleUnaryNotOnBooleanProducer) {
  ASSERT_NE(nullptr, mod);

  OpBuilder tmp_builder;
  tmp_builder.SetModuleOp(mod);

  std::string func_name = "test_double_not_bool";
  FuncOp* func_op;
  CreateTestFuncOp(func_name, func_op);
  auto region = func_op->GetSingleRegion();
  auto block = &region->Front();

  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);

  uint32_t a_idx = lepus_func->AddConstNumber(1);
  uint32_t b_idx = lepus_func->AddConstNumber(2);

  tmp_builder.SetInsertionPointToEnd(block);
  auto a =
      tmp_builder.Create<LoadConstInst>(0, tmp_builder.GetLiteralUint32(a_idx),
                                        TypeOp::CreateInt64(&tmp_builder));
  auto b =
      tmp_builder.Create<LoadConstInst>(0, tmp_builder.GetLiteralUint32(b_idx),
                                        TypeOp::CreateInt64(&tmp_builder));
  auto* cmp = tmp_builder.Create<BinaryOperatorInst>(
      0, a, b, ValueKind::BinaryStrictlyEqualInstKind,
      TypeOp::CreateBoolean(&tmp_builder));

  auto* not1 = tmp_builder.Create<UnaryOperatorInst>(
      0, cmp, ValueKind::UnaryNotInstKind);
  auto* not2 = tmp_builder.Create<UnaryOperatorInst>(
      0, not1, ValueKind::UnaryNotInstKind);

  auto* folded = ConstantFold(&tmp_builder, not2);
  ASSERT_TRUE(folded != nullptr);
  // !!b -> b when b is always boolean.
  EXPECT_EQ(folded, cmp);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
