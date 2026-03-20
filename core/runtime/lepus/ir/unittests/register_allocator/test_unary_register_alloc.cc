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

class LEPUSIRUnaryRegisterAllocTest : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }

  virtual void TearDown(void) {}

  // Helper function to create a function with a basic block
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

  // Helper function to run register allocation on a function
  RegisterAllocator* runRegisterAllocation(FuncOp* func) {
    auto* ra = new RegisterAllocator(func);
    PostOrderAnalysis po(func);
    llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
    ra->Preallocate();
    ra->Allocate(order);
    return ra;
  }
};

// Test 1: Simple unary operator with single use
TEST_F(LEPUSIRUnaryRegisterAllocTest, SimpleUnaryOperatorSingleUse) {
  auto* func = createTestFunction("test_simple_unary");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();

  builder->SetInsertionPointToEnd(block);

  // Create: val = LoadConst(42)
  auto* val = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                             TypeOp::CreateInt32(builder));

  // Create: result = -val (UnaryNeg)
  auto* neg_inst =
      builder->Create<UnaryOperatorInst>(0, val, ValueKind::UnaryNegInstKind);

  // Create: return result
  builder->Create<ReturnInst>(0, neg_inst);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify: both values are allocated (src/dst may or may not share a
  // register).
  EXPECT_TRUE(ra->IsAllocated(val));
  EXPECT_TRUE(ra->IsAllocated(neg_inst));

  auto src_reg = ra->GetRegister(val);
  auto dst_reg = ra->GetRegister(neg_inst);

  EXPECT_TRUE(src_reg.IsValid());
  EXPECT_TRUE(dst_reg.IsValid());

  delete ra;
}

// Test 2: Source operand with multiple users
TEST_F(LEPUSIRUnaryRegisterAllocTest, UnaryOperatorWithMultipleUsers) {
  auto* func = createTestFunction("test_multiple_users");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();

  builder->SetInsertionPointToEnd(block);

  // Create: val = LoadConst(10)
  auto* val = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(10),
                                             TypeOp::CreateInt32(builder));

  // Create: result1 = ~val (UnaryBitNot)
  auto* bit_not_inst = builder->Create<UnaryOperatorInst>(
      0, val, ValueKind::UnaryBitNotInstKind);

  // Create: result2 = val + 5 (val is used again)
  auto* const5 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(5),
                                                TypeOp::CreateInt32(builder));
  auto* add_inst = builder->Create<BinaryOperatorInst>(
      0, val, const5, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));

  // Create: return result1 + result2
  auto* final_add = builder->Create<BinaryOperatorInst>(
      0, bit_not_inst, add_inst, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));

  builder->Create<ReturnInst>(0, final_add);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify: bit_not_inst should have its own register since val has multiple
  // users. The coalescing logic should NOT merge them because val is used
  // elsewhere.
  EXPECT_TRUE(ra->IsAllocated(val));
  EXPECT_TRUE(ra->IsAllocated(bit_not_inst));

  auto src_reg = ra->GetRegister(val);
  auto dst_reg = ra->GetRegister(bit_not_inst);

  // When the source has multiple users, they will have different registers.
  // This is correct behavior - backend can emit non-destructive unary opcodes
  // (e.g. BitNot2) without forcing src==dst.
  EXPECT_NE(src_reg.GetIndex(), dst_reg.GetIndex())
      << "UnaryBitNot with multiple users: src and dst should use different "
         "registers";

  delete ra;
}

// Test 3: Parameter as source operand
TEST_F(LEPUSIRUnaryRegisterAllocTest, UnaryOperatorWithParameter) {
  auto* func = createTestFunction("test_parameter");
  auto builder = ir_ctx->GetOpBuilder();

  // Add a parameter to the function
  auto* param = new Parameter(func, 0);
  param->SetType(TypeOp::CreateInt32(builder));
  func->AddParam(param);

  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Create: result = !param (UnaryNot)
  auto* not_inst =
      builder->Create<UnaryOperatorInst>(0, param, ValueKind::UnaryNotInstKind);

  builder->Create<ReturnInst>(0, not_inst);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify: Parameter should have a fixed register
  // The coalescing logic should NOT merge them because parameters are
  // pre-allocated
  EXPECT_TRUE(ra->IsAllocated(param));
  EXPECT_TRUE(ra->IsAllocated(not_inst));

  auto param_reg = ra->GetRegister(param);
  auto dst_reg = ra->GetRegister(not_inst);

  // Parameters are pre-allocated, so they will have different registers.
  // This is correct behavior - backend can emit non-destructive unary opcodes
  // (e.g. Not2) without forcing src==dst.
  EXPECT_NE(param_reg.GetIndex(), dst_reg.GetIndex())
      << "UnaryNot with parameter: src and dst should use different registers "
         "because parameters are pre-allocated";

  delete ra;
}

// Test 4: All unary operator types
TEST_F(LEPUSIRUnaryRegisterAllocTest, AllUnaryOperatorTypes) {
  struct UnaryOpTestCase {
    ValueKind kind;
    const char* name;
  };

  UnaryOpTestCase test_cases[] = {
      {ValueKind::UnaryNegInstKind, "UnaryNeg"},
      {ValueKind::UnaryPosInstKind, "UnaryPos"},
      {ValueKind::UnaryNotInstKind, "UnaryNot"},
      {ValueKind::UnaryBitNotInstKind, "UnaryBitNot"},
      {ValueKind::UnaryIncInstKind, "UnaryInc"},
      {ValueKind::UnaryDecInstKind, "UnaryDec"},
      {ValueKind::UnaryTypeofInstKind, "UnaryTypeof"},
  };

  for (const auto& test_case : test_cases) {
    auto* func = createTestFunction(std::string("test_") + test_case.name);
    auto builder = ir_ctx->GetOpBuilder();
    Block* block = &func->Front();

    builder->SetInsertionPointToEnd(block);

    // Create: val = LoadConst(42)
    auto* val = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                               TypeOp::CreateInt32(builder));

    // Create: result = Op(val)
    auto* unary_inst =
        builder->Create<UnaryOperatorInst>(0, val, test_case.kind);

    builder->Create<ReturnInst>(0, unary_inst);

    // Run register allocation
    auto* ra = runRegisterAllocation(func);

    // Verify: both values are allocated (src/dst may or may not share a
    // register).
    EXPECT_TRUE(ra->IsAllocated(val))
        << test_case.name << ": val not allocated";
    EXPECT_TRUE(ra->IsAllocated(unary_inst))
        << test_case.name << ": unary_inst not allocated";

    EXPECT_TRUE(ra->GetRegister(val).IsValid());
    EXPECT_TRUE(ra->GetRegister(unary_inst).IsValid());

    delete ra;
  }
}

// Test 5: Nested unary operators
TEST_F(LEPUSIRUnaryRegisterAllocTest, NestedUnaryOperators) {
  auto* func = createTestFunction("test_nested");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();

  builder->SetInsertionPointToEnd(block);

  // Create: val = LoadConst(42)
  auto* val = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                             TypeOp::CreateInt32(builder));

  // Create: temp1 = -val
  auto* neg_inst =
      builder->Create<UnaryOperatorInst>(0, val, ValueKind::UnaryNegInstKind);

  // Create: temp2 = ~temp1
  auto* bit_not_inst = builder->Create<UnaryOperatorInst>(
      0, neg_inst, ValueKind::UnaryBitNotInstKind);

  // Create: result = !temp2
  auto* not_inst = builder->Create<UnaryOperatorInst>(
      0, bit_not_inst, ValueKind::UnaryNotInstKind);

  builder->Create<ReturnInst>(0, not_inst);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify: All should be allocated (chained unary operations).
  EXPECT_TRUE(ra->IsAllocated(val));
  EXPECT_TRUE(ra->IsAllocated(neg_inst));
  EXPECT_TRUE(ra->IsAllocated(bit_not_inst));
  EXPECT_TRUE(ra->IsAllocated(not_inst));

  EXPECT_TRUE(ra->GetRegister(val).IsValid());
  EXPECT_TRUE(ra->GetRegister(neg_inst).IsValid());
  EXPECT_TRUE(ra->GetRegister(bit_not_inst).IsValid());
  EXPECT_TRUE(ra->GetRegister(not_inst).IsValid());

  delete ra;
}

// New: Unary operators should not be forced to share src/dst registers.
// When the operand is used later, src and dst intervals overlap and must NOT
// share the same physical register.
TEST_F(LEPUSIRUnaryRegisterAllocTest, UnaryOperatorAllowsSrcNotEqualDst) {
  struct UnaryOpTestCase {
    ValueKind kind;
    const char* name;
  };

  UnaryOpTestCase test_cases[] = {
      {ValueKind::UnaryNegInstKind, "UnaryNeg"},
      {ValueKind::UnaryPosInstKind, "UnaryPos"},
      {ValueKind::UnaryNotInstKind, "UnaryNot"},
      {ValueKind::UnaryBitNotInstKind, "UnaryBitNot"},
      {ValueKind::UnaryIncInstKind, "UnaryInc"},
      {ValueKind::UnaryDecInstKind, "UnaryDec"},
      {ValueKind::UnaryTypeofInstKind, "UnaryTypeof"},
  };

  for (const auto& test_case : test_cases) {
    auto* func =
        createTestFunction(std::string("test_src_ne_dst_") + test_case.name);
    auto builder = ir_ctx->GetOpBuilder();
    Block* block = &func->Front();
    builder->SetInsertionPointToEnd(block);

    auto* val = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(10),
                                               TypeOp::CreateInt32(builder));
    auto* unary_inst =
        builder->Create<UnaryOperatorInst>(0, val, test_case.kind);

    // Keep `val` live after `unary_inst`.
    auto* one = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                               TypeOp::CreateInt32(builder));
    auto* add_inst = builder->Create<BinaryOperatorInst>(
        0, val, one, ValueKind::BinaryAddInstKind,
        TypeOp::CreateAnyType(builder));

    // Keep both `unary_inst` and `add_inst` live.
    auto* final_or = builder->Create<BinaryOperatorInst>(
        0, unary_inst, add_inst, ValueKind::BinaryOrInstKind,
        TypeOp::CreateAnyType(builder));
    builder->Create<ReturnInst>(0, final_or);

    auto* ra = runRegisterAllocation(func);
    ASSERT_TRUE(ra->IsAllocated(val)) << test_case.name;
    ASSERT_TRUE(ra->IsAllocated(unary_inst)) << test_case.name;

    auto src_reg = ra->GetRegister(val);
    auto dst_reg = ra->GetRegister(unary_inst);
    EXPECT_NE(src_reg.GetIndex(), dst_reg.GetIndex())
        << test_case.name
        << ": operand is used later; src and dst must use different registers";

    delete ra;
  }
}

// Test 6: Unary operator with literal source (should not optimize)
TEST_F(LEPUSIRUnaryRegisterAllocTest, UnaryOperatorWithLiteralSource) {
  auto* func = createTestFunction("test_literal");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();

  builder->SetInsertionPointToEnd(block);

  // Create: result = -42 (literal as source)
  auto* literal = builder->GetLiteralInt32(42);
  auto* neg_inst = builder->Create<UnaryOperatorInst>(
      0, literal, ValueKind::UnaryNegInstKind);

  builder->Create<ReturnInst>(0, neg_inst);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify: neg_inst should be allocated, but literal won't be
  EXPECT_TRUE(ra->IsAllocated(neg_inst));
  EXPECT_FALSE(ra->IsAllocated(literal))
      << "Literal should not be allocated a register";

  delete ra;
}

// Test 7: Pre-allocated register scenario
TEST_F(LEPUSIRUnaryRegisterAllocTest, UnaryOperatorWithPreAllocatedRegister) {
  auto* func = createTestFunction("test_preallocated");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();

  builder->SetInsertionPointToEnd(block);

  // Create multiple values to occupy registers
  auto* val1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                              TypeOp::CreateInt32(builder));
  auto* val2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                              TypeOp::CreateInt32(builder));
  auto* val3 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(3),
                                              TypeOp::CreateInt32(builder));

  // Create: result = ++val2 (UnaryInc)
  auto* inc_inst =
      builder->Create<UnaryOperatorInst>(0, val2, ValueKind::UnaryIncInstKind);

  // Use all values to keep them live
  auto* sum1 = builder->Create<BinaryOperatorInst>(
      0, val1, inc_inst, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  auto* sum2 = builder->Create<BinaryOperatorInst>(
      0, sum1, val3, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));

  builder->Create<ReturnInst>(0, sum2);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify: both values are allocated (src/dst may or may not share a
  // register).
  EXPECT_TRUE(ra->IsAllocated(val2));
  EXPECT_TRUE(ra->IsAllocated(inc_inst));

  EXPECT_TRUE(ra->GetRegister(val2).IsValid());
  EXPECT_TRUE(ra->GetRegister(inc_inst).IsValid());

  delete ra;
}

// Test 8: Unary operator where src is a parameter (pre-allocated)
// This test verifies that we don't Coalesce when one side is already allocated,
// which prevents crashes in the linear scan phase.
TEST_F(LEPUSIRUnaryRegisterAllocTest, UnaryOperatorWithParameterNoCoalesce) {
  auto* func = createTestFunction("test_param_no_coalesce");
  auto builder = ir_ctx->GetOpBuilder();

  // Add a parameter
  auto* param = new Parameter(func, 0);
  param->SetType(TypeOp::CreateInt32(builder));
  func->AddParam(param);

  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Create: result = -param
  auto* neg_inst =
      builder->Create<UnaryOperatorInst>(0, param, ValueKind::UnaryNegInstKind);

  builder->Create<ReturnInst>(0, neg_inst);

  // Run register allocation
  auto* ra = runRegisterAllocation(func);

  // Verify: src (param) and dst (neg_inst) should NOT use the same register
  // because param is pre-allocated and we should avoid coalescing it.
  EXPECT_TRUE(ra->IsAllocated(param));
  EXPECT_TRUE(ra->IsAllocated(neg_inst));

  auto src_reg = ra->GetRegister(param);
  auto dst_reg = ra->GetRegister(neg_inst);

  EXPECT_NE(src_reg.GetIndex(), dst_reg.GetIndex())
      << "Should NOT Coalesce when source is a pre-allocated parameter";

  delete ra;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
