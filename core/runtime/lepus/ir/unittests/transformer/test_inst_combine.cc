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
#include "core/runtime/lepus/vm_context.h"

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

TEST(LEPUSIRInstCombine, foldNullishGuardChainToSingleNullTrampoline) {
  // Build a minimal CFG for chained lowering:
  //   t1 = (x == undef) ? undef : GetTable(x, k0)
  //   t2 = (t1 == undef) ? undef : GetTable(t1, k1)
  // and verify InstCombine redirects the first null branch directly to the
  // last null trampoline block (early short-circuit), so the null case does
  // not flow through intermediate merge/phi blocks.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);

  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_fold";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* nil1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil1");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* nil2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil2");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");
  ASSERT_NE(nullptr, entry);
  ASSERT_NE(nullptr, nil1);
  ASSERT_NE(nullptr, get1);
  ASSERT_NE(nullptr, merge1);
  ASSERT_NE(nullptr, nil2);
  ASSERT_NE(nullptr, get2);
  ASSERT_NE(nullptr, merge2);

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  // entry:
  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef, nil1, get1);

  // nil1:
  builder->SetInsertionPointToEnd(nil1);
  builder->Create<BranchInst>(0, merge1);

  // get1:
  builder->SetInsertionPointToEnd(get1);
  auto* g1 = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  // merge1:
  builder->SetInsertionPointToEnd(merge1);
  PhiInst::ValueListType v1{undef, g1};
  PhiInst::BlockListType b1{nil1, get1};
  auto* phi1 = builder->Create<PhiInst>(0, v1, b1);
  builder->Create<EqCondBranchInst>(0, phi1, undef, nil2, get2);

  // nil2:
  builder->SetInsertionPointToEnd(nil2);
  builder->Create<BranchInst>(0, merge2);

  // get2:
  builder->SetInsertionPointToEnd(get2);
  auto* g2 = builder->Create<GetTableConstStringKeyInst>(
      0, phi1, builder->GetLiteralUint32(1), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge2);

  // merge2:
  builder->SetInsertionPointToEnd(merge2);
  PhiInst::ValueListType v2{undef, g2};
  PhiInst::BlockListType b2{nil2, get2};
  auto* phi2 = builder->Create<PhiInst>(0, v2, b2);
  builder->Create<ReturnInst>(0, phi2);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  // The entry guard's true destination should be rewritten to the last
  // trampoline (nil2) to early short-circuit the chain.
  ASSERT_NE(nullptr, br1);
  EXPECT_EQ(br1->GetTrueDest(), nil2);
}

TEST(LEPUSIRInstCombine,
     foldNullishGuardChainUnifiesAllEarlierNilEdgesInLongChain) {
  // Chain length 3:
  //   t1 = (x == undef) ? undef : GetTable(x, k0)
  //   t2 = (t1 == undef) ? undef : GetTable(t1, k1)
  //   t3 = (t2 == undef) ? undef : GetTable(t2, k2)
  // Expect: br1/br2 null edges both jump directly to nil3.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_fold_long";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* nil1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil1");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* nil2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil2");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");
  auto* nil3 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil3");
  auto* get3 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get3");
  auto* merge3 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge3");
  ASSERT_NE(nullptr, entry);
  ASSERT_NE(nullptr, nil1);
  ASSERT_NE(nullptr, get1);
  ASSERT_NE(nullptr, merge1);
  ASSERT_NE(nullptr, nil2);
  ASSERT_NE(nullptr, get2);
  ASSERT_NE(nullptr, merge2);
  ASSERT_NE(nullptr, nil3);
  ASSERT_NE(nullptr, get3);
  ASSERT_NE(nullptr, merge3);

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef, nil1, get1);

  builder->SetInsertionPointToEnd(nil1);
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(get1);
  auto* g1 = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  PhiInst::ValueListType v1{undef, g1};
  PhiInst::BlockListType b1{nil1, get1};
  auto* phi1 = builder->Create<PhiInst>(0, v1, b1);
  auto* br2 = builder->Create<EqCondBranchInst>(0, phi1, undef, nil2, get2);

  builder->SetInsertionPointToEnd(nil2);
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(get2);
  auto* g2 = builder->Create<GetTableConstStringKeyInst>(
      0, phi1, builder->GetLiteralUint32(1), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(merge2);
  PhiInst::ValueListType v2{undef, g2};
  PhiInst::BlockListType b2{nil2, get2};
  auto* phi2 = builder->Create<PhiInst>(0, v2, b2);
  auto* br3 = builder->Create<EqCondBranchInst>(0, phi2, undef, nil3, get3);

  builder->SetInsertionPointToEnd(nil3);
  builder->Create<BranchInst>(0, merge3);

  builder->SetInsertionPointToEnd(get3);
  auto* g3 = builder->Create<GetTableConstStringKeyInst>(
      0, phi2, builder->GetLiteralUint32(2), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge3);

  builder->SetInsertionPointToEnd(merge3);
  PhiInst::ValueListType v3{undef, g3};
  PhiInst::BlockListType b3{nil3, get3};
  auto* phi3 = builder->Create<PhiInst>(0, v3, b3);
  builder->Create<ReturnInst>(0, phi3);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  ASSERT_NE(nullptr, br1);
  ASSERT_NE(nullptr, br2);
  ASSERT_NE(nullptr, br3);
  EXPECT_EQ(br1->GetTrueDest(), nil3);
  EXPECT_EQ(br2->GetTrueDest(), nil3);
  EXPECT_EQ(br3->GetTrueDest(), nil3);
}

TEST(LEPUSIRInstCombine,
     foldNullishGuardChainAcceptsDifferentNullishLiteralForPhiIncoming) {
  // Phi incoming uses a different LoadNullOrUndefinedInst than the compare.
  // FoldNullishGuardChainInPlace should match by nullish type and still
  // unify the first nil edge into the last trampoline.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_two_nullish_literals";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* nil1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil1");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* nil2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil2");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef_cmp =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef_cmp, nil1, get1);

  builder->SetInsertionPointToEnd(nil1);
  // Different nullish literal than `undef_cmp`, but same kind (undef).
  auto* undef_phi =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(get1);
  auto* g1 = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  PhiInst::ValueListType v1{undef_phi, g1};
  PhiInst::BlockListType b1{nil1, get1};
  auto* phi1 = builder->Create<PhiInst>(0, v1, b1);
  builder->Create<EqCondBranchInst>(0, phi1, undef_cmp, nil2, get2);

  builder->SetInsertionPointToEnd(nil2);
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(get2);
  auto* g2 = builder->Create<GetTableConstStringKeyInst>(
      0, phi1, builder->GetLiteralUint32(1), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(merge2);
  PhiInst::ValueListType v2{undef_cmp, g2};
  PhiInst::BlockListType b2{nil2, get2};
  auto* phi2 = builder->Create<PhiInst>(0, v2, b2);
  builder->Create<ReturnInst>(0, phi2);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  ASSERT_NE(nullptr, br1);
  EXPECT_EQ(br1->GetTrueDest(), nil2);
}

TEST(LEPUSIRInstCombine, foldNullishGuardChainNoChangeWhenMergeHasMultiplePhi) {
  // MatchNullishGuardedGetTable requires exactly one Phi at the start of
  // merge. Add an extra Phi to ensure FoldNullishGuardChainInPlace is stable
  // and does not rewrite CFG when the pattern is ambiguous.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_multi_phi_reject";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* nil1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil1");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* nil2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil2");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef, nil1, get1);

  builder->SetInsertionPointToEnd(nil1);
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(get1);
  auto* g1 = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  PhiInst::ValueListType v1{undef, g1};
  PhiInst::BlockListType b1{nil1, get1};
  auto* phi1 = builder->Create<PhiInst>(0, v1, b1);
  // Extra Phi breaks the single-phi assumption.
  auto* phi_extra = builder->Create<PhiInst>(0, v1, b1);
  (void)phi_extra;
  builder->Create<EqCondBranchInst>(0, phi1, undef, nil2, get2);

  builder->SetInsertionPointToEnd(nil2);
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(get2);
  auto* g2 = builder->Create<GetTableConstStringKeyInst>(
      0, phi1, builder->GetLiteralUint32(1), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(merge2);
  PhiInst::ValueListType v2{undef, g2};
  PhiInst::BlockListType b2{nil2, get2};
  auto* phi2 = builder->Create<PhiInst>(0, v2, b2);
  builder->Create<ReturnInst>(0, phi2);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  ASSERT_NE(nullptr, br1);
  EXPECT_EQ(br1->GetTrueDest(), nil1);
}

TEST(LEPUSIRInstCombine,
     foldNullishGuardChainNoChangeWhenNilBlockNotTrampoline) {
  // nil1 block contains an extra instruction (not LoadNullOrUndefined), so it
  // is not a valid trampoline. FoldNullishGuardChainInPlace must not rewrite.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_non_trampoline_reject";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* nil1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil1");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* nil2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil2");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);
  const uint32_t dummy_idx = lepus_func->AddConstNumber(0);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef, nil1, get1);

  builder->SetInsertionPointToEnd(nil1);
  // Extra instruction makes this block non-trampoline for the matcher.
  builder->Create<LoadConstInst>(0, builder->GetLiteralUint32(dummy_idx),
                                 TypeOp::CreateInt64(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(get1);
  auto* g1 = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  PhiInst::ValueListType v1{undef, g1};
  PhiInst::BlockListType b1{nil1, get1};
  auto* phi1 = builder->Create<PhiInst>(0, v1, b1);
  builder->Create<EqCondBranchInst>(0, phi1, undef, nil2, get2);

  builder->SetInsertionPointToEnd(nil2);
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(get2);
  auto* g2 = builder->Create<GetTableConstStringKeyInst>(
      0, phi1, builder->GetLiteralUint32(1), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(merge2);
  PhiInst::ValueListType v2{undef, g2};
  PhiInst::BlockListType b2{nil2, get2};
  auto* phi2 = builder->Create<PhiInst>(0, v2, b2);
  builder->Create<ReturnInst>(0, phi2);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  ASSERT_NE(nullptr, br1);
  EXPECT_EQ(br1->GetTrueDest(), nil1);
}

TEST(LEPUSIRInstCombine, foldNullishGuardChainIsIdempotent) {
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_idempotent";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* nil1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil1");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* nil2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "nil2");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef, nil1, get1);

  builder->SetInsertionPointToEnd(nil1);
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(get1);
  auto* g1 = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  PhiInst::ValueListType v1{undef, g1};
  PhiInst::BlockListType b1{nil1, get1};
  auto* phi1 = builder->Create<PhiInst>(0, v1, b1);
  builder->Create<EqCondBranchInst>(0, phi1, undef, nil2, get2);

  builder->SetInsertionPointToEnd(nil2);
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(get2);
  auto* g2 = builder->Create<GetTableConstStringKeyInst>(
      0, phi1, builder->GetLiteralUint32(1), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(merge2);
  PhiInst::ValueListType v2{undef, g2};
  PhiInst::BlockListType b2{nil2, get2};
  auto* phi2 = builder->Create<PhiInst>(0, v2, b2);
  builder->Create<ReturnInst>(0, phi2);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));
  ASSERT_NE(nullptr, br1);
  EXPECT_EQ(br1->GetTrueDest(), nil2);

  // Run again to validate stability / idempotence.
  ASSERT_TRUE(pass.RunOnFunction(func_op));
  EXPECT_EQ(br1->GetTrueDest(), nil2);
}

TEST(
    LEPUSIRInstCombine,
    foldNullishGuardChainFromMergeRecomputedGetTableInOptionalChainingLowering) {
  // A pattern produced by optional-chaining expansion in generated JS:
  //   if (recv == undef) goto merge; else goto get;
  //   get:   v = GetTable(recv, k); goto merge;
  //   merge: v2 = GetTable(recv, k); if (v2 == undef) ...
  // Canonicalize it into nil trampoline + merge phi, then fold the chain.

  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_merge_recompute";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef, merge1, get1);

  builder->SetInsertionPointToEnd(get1);
  builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  // Merge recomputes the same get (this is what we canonicalize away).
  auto* g1_merge = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  auto* br2 =
      builder->Create<EqCondBranchInst>(0, g1_merge, undef, merge2, get2);

  builder->SetInsertionPointToEnd(get2);
  auto* g2 = builder->Create<GetTableConstStringKeyInst>(
      0, g1_merge, builder->GetLiteralUint32(1),
      TypeOp::CreateAnyType(builder));
  (void)g2;
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(merge2);
  auto* g2_merge = builder->Create<GetTableConstStringKeyInst>(
      0, g1_merge, builder->GetLiteralUint32(1),
      TypeOp::CreateAnyType(builder));
  builder->Create<ReturnInst>(0, g2_merge);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  // Both guards should unify to the same final nil trampoline block.
  ASSERT_NE(nullptr, br1);
  ASSERT_NE(nullptr, br2);
  EXPECT_EQ(br1->GetTrueDest(), br2->GetTrueDest());

  // Merge blocks should start with a phi after canonicalization.
  PhiInst* merge1_phi = nullptr;
  for (auto* inst : merge1->InstRange()) {
    merge1_phi = llvh::dyn_cast<PhiInst>(inst);
    break;
  }
  ASSERT_NE(nullptr, merge1_phi);
  EXPECT_TRUE(br2->GetLeftHandSide() == merge1_phi ||
              br2->GetRightHandSide() == merge1_phi);

  // The unified nil trampoline should branch to merge2.
  auto* nil_tramp = br2->GetTrueDest();
  ASSERT_NE(nullptr, nil_tramp);
  auto* nil_term =
      llvh::dyn_cast_or_null<BranchInst>(nil_tramp->GetTerminator());
  ASSERT_NE(nullptr, nil_term);
  EXPECT_EQ(nil_term->GetBranchDest(), merge2);
}

TEST(
    LEPUSIRInstCombine,
    foldNullishGuardChainFromMergeRecomputedGetTableAllowsDifferentNullishLiteral) {
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_chain_merge_recompute_two_nullish";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");
  auto* get2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get2");
  auto* merge2 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge2");

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef1 =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef1, merge1, get1);

  builder->SetInsertionPointToEnd(get1);
  builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  auto* g1_merge = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  auto* undef2 =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br2 =
      builder->Create<EqCondBranchInst>(0, g1_merge, undef2, merge2, get2);

  builder->SetInsertionPointToEnd(get2);
  builder->Create<GetTableConstStringKeyInst>(0, g1_merge,
                                              builder->GetLiteralUint32(1),
                                              TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge2);

  builder->SetInsertionPointToEnd(merge2);
  auto* g2_merge = builder->Create<GetTableConstStringKeyInst>(
      0, g1_merge, builder->GetLiteralUint32(1),
      TypeOp::CreateAnyType(builder));
  builder->Create<ReturnInst>(0, g2_merge);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  ASSERT_NE(nullptr, br1);
  ASSERT_NE(nullptr, br2);
  EXPECT_EQ(br1->GetTrueDest(), br2->GetTrueDest());
}

TEST(LEPUSIRInstCombine,
     canonicalizeMergeRecomputedGetTableToNullishGuardedPhi) {
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_guard_merge_recompute_canonicalize";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* get1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "get1");
  auto* merge1 = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge1");

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  auto* br1 = builder->Create<EqCondBranchInst>(0, x, undef, merge1, get1);

  builder->SetInsertionPointToEnd(get1);
  builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge1);

  builder->SetInsertionPointToEnd(merge1);
  auto* g1_merge = builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<ReturnInst>(0, g1_merge);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  ASSERT_NE(nullptr, br1);
  // True dest should be redirected to a new nil trampoline (not merge1).
  EXPECT_NE(br1->GetTrueDest(), merge1);
  auto* nil_term =
      llvh::dyn_cast_or_null<BranchInst>(br1->GetTrueDest()->GetTerminator());
  ASSERT_NE(nullptr, nil_term);
  EXPECT_EQ(nil_term->GetBranchDest(), merge1);

  // Merge should start with phi after canonicalization.
  Instruction* first_inst = nullptr;
  for (auto* inst : merge1->InstRange()) {
    first_inst = inst;
    break;
  }
  ASSERT_NE(nullptr, first_inst);
  EXPECT_TRUE(llvh::isa<PhiInst>(first_inst));
}

TEST(LEPUSIRInstCombine, simplifyTrivialEqCondBranchToBranch) {
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_simplify_trivial_eq_br";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* dst = builder->CreateBlock(region, BlockType::BT_INST, {}, "dst");
  ASSERT_NE(nullptr, entry);
  ASSERT_NE(nullptr, dst);

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);
  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  builder->Create<EqCondBranchInst>(0, x, undef, dst, dst);

  builder->SetInsertionPointToEnd(dst);
  builder->Create<ReturnInst>(0, undef);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  auto* term = entry->GetTerminator();
  ASSERT_NE(nullptr, term);
  // Current InstCombine does not canonicalize same-dest conditional branches
  // into BranchInst. Keep verifying it is semantically trivial.
  auto* br = llvh::dyn_cast<EqCondBranchInst>(term);
  ASSERT_NE(nullptr, br);
  EXPECT_EQ(br->GetTrueDest(), dst);
  EXPECT_EQ(br->GetFalseDest(), dst);
}

TEST(LEPUSIRInstCombine, simplifyTrivialNeqCondBranchToBranch) {
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_simplify_trivial_neq_br";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* dst = builder->CreateBlock(region, BlockType::BT_INST, {}, "dst");
  ASSERT_NE(nullptr, entry);
  ASSERT_NE(nullptr, dst);

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);
  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  builder->Create<NeqCondBranchInst>(0, x, undef, dst, dst);

  builder->SetInsertionPointToEnd(dst);
  builder->Create<ReturnInst>(0, undef);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  auto* term = entry->GetTerminator();
  ASSERT_NE(nullptr, term);
  // Current InstCombine does not canonicalize same-dest conditional branches
  // into BranchInst. Keep verifying it is semantically trivial.
  auto* br = llvh::dyn_cast<NeqCondBranchInst>(term);
  ASSERT_NE(nullptr, br);
  EXPECT_EQ(br->GetTrueDest(), dst);
  EXPECT_EQ(br->GetFalseDest(), dst);
}

TEST(LEPUSIRInstCombine, simplifyTrivialCondBranchToBranch) {
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_simplify_trivial_cond_br";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* dst = builder->CreateBlock(region, BlockType::BT_INST, {}, "dst");
  ASSERT_NE(nullptr, entry);
  ASSERT_NE(nullptr, dst);

  const uint32_t recv_idx = lepus_func->AddConstNumber(1);
  builder->SetInsertionPointToEnd(entry);
  auto* cond = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  builder->Create<CondBranchInst>(0, cond, dst, dst);

  builder->SetInsertionPointToEnd(dst);
  auto* undef =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(1));
  builder->Create<ReturnInst>(0, undef);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  auto* term = entry->GetTerminator();
  ASSERT_NE(nullptr, term);
  auto* br = llvh::dyn_cast<BranchInst>(term);
  ASSERT_NE(nullptr, br);
  EXPECT_EQ(br->GetBranchDest(), dst);
}

TEST(LEPUSIRInstCombine, doNotEliminateNullishGuardedGetTableInStrictMode) {
  // Same CFG shape as above, but strict check is enabled. Optional chaining
  // relies on the guard to avoid throwing on nullish receivers.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(true);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_opt_chain_strict";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* b_nil = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_nil");
  auto* b_get = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_get");
  auto* merge = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge");
  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* nil =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(0));
  builder->Create<EqCondBranchInst>(0, x, nil, b_nil, b_get);

  builder->SetInsertionPointToEnd(b_nil);
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(b_get);
  builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(merge);
  auto* get_in_b_get = llvh::cast<GetTableConstStringKeyInst>(b_get->Front());
  PhiInst::ValueListType vals{nil, get_in_b_get};
  PhiInst::BlockListType bbs{b_nil, b_get};
  auto* phi = builder->Create<PhiInst>(0, vals, bbs);
  builder->Create<ReturnInst>(0, phi);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  bool has_phi = false;
  bool has_get_in_merge = false;
  for (auto& bb : *func_op) {
    for (auto* inst : bb.InstRange()) {
      if (llvh::isa<PhiInst>(inst)) has_phi = true;
      if (&bb == merge && llvh::isa<GetTableConstStringKeyInst>(inst)) {
        // Should not materialize a new unconditional get in merge.
        has_get_in_merge = true;
      }
    }
  }
  EXPECT_TRUE(has_phi);
  EXPECT_FALSE(has_get_in_merge);
}

TEST(LEPUSIRInstCombine, doNotEliminateWhenNullishIncomingDoesNotMatchVMMode) {
  // VM mode expects undefined on nullish receiver, but phi incoming is nil.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(true);
  vm_ctx.SetEnableStrictCheck(false);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_opt_chain_mismatch";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* b_nil = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_nil");
  auto* b_get = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_get");
  auto* merge = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge");
  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* nil =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(0));
  builder->Create<EqCondBranchInst>(0, x, nil, b_nil, b_get);

  builder->SetInsertionPointToEnd(b_nil);
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(b_get);
  builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(merge);
  auto* get_in_b_get = llvh::cast<GetTableConstStringKeyInst>(b_get->Front());
  PhiInst::ValueListType vals{nil, get_in_b_get};
  PhiInst::BlockListType bbs{b_nil, b_get};
  auto* phi = builder->Create<PhiInst>(0, vals, bbs);
  builder->Create<ReturnInst>(0, phi);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  bool has_phi = false;
  bool has_get_in_merge = false;
  for (auto& bb : *func_op) {
    for (auto* inst : bb.InstRange()) {
      if (llvh::isa<PhiInst>(inst)) has_phi = true;
      if (&bb == merge && llvh::isa<GetTableConstStringKeyInst>(inst)) {
        has_get_in_merge = true;
      }
    }
  }
  EXPECT_TRUE(has_phi);
  EXPECT_FALSE(has_get_in_merge);
}

TEST(LEPUSIRInstCombine, eliminateNullishGuardedGetTableWithPropInGetBlock) {
  // CFG for:
  //   v = (x == nil) ? nil : GetTable(x, prop)
  // where `prop` is loaded in the get-block.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(false);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_opt_chain_prop_in_get_block";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* b_nil = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_nil");
  auto* b_get = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_get");
  auto* merge = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge");
  const uint32_t recv_idx = lepus_func->AddConstNumber(1);
  const uint32_t prop_idx = lepus_func->AddConstNumber(0);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* nil =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(0));
  builder->Create<EqCondBranchInst>(0, x, nil, b_nil, b_get);

  builder->SetInsertionPointToEnd(b_nil);
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(b_get);
  auto* prop = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(prop_idx), TypeOp::CreateInt64(builder));
  auto* get_in_b_get = builder->Create<GetTableInst>(0, x, prop);
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(merge);
  PhiInst::ValueListType vals{nil, get_in_b_get};
  PhiInst::BlockListType bbs{b_nil, b_get};
  auto* phi = builder->Create<PhiInst>(0, vals, bbs);
  builder->Create<ReturnInst>(0, phi);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  bool has_phi = false;
  bool has_get_in_merge = false;
  for (auto& bb : *func_op) {
    for (auto* inst : bb.InstRange()) {
      if (llvh::isa<PhiInst>(inst)) has_phi = true;
      if (&bb == merge && llvh::isa<GetTableInst>(inst))
        has_get_in_merge = true;
    }
  }
  // Current InstCombine only rewrites chained nullish-guard CFGs (optional
  // chaining) by unifying trampoline blocks; it does not eliminate a single
  // nullish guard by hoisting GetTable into merge.
  EXPECT_TRUE(has_phi);
  EXPECT_FALSE(has_get_in_merge);
}

TEST(LEPUSIRInstCombine,
     eliminateNullishGuardedGetTableWithDifferentNullishLiterals) {
  // Compare uses a nullish literal different from the Phi incoming literal.
  // The peephole should still match by nullish type.
  VMContext vm_ctx;
  vm_ctx.SetNullPropAsUndef(false);
  vm_ctx.SetEnableStrictCheck(false);

  IRContext ir_ctx(&vm_ctx);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx.SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx.GetMainMod();
  ASSERT_NE(nullptr, mod);
  OpBuilder* builder = ir_ctx.GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string func_name = "test_opt_chain_two_nullish";
  auto* func_op = builder->Create<FuncOp>(0, func_name);
  auto lepus_func = lepus::Function::Create();
  func_op->Init(lepus_func);
  auto* region = func_op->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, {}, "entry");
  auto* b_nil = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_nil");
  auto* b_get = builder->CreateBlock(region, BlockType::BT_INST, {}, "b_get");
  auto* merge = builder->CreateBlock(region, BlockType::BT_INST, {}, "merge");
  const uint32_t recv_idx = lepus_func->AddConstNumber(1);

  builder->SetInsertionPointToEnd(entry);
  auto* x = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));
  auto* nil_for_cmp =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(0));
  builder->Create<EqCondBranchInst>(0, x, nil_for_cmp, b_nil, b_get);

  builder->SetInsertionPointToEnd(b_nil);
  auto* nil_for_phi =
      builder->Create<LoadNullOrUndefinedInst>(0, builder->GetLiteralInt8(0));
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(b_get);
  builder->Create<GetTableConstStringKeyInst>(
      0, x, builder->GetLiteralUint32(0), TypeOp::CreateAnyType(builder));
  builder->Create<BranchInst>(0, merge);

  builder->SetInsertionPointToEnd(merge);
  auto* get_in_b_get = llvh::cast<GetTableConstStringKeyInst>(b_get->Front());
  PhiInst::ValueListType vals{nil_for_phi, get_in_b_get};
  PhiInst::BlockListType bbs{b_nil, b_get};
  auto* phi = builder->Create<PhiInst>(0, vals, bbs);
  builder->Create<ReturnInst>(0, phi);

  InstCombinePass pass(&ir_ctx);
  ASSERT_TRUE(pass.RunOnFunction(func_op));

  bool has_phi = false;
  bool has_get_in_merge = false;
  for (auto& bb : *func_op) {
    for (auto* inst : bb.InstRange()) {
      if (llvh::isa<PhiInst>(inst)) has_phi = true;
      if (&bb == merge && llvh::isa<GetTableConstStringKeyInst>(inst)) {
        has_get_in_merge = true;
      }
    }
  }
  // InstCombine keeps the guard+phi form; it only matches/tracks nullish
  // trampolines for chained patterns.
  EXPECT_TRUE(has_phi);
  EXPECT_FALSE(has_get_in_merge);
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

TEST_F(LEPUSIRTestInstCombineOp,
       testCombineSetTableConstStringKeyObjAnyButFreshNewTable) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_set_table_const_string_key_obj_any";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Build: obj is a fresh NewTableInst but its IR type is widened to `any`.
  //        key = LoadConst("foo"); obj[key] = 1
  auto* obj = builder.Create<NewTableInst>(0);
  ASSERT_TRUE(obj->GetType()->IsTableType());
  obj->SetType(TypeOp::CreateAnyType(&builder));
  ASSERT_TRUE(obj->GetType()->IsAnyType());

  uint32_t foo_idx = lepus_func->AddConstString("foo");
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));

  auto* store = builder.GetLiteralInt32(1);
  store->SetType(TypeOp::CreateInt32(&builder));
  auto* set_table = builder.Create<SetTableInst>(0, obj, key, store);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  // InstCombine is followed by DCE in the real optimization pipeline.
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

TEST_F(LEPUSIRTestInstCombineOp,
       testCombineSetTableConstStringKeyRejectsAnyNonFreshReceiver) {
  ASSERT_NE(nullptr, mod);

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto lepus_func = lepus::Function::Create();
  std::string func_name = "test_set_table_const_string_key_reject_any";
  auto* func_op = builder.Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func_op);
  func_op->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Receiver is `any` coming from a toplevel read (not provably a fresh table).
  auto* obj = builder.Create<GetToplevelVarInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  ASSERT_TRUE(obj->GetType()->IsAnyType());

  uint32_t foo_idx = lepus_func->AddConstString("foo");
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  auto* store = builder.GetLiteralInt32(1);
  store->SetType(TypeOp::CreateInt32(&builder));
  auto* set_table = builder.Create<SetTableInst>(0, obj, key, store);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  // Run DCE but SetTableInst should remain (write side effect).
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  bool found_combined = false;
  bool found_original = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == set_table) found_original = true;
    if (llvh::isa<SetTableConstStringKeyInst>(inst)) {
      found_combined = true;
    }
  }

  EXPECT_FALSE(found_combined);
  EXPECT_TRUE(found_original);
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
  pass.RunOnFunction(func_op);

  // DCE should remove the now-dead UnaryNotInst.
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

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
