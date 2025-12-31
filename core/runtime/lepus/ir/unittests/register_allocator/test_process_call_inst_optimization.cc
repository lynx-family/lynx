// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRProcessCallInstOptimizationTest : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }

  FuncOp* createTestFunction(std::string name) {
    auto builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

    auto* func_op = builder->Create<FuncOp>(0, name);
    EXPECT_NE(nullptr, func_op);

    auto region = builder->CreateRegion(func_op);
    EXPECT_NE(nullptr, region);

    auto block = builder->CreateBlock(region, BlockType::BT_INST, {});
    EXPECT_NE(nullptr, block);

    return func_op;
  }
};

// Expose protected helpers for unit testing.
class TestableRegisterAllocator : public RegisterAllocator {
 public:
  explicit TestableRegisterAllocator(FuncOp* func) : RegisterAllocator(func) {}
  using RegisterAllocator::GetTargetRegForCallFunction;
  void setPrefixRegForTest(unsigned prefix) { prefix_reg_ = prefix; }
};

TEST_F(LEPUSIRProcessCallInstOptimizationTest, ReuseExistingMovForCallFunc) {
  auto* func = createTestFunction("test_reuse_mov");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Setup:
  // src (reg 0)
  // v1 (reg 1)
  // mov (reg 0 -> reg 2)
  // call (func = mov)
  //
  // We want `processCallInst` to put `func` into a register higher than any
  // live register. Assume `v1` is live at call. Max live reg = 1. Target reg
  // = 2. But if we force `mov` to be initially at reg 0 (which is <= max live
  // reg 1), `processCallInst` should detect it needs to move. Instead of
  // creating a NEW mov, it should update `mov` to reg 2.

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));

  // Make `v1` live across the call to force `local_max_reg` to be at least
  // reg(v1).
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(200),
                                            TypeOp::CreateInt32(builder));

  auto* mov = builder->Create<MovInst>(0, src);

  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);

  auto* ret = builder->Create<ReturnInst>(0, v1);  // use v1

  TestableRegisterAllocator ra(func);

  // Fake allocation
  ra.InsertRegister(src, Register(0));
  ra.InsertRegister(v1, Register(5));   // v1 is at 5. So max live at call is 5.
  ra.InsertRegister(mov, Register(0));  // mov is at 0. 0 <= 5. Needs update.
  ra.InsertRegister(ret, Register(6));

  // Initialize intervals (needed for GetTargetRegForCallFunction)
  // We need to properly setup instruction numbers and intervals manually since
  // we skipped `Allocate()`.
  ra.GetInstructionNumber(src);
  ra.GetInstructionNumber(v1);
  ra.GetInstructionNumber(mov);
  ra.GetInstructionNumber(call);
  ra.GetInstructionNumber(ret);

  // Set intervals.
  // src: [1, 2) (dead after mov?) actually src is used by mov.
  ra.GetInstructionInterval(src).Add(Segment(1, 4));  // used by mov at 3?
  // v1: live until ret.
  unsigned call_idx = ra.GetInstructionNumber(call);
  unsigned ret_idx = ra.GetInstructionNumber(ret);
  // Interval for v1 must cover call_idx+1 to be seen as live by GetMaxLiveReg.
  ra.GetInstructionInterval(v1).Add(Segment(2, ret_idx + 1));

  // mov interval needs to be set.
  ra.GetInstructionInterval(mov).Add(Segment(3, call_idx + 1));

  // Verify precondition
  EXPECT_EQ(ra.GetRegister(mov).GetIndex(), 0u);

  // Execute ProcessCallInst
  ra.ProcessCallInst(builder, call);

  // Verification

  // 1. mov register should be updated to max_live + 1 = 6.
  EXPECT_EQ(ra.GetRegister(mov).GetIndex(), 6u);

  // 2. call function should still be `mov`.
  EXPECT_EQ(call->GetFunction(), mov);

  // 3. NO new MovInst should be created.
  // Count MovInsts in block.
  int mov_count = 0;
  for (auto& inst : *block) {
    if (llvh::isa<MovInst>(&inst)) mov_count++;
  }
  EXPECT_EQ(mov_count, 1);

  // 4. mov should be marked as CallFuncMov
  EXPECT_TRUE(mov->IsCallFuncMov());
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest, DoNotReuseMovIfMultipleUsers) {
  auto* func = createTestFunction("test_no_reuse_multi_user");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  [[maybe_unused]] auto* v1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(200), TypeOp::CreateInt32(builder));

  auto* mov = builder->Create<MovInst>(0, src);

  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);

  // Another use of mov
  auto* ret = builder->Create<ReturnInst>(0, mov);
  // also use v1 to keep it live

  TestableRegisterAllocator ra(func);
  ra.InsertRegister(src, Register(0));
  ra.InsertRegister(v1, Register(5));
  ra.InsertRegister(mov, Register(0));
  ra.InsertRegister(ret, Register(6));

  ra.GetInstructionNumber(src);
  ra.GetInstructionNumber(v1);
  ra.GetInstructionNumber(mov);
  ra.GetInstructionNumber(call);
  ra.GetInstructionNumber(ret);

  unsigned call_idx = ra.GetInstructionNumber(call);
  ra.GetInstructionInterval(v1).Add(
      Segment(2, call_idx + 2));  // live across call
  ra.GetInstructionInterval(mov).Add(Segment(3, call_idx + 2));  // used by ret

  ra.ProcessCallInst(builder, call);

  // Should NOT reuse mov because it has multiple users (call and ret)

  // 1. mov register should stay 0? No, ProcessCallInst doesn't touch old reg if
  // not reusing.
  EXPECT_EQ(ra.GetRegister(mov).GetIndex(), 0u);

  // 2. call function should be a NEW mov.
  EXPECT_NE(call->GetFunction(), mov);
  auto* new_mov = llvh::dyn_cast<MovInst>(call->GetFunction());
  ASSERT_NE(new_mov, nullptr);
  EXPECT_EQ(new_mov->GetSingleOperand(),
            mov);  // It wraps the original func (which is `mov`)

  // 3. New mov register should be 6
  EXPECT_EQ(ra.GetRegister(new_mov).GetIndex(), 6u);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       DoNotReuseMovIfNotImmediatelyBefore) {
  auto* func = createTestFunction("test_no_reuse_gap");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(200),
                                            TypeOp::CreateInt32(builder));

  auto* mov = builder->Create<MovInst>(0, src);

  // Insert gap instruction
  [[maybe_unused]] auto* gap = builder->Create<BinaryOperatorInst>(
      0, src, src, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(builder));

  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);

  auto* ret = builder->Create<ReturnInst>(0, v1);

  TestableRegisterAllocator ra(func);
  ra.InsertRegister(src, Register(0));
  ra.InsertRegister(v1, Register(5));
  ra.InsertRegister(mov, Register(0));
  ra.InsertRegister(gap, Register(1));
  ra.InsertRegister(ret, Register(6));

  ra.GetInstructionNumber(src);
  ra.GetInstructionNumber(v1);
  ra.GetInstructionNumber(mov);
  ra.GetInstructionNumber(gap);
  ra.GetInstructionNumber(call);
  ra.GetInstructionNumber(ret);

  unsigned call_idx = ra.GetInstructionNumber(call);
  ra.GetInstructionInterval(v1).Add(Segment(2, call_idx + 2));

  ra.ProcessCallInst(builder, call);

  // Should NOT reuse mov because of gap
  EXPECT_NE(call->GetFunction(), mov);
  auto* new_mov = llvh::dyn_cast<MovInst>(call->GetFunction());
  ASSERT_NE(new_mov, nullptr);
  EXPECT_EQ(new_mov->GetSingleOperand(), mov);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       CallFuncMovIntervalDoesNotOverlapPreviousCall) {
  auto* func = createTestFunction("test_call_func_mov_interval_no_overlap");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Two adjacent calls:
  //   call1
  //   call2
  // ProcessCallInst may insert/adjust a call-func MOV for call2.
  // That MOV must NOT be considered live at call1's call-site.

  auto* callee1 =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* callee2 =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(1));

  // Live value across call2 to force callee2 to be moved.
  auto* live = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                              TypeOp::CreateInt32(builder));

  ArgList args;
  auto* call1 = builder->Create<CallInst>(0, callee1, args);
  auto* call2 = builder->Create<CallInst>(0, callee2, args);
  [[maybe_unused]] auto* ret = builder->Create<ReturnInst>(0, live);

  TestableRegisterAllocator ra(func);
  ra.InsertRegister(callee1, Register(0));
  ra.InsertRegister(callee2, Register(0));
  ra.InsertRegister(live, Register(5));
  ra.InsertRegister(call1, Register(1));
  ra.InsertRegister(call2, Register(2));
  ra.InsertRegister(ret, Register(3));

  ra.GetInstructionNumber(callee1);
  ra.GetInstructionNumber(callee2);
  ra.GetInstructionNumber(live);
  ra.GetInstructionNumber(call1);
  ra.GetInstructionNumber(call2);
  ra.GetInstructionNumber(ret);

  const unsigned call1_idx = ra.GetInstructionNumber(call1);
  [[maybe_unused]] const unsigned call2_idx = ra.GetInstructionNumber(call2);
  const unsigned ret_idx = ra.GetInstructionNumber(ret);

  // Make `live` span across call2 (it is returned after call2).
  ra.GetInstructionInterval(live).Add(Segment(1, ret_idx + 1));

  ra.ProcessCallInst(builder, call2);

  auto* call2_func = call2->GetFunction();
  auto* call2_mov = llvh::dyn_cast<MovInst>(call2_func);
  ASSERT_NE(call2_mov, nullptr);
  ASSERT_TRUE(call2_mov->IsCallFuncMov());

  Segment call1_point(call1_idx + 1, call1_idx + 2);
  EXPECT_FALSE(ra.GetInstructionInterval(call2_mov).Intersects(call1_point));
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       GetTargetRegForCallFunctionReturnsZeroWhenOnlyExcludedCallee) {
  auto* func = createTestFunction("test_get_target_reg_only_excluded_callee");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  ArgList args;
  auto* call = builder->Create<CallInst>(0, callee, args);
  builder->Create<ReturnInst>(0, callee);

  TestableRegisterAllocator ra(func);
  ra.InsertRegister(callee, Register(10));

  ra.GetInstructionNumber(callee);
  ra.GetInstructionNumber(call);

  // Excluding the current callee + no args + no other allocated live values =>
  // helper should return 0 (no need to move callee).
  EXPECT_EQ(ra.GetTargetRegForCallFunction(call, callee), 0u);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       GetTargetRegForCallFunctionReturnsMaxLivePlusOne) {
  auto* func = createTestFunction("test_get_target_reg_max_live_plus_one");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* live = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                              TypeOp::CreateInt32(builder));
  ArgList args;
  auto* call = builder->Create<CallInst>(0, callee, args);
  [[maybe_unused]] auto* ret = builder->Create<ReturnInst>(0, live);

  TestableRegisterAllocator ra(func);
  ra.InsertRegister(callee, Register(0));
  ra.InsertRegister(live, Register(5));

  ra.GetInstructionNumber(callee);
  ra.GetInstructionNumber(live);
  ra.GetInstructionNumber(call);
  ra.GetInstructionNumber(ret);

  const unsigned ret_idx = ra.GetInstructionNumber(ret);
  // Make `live` span across the call-site point (call_idx + 1).
  ra.GetInstructionInterval(live).Add(Segment(1, ret_idx + 1));

  EXPECT_EQ(ra.GetTargetRegForCallFunction(call, callee), 6u);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       GetTargetRegForCallFunctionRespectsParamPrefix) {
  auto* func = createTestFunction("test_get_target_reg_param_prefix");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                             TypeOp::CreateInt32(builder));

  ArgList args;
  args.push_back(arg);
  auto* call = builder->Create<CallInst>(0, callee, args);
  builder->Create<ReturnInst>(0, arg);

  TestableRegisterAllocator ra(func);
  // Simulate a function with parameters occupying [0..2].
  ra.setPrefixRegForTest(3);

  // callee is already far above the prefix.
  ra.InsertRegister(callee, Register(10));
  ra.InsertRegister(arg, Register(0));

  ra.GetInstructionNumber(callee);
  ra.GetInstructionNumber(arg);
  ra.GetInstructionNumber(call);

  // Callee is already above the prefix, no need to force the target to prefix.
  // The target is computed from the (non-callee) operands/live set.
  EXPECT_EQ(ra.GetTargetRegForCallFunction(call, callee), 1u);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       GetTargetRegForCallFunctionMovesCalleeAboveParamPrefix) {
  auto* func =
      createTestFunction("test_get_target_reg_move_above_param_prefix");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                             TypeOp::CreateInt32(builder));

  ArgList args;
  args.push_back(arg);
  auto* call = builder->Create<CallInst>(0, callee, args);
  builder->Create<ReturnInst>(0, arg);

  TestableRegisterAllocator ra(func);
  ra.setPrefixRegForTest(3);

  // Place callee inside the prefix. This would make `a+1..a+argc` overlap
  // parameter registers unless we move callee up.
  ra.InsertRegister(callee, Register(0));
  ra.InsertRegister(arg, Register(1));

  ra.GetInstructionNumber(callee);
  ra.GetInstructionNumber(arg);
  ra.GetInstructionNumber(call);

  EXPECT_EQ(ra.GetTargetRegForCallFunction(call, callee), 3u);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       GetTargetRegForCallFunctionDoesNotForceWhenFuncIsPrefixMinusOne) {
  auto* func =
      createTestFunction("test_get_target_reg_no_force_prefix_minus_one");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                             TypeOp::CreateInt32(builder));

  ArgList args;
  args.push_back(arg);
  auto* call = builder->Create<CallInst>(0, callee, args);
  builder->Create<ReturnInst>(0, arg);

  TestableRegisterAllocator ra(func);
  ra.setPrefixRegForTest(3);

  // func_reg == prefix_reg_ - 1, so (a+1) starts at prefix_reg_ and won't
  // clobber prefix values.
  ra.InsertRegister(callee, Register(2));
  ra.InsertRegister(arg, Register(0));

  ra.GetInstructionNumber(callee);
  ra.GetInstructionNumber(arg);
  ra.GetInstructionNumber(call);

  // Should not be forced to >= prefix.
  EXPECT_EQ(ra.GetTargetRegForCallFunction(call, callee), 1u);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       ProcessCallInstDoesNotMoveWhenFuncRegEqualsPrefix) {
  auto* func = createTestFunction("test_process_call_inst_func_eq_prefix");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                             TypeOp::CreateInt32(builder));

  ArgList args;
  args.push_back(arg);
  auto* call = builder->Create<CallInst>(0, callee, args);
  builder->Create<ReturnInst>(0, arg);

  TestableRegisterAllocator ra(func);
  ra.setPrefixRegForTest(3);

  // callee already at prefix boundary.
  ra.InsertRegister(callee, Register(3));
  ra.InsertRegister(arg, Register(0));

  ra.GetInstructionNumber(callee);
  ra.GetInstructionNumber(arg);
  ra.GetInstructionNumber(call);

  // Should not insert/replace call function since func_reg == target_reg.
  ra.ProcessCallInst(builder, call);
  EXPECT_EQ(call->GetFunction(), callee);
  EXPECT_EQ(ra.GetRegister(callee).GetIndex(), 3u);
}

TEST_F(LEPUSIRProcessCallInstOptimizationTest,
       ProcessCallInstMovesWhenFuncIsInsideParamPrefix) {
  auto* func = createTestFunction("test_process_call_inst_move_from_prefix");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                             TypeOp::CreateInt32(builder));

  ArgList args;
  args.push_back(arg);
  auto* call = builder->Create<CallInst>(0, callee, args);
  builder->Create<ReturnInst>(0, arg);

  TestableRegisterAllocator ra(func);
  ra.setPrefixRegForTest(3);

  ra.InsertRegister(callee, Register(0));
  ra.InsertRegister(arg, Register(1));

  ra.GetInstructionNumber(callee);
  ra.GetInstructionNumber(arg);
  ra.GetInstructionNumber(call);

  ra.ProcessCallInst(builder, call);

  // Call should now use a MovInst as its function operand.
  auto* moved = llvh::dyn_cast<MovInst>(call->GetFunction());
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->GetSingleOperand(), callee);
  EXPECT_EQ(ra.GetRegister(moved).GetIndex(), 3u);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
