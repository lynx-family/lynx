// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRCallRegisterAllocTest : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }

  virtual void TearDown(void) {}

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

  RegisterAllocator* runRegisterAllocation(FuncOp* func) {
    auto* ra = new RegisterAllocator(func);
    PostOrderAnalysis po(func);
    llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
    ra->Preallocate();
    ra->Allocate(order);
    return ra;
  }

  // Compute the maximum register index that is live at the call-site,
  // excluding the call's function operand.
  unsigned GetMaxLiveRegAtCallExcludingCallFunc(RegisterAllocator* ra,
                                                CallInst* call_inst) {
    unsigned call_idx = ra->GetInstructionNumber(call_inst);
    Segment call_point(call_idx + 1, call_idx + 2);

    Value* call_func = call_inst->GetFunction();

    unsigned max_live = 0;
    for (auto& pair : ra->GetAllocatedMap()) {
      Value* v = pair.first;
      Register r = pair.second;
      if (!r.IsValid()) continue;
      if (v == call_func) continue;

      auto* inst = llvh::dyn_cast<Instruction>(v);
      if (!inst) continue;
      if (!ra->HasInstructionNumber(inst)) continue;
      if (inst == call_inst) continue;  // exclude call dst

      Interval& ivl = ra->GetInstructionInterval(inst);
      if (ivl.Intersects(call_point)) {
        max_live = std::max(max_live, r.GetIndex());
      }
    }

    // Conservatively include call operands (arguments) that are allocated.
    for (int i = 0, e = call_inst->GetNumOperands(); i < e; ++i) {
      Value* op = call_inst->GetOperand(i);
      if (op == call_func) continue;
      if (!ra->IsAllocated(op)) continue;
      auto* inst = llvh::dyn_cast<Instruction>(op);
      if (inst && ra->HasInstructionNumber(inst)) {
        if (!ra->GetInstructionInterval(inst).Intersects(call_point)) {
          continue;
        }
      }
      max_live = std::max(max_live, ra->GetRegister(op).GetIndex());
    }

    return max_live;
  }
};

TEST_F(LEPUSIRCallRegisterAllocTest, PreallocatedParametersAreFixReg) {
  auto builder = ir_ctx->GetOpBuilder();

  // Create FuncOp via `init()` so it owns exactly one region.
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string func_name = "test_param_prealloc_fix_reg";
  auto* func = builder->Create<FuncOp>(0, func_name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  auto* region = func->GetSingleRegion();
  auto* block = builder->CreateBlock(region, BlockType::BT_INST, {});
  builder->SetInsertionPointToEnd(block);

  // Create 2 parameters. RegisterAllocator::Preallocate() assigns fixed
  // registers to parameters first.
  auto* p0 = func->CreateParam(0);
  auto* p1 = func->CreateParam(1);

  // Use parameters so the function has at least one instruction.
  auto* add = builder->Create<BinaryOperatorInst>(
      0, p0, p1, ValueKind::BinaryAddInstKind, TypeOp::CreateAnyType(builder));
  builder->Create<ReturnInst>(0, add);

  auto* ra = runRegisterAllocation(func);
  ASSERT_TRUE(ra->IsAllocated(p0));
  ASSERT_TRUE(ra->IsAllocated(p1));

  EXPECT_TRUE(p0->IsFixReg());
  EXPECT_TRUE(p1->IsFixReg());

  // Param registers should be in the prefix range starting from 0.
  EXPECT_EQ(ra->GetRegister(p0).GetIndex(), 0u);
  EXPECT_EQ(ra->GetRegister(p1).GetIndex(), 1u);

  delete ra;
}

// Test 1: Ensure call function MOV does not clobber other live values.
TEST_F(LEPUSIRCallRegisterAllocTest, CallInstWithMultipleLiveVariables) {
  auto* func = createTestFunction("test_call_max_reg");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Create several live variables
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                            TypeOp::CreateInt32(builder));
  auto* v2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                            TypeOp::CreateInt32(builder));
  auto* v3 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(3),
                                            TypeOp::CreateInt32(builder));

  // 2. Create a function to be called
  auto* target_func = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(100), TypeOp::CreateInt32(builder));

  // 3. Create CallInst
  // TypeOp_Call, SrcReg(func), UInt8(argc+1), DstReg(result)
  ArgList args;
  auto* call = builder->Create<CallInst>(0, target_func, args);

  // 4. Use v1, v2, v3 after CallInst to keep them live across the call
  auto* sum1 = builder->Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(builder));
  auto* sum2 = builder->Create<BinaryOperatorInst>(
      0, sum1, v3, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, sum2);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify function register in CallInst
  // The original target_func might have been moved
  Value* actual_func = call->GetFunction();
  ASSERT_TRUE(ra->IsAllocated(actual_func));
  Register func_reg = ra->GetRegister(actual_func);

  // Call function register must be the maximum live register index at this
  // call-site, otherwise VM's argument materialization may clobber live values.
  EXPECT_GE(func_reg.GetIndex(),
            GetMaxLiveRegAtCallExcludingCallFunc(ra, call));

  // Get registers of other live variables
  Register r1 = ra->GetRegister(v1);
  Register r2 = ra->GetRegister(v2);
  Register r3 = ra->GetRegister(v3);

  // The function register must not overlap with other values that are live
  // across the call.
  EXPECT_NE(func_reg.GetIndex(), r1.GetIndex());
  EXPECT_NE(func_reg.GetIndex(), r2.GetIndex());
  EXPECT_NE(func_reg.GetIndex(), r3.GetIndex());

  delete ra;
}

// Test 2: Nested CallInst
TEST_F(LEPUSIRCallRegisterAllocTest, NestedCallInst) {
  auto* func = createTestFunction("test_nested_call");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* f1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(101),
                                            TypeOp::CreateInt32(builder));
  auto* f2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(102),
                                            TypeOp::CreateInt32(builder));

  // call1 = f1()
  ArgList args1;
  auto* call1 = builder->Create<CallInst>(0, f1, args1);
  // call2 = f2(call1)
  ArgList args2;
  args2.push_back(call1);
  auto* call2 = builder->Create<CallInst>(0, f2, args2);

  builder->Create<ReturnInst>(0, call2);

  auto* ra = runRegisterAllocation(func);

  Register r_f1 = ra->GetRegister(call1->GetFunction());
  Register r_f2 = ra->GetRegister(call2->GetFunction());

  EXPECT_TRUE(ra->IsAllocated(call1->GetFunction()));
  EXPECT_TRUE(ra->IsAllocated(call2->GetFunction()));

  // For each call, the function reg should be the maximum live reg at that
  // call-site.
  EXPECT_GE(r_f1.GetIndex(), GetMaxLiveRegAtCallExcludingCallFunc(ra, call1));
  EXPECT_GE(r_f2.GetIndex(), GetMaxLiveRegAtCallExcludingCallFunc(ra, call2));

  delete ra;
}

// Test 3: CallInst with arguments should not clobber argument regs.
TEST_F(LEPUSIRCallRegisterAllocTest, CallWithArguments) {
  auto* func = createTestFunction("test_call_args");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                            TypeOp::CreateInt32(builder));
  auto* target_func = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(100), TypeOp::CreateInt32(builder));

  // call = target_func(v1)
  ArgList args;
  args.push_back(v1);
  auto* call = builder->Create<CallInst>(0, target_func, args);

  // keep v1 alive after call
  builder->Create<BinaryOperatorInst>(0, v1, call, ValueKind::BinaryAddInstKind,
                                      TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, call);

  auto* ra = runRegisterAllocation(func);

  Register r_func = ra->GetRegister(call->GetFunction());
  Register r_v1 = ra->GetRegister(v1);

  EXPECT_GE(r_func.GetIndex(), GetMaxLiveRegAtCallExcludingCallFunc(ra, call));

  EXPECT_NE(r_func.GetIndex(), r_v1.GetIndex());

  delete ra;
}

// Test 4: CallInst where function is already in the highest register
TEST_F(LEPUSIRCallRegisterAllocTest, CallAlreadyInMaxRegister) {
  auto* func = createTestFunction("test_call_already_max");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Create a function first (lower register potentially)
  auto* target_func = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(100), TypeOp::CreateInt32(builder));

  // call = target_func()
  ArgList args;
  auto* call = builder->Create<CallInst>(0, target_func, args);

  builder->Create<ReturnInst>(0, call);

  auto* ra = runRegisterAllocation(func);

  Register r_func = ra->GetRegister(call->GetFunction());
  // In this simple case, target_func should be at a valid register,
  // and since there are no other live variables, it might already be the max.

  EXPECT_TRUE(ra->IsAllocated(call->GetFunction()));
  EXPECT_GE(r_func.GetIndex(), GetMaxLiveRegAtCallExcludingCallFunc(ra, call));

  delete ra;
}

// Test 5: Complex nested calls with overlapping live variables.
TEST_F(LEPUSIRCallRegisterAllocTest, ComplexNestedCallsWithLiveVariables) {
  auto* func = createTestFunction("test_complex_nested_call");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Live variables that span across both calls
  auto* v_global = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(10), TypeOp::CreateInt32(builder));

  // 2. First call
  auto* f1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(101),
                                            TypeOp::CreateInt32(builder));
  ArgList args1;
  auto* call1 = builder->Create<CallInst>(0, f1, args1);

  // 3. Variable live between calls
  auto* v_mid = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(20),
                                               TypeOp::CreateInt32(builder));

  // 4. Second call
  auto* f2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(102),
                                            TypeOp::CreateInt32(builder));
  ArgList args2;
  args2.push_back(call1);
  auto* call2 = builder->Create<CallInst>(0, f2, args2);

  // 5. Use all variables at the end
  auto* sum1 = builder->Create<BinaryOperatorInst>(
      0, v_global, v_mid, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  auto* sum2 = builder->Create<BinaryOperatorInst>(
      0, sum1, call2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, sum2);

  auto* ra = runRegisterAllocation(func);

  Register reg_global = ra->GetRegister(v_global);
  Register reg_mid = ra->GetRegister(v_mid);
  Register reg_f1 = ra->GetRegister(call1->GetFunction());
  Register reg_f2 = ra->GetRegister(call2->GetFunction());

  EXPECT_GE(reg_f1.GetIndex(), GetMaxLiveRegAtCallExcludingCallFunc(ra, call1));
  EXPECT_GE(reg_f2.GetIndex(), GetMaxLiveRegAtCallExcludingCallFunc(ra, call2));

  // Function register must not overlap with values that are live across calls.
  EXPECT_NE(reg_f1.GetIndex(), reg_global.GetIndex());
  EXPECT_NE(reg_f2.GetIndex(), reg_global.GetIndex());
  EXPECT_NE(reg_f2.GetIndex(), reg_mid.GetIndex());

  delete ra;
}

// Test 6: Prefer reusing a small set of registers for many call func MOVs.
// The goal is to avoid allocating a fresh high register per call, which can
// significantly inflate the max register usage in real-world scripts.
TEST_F(LEPUSIRCallRegisterAllocTest,
       CallFuncMovKeepsRegisterUsageBoundedAcrossManyCalls) {
  auto* func = createTestFunction("test_call_func_mov_many_calls");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // A chain of calls with no arguments.
  auto* f = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(201),
                                           TypeOp::CreateInt32(builder));
  constexpr int kCallCount = 32;
  CallInst* last_call = nullptr;
  for (int i = 0; i < kCallCount; ++i) {
    ArgList args;
    last_call = builder->Create<CallInst>(0, f, args);
  }
  builder->Create<ReturnInst>(0, last_call);

  auto* ra = runRegisterAllocation(func);

  // Count the max register index used by any Value. This is the effective
  // register-file size needed by the generated bytecode.
  unsigned max_reg = 0;
  for (auto& pair : ra->GetAllocatedMap()) {
    if (!pair.second.IsValid()) continue;
    max_reg = std::max(max_reg, pair.second.GetIndex());
  }

  // Heuristic upper bound: even with many calls, we should not keep growing
  // the register file linearly.
  EXPECT_LT(max_reg, 16u);

  delete ra;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
