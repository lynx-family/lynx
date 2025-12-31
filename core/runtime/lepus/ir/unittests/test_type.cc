// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"
#include "core/runtime/lepus/ir/type_op.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestType : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestType, test_primitive_type_initialize) {
  auto* builder = ir_ctx->GetOpBuilder();

  auto* null_type = TypeOp::CreateNull(builder);
  ASSERT_TRUE(null_type->IsNullType());
  auto* null_type2 = TypeOp::CreateNull(builder);
  ASSERT_TRUE(null_type2->IsNullType());
  ASSERT_TRUE((intptr_t)null_type == (intptr_t)null_type2);
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(null_type));
  ASSERT_TRUE(null_type->IsNullOrUndefinedType());

  auto* undefined_type = TypeOp::CreateUndefined(builder);
  ASSERT_TRUE(undefined_type->IsUndefinedType());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(undefined_type));
  ASSERT_TRUE(undefined_type->IsNullOrUndefinedType());

  auto* null_or_undefined_type = TypeOp::CreateNullOrUndefined(builder);
  ASSERT_TRUE(null_or_undefined_type->IsNullOrUndefinedType());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(null_or_undefined_type));

  auto* int32_type = TypeOp::CreateInt32(builder);
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(int32_type));
  ASSERT_TRUE(int32_type->IsInt32Type());
  ASSERT_TRUE(int32_type->IsNumberType());

  auto* uint32_type = TypeOp::CreateUint32(builder);
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(uint32_type));
  ASSERT_TRUE(uint32_type->IsUint32Type());
  ASSERT_TRUE(uint32_type->IsNumberType());

  auto* int64_type = TypeOp::CreateInt64(builder);
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(int64_type));
  ASSERT_TRUE(int64_type->IsInt64Type());
  ASSERT_TRUE(int64_type->IsNumberType());

  auto* float64_type = TypeOp::CreateFloat64(builder);
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(float64_type));
  ASSERT_TRUE(float64_type->IsFloat64Type());
  ASSERT_TRUE(float64_type->IsNumberType());

  auto* boolean_type = TypeOp::CreateBoolean(builder);
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(boolean_type));
  ASSERT_TRUE(boolean_type->IsBooleanType());

  auto* int8_type = TypeOp::CreateInt8(builder);
  ASSERT_TRUE(int8_type->IsInt8Type());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(int8_type));

  auto* uint8_type = TypeOp::CreateUint8(builder);
  ASSERT_TRUE(uint8_type->IsUint8Type());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(uint8_type));

  auto* array_type = TypeOp::CreateArray(builder);
  ASSERT_TRUE(array_type->IsArrayType());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(array_type));

  auto* table_type = TypeOp::CreateTable(builder);
  ASSERT_TRUE(table_type->IsTableType());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(table_type));

  auto* string_type = TypeOp::CreateString(builder);
  ASSERT_TRUE(string_type->IsStringType());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(string_type));

  auto* any_type = TypeOp::CreateAnyType(builder);
  ASSERT_TRUE(any_type->IsAnyType());
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(any_type));

  // literal
  auto* true_literal = builder->GetLiteralBool(true);
  ASSERT_TRUE(true_literal->GetType()->IsBooleanType());
  auto* false_literal = builder->GetLiteralBool(false);
  ASSERT_TRUE(false_literal->GetType()->IsBooleanType());
  auto* null_literal = builder->GetLiteralNull();
  ASSERT_TRUE(null_literal->GetType()->IsNullType());
  auto* uint8_literal = builder->GetLiteralUint8(1);
  ASSERT_TRUE(uint8_literal->GetType()->IsUint8Type());
  auto* int8_literal = builder->GetLiteralInt8(1);
  ASSERT_TRUE(int8_literal->GetType()->IsInt8Type());
  auto* uint32_literal = builder->GetLiteralUint32(1);
  ASSERT_TRUE(uint32_literal->GetType()->IsUint32Type());
  auto* int32_literal = builder->GetLiteralInt32(1);
  ASSERT_TRUE(int32_literal->GetType()->IsInt32Type());
  auto* float64_literal = builder->GetLiteralFloat64(1.0);
  ASSERT_TRUE(float64_literal->GetType()->IsFloat64Type());
  auto* uint64_type = TypeOp::CreateUint64(builder);
  ASSERT_TRUE(llvh::isa<PrimitiveTypeOp>(uint64_type));
  ASSERT_TRUE(uint64_type->IsNumberType());
}

TEST_F(LEPUSIRTestType,
       BinaryAddSelectsStringTypeWhenEitherOperandIsStringType) {
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_add_select_string_type";
  auto* func = builder->Create<FuncOp>(0, name);
  auto lepus_func = lynx::lepus::Function::Create();
  func->Init(lepus_func);

  // Create params with explicit types.
  auto* str = func->CreateParam(0);
  str->SetType(TypeOp::CreateString(builder));
  auto* num = func->CreateParam(1);
  num->SetType(TypeOp::CreateInt64(builder));

  Block* entry =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  // When ret_type is Any, BinaryOperatorInst will select its result type based
  // on operand types.
  auto* add_str_num = builder->Create<BinaryOperatorInst>(
      0, str, num, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(builder));
  ASSERT_NE(add_str_num->GetType(), nullptr);
  EXPECT_TRUE(add_str_num->GetType()->IsStringType());

  auto* add_num_str = builder->Create<BinaryOperatorInst>(
      0, num, str, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(builder));
  ASSERT_NE(add_num_str->GetType(), nullptr);
  EXPECT_TRUE(add_num_str->GetType()->IsStringType());

  builder->Create<ReturnInst>(0, add_num_str);
}

TEST_F(LEPUSIRTestType, BinaryAddKeepsAnyTypeWhenEitherOperandIsAnyType) {
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_add_keeps_any_type";
  auto* func = builder->Create<FuncOp>(0, name);
  auto lepus_func = lynx::lepus::Function::Create();
  func->Init(lepus_func);

  // any + int64
  auto* any_v = func->CreateParam(0);
  any_v->SetType(TypeOp::CreateAnyType(builder));
  auto* int_v = func->CreateParam(1);
  int_v->SetType(TypeOp::CreateInt64(builder));
  // any2
  auto* any_v2 = func->CreateParam(2);
  any_v2->SetType(TypeOp::CreateAnyType(builder));

  Block* entry =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  // When ret_type is Any, BinaryOperatorInst selects the result type based on
  // operand types. If either operand is AnyType, the result must remain AnyType
  // (otherwise isel may pick an unsound specialized opcode).
  auto* add_any_int = builder->Create<BinaryOperatorInst>(
      0, any_v, int_v, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(builder));
  ASSERT_NE(add_any_int->GetType(), nullptr);
  EXPECT_TRUE(add_any_int->GetType()->IsAnyType());

  auto* add_int_any = builder->Create<BinaryOperatorInst>(
      0, int_v, any_v, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(builder));
  ASSERT_NE(add_int_any->GetType(), nullptr);
  EXPECT_TRUE(add_int_any->GetType()->IsAnyType());

  auto* add_any_any = builder->Create<BinaryOperatorInst>(
      0, any_v, any_v2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(builder));
  ASSERT_NE(add_any_any->GetType(), nullptr);
  EXPECT_TRUE(add_any_any->GetType()->IsAnyType());

  builder->Create<ReturnInst>(0, add_any_any);
}

TEST_F(LEPUSIRTestType, UnaryOperatorTypeSelect) {
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  // Fix FuncOp constructor: string ref issue
  std::string name = "test_unary";
  auto* func = builder->Create<FuncOp>(0, name);
  auto lepus_func = lynx::lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  // Using LiteralInt64 as value
  auto* lit = builder->GetLiteralInt32(123);

  // UnaryNot (!) should return Boolean
  auto* not_inst =
      builder->Create<UnaryOperatorInst>(0, lit, ValueKind::UnaryNotInstKind);
  EXPECT_TRUE(not_inst->GetType()->IsBooleanType())
      << "UnaryNot should be Boolean";

  // UnaryTypeof should return String
  auto* typeof_inst = builder->Create<UnaryOperatorInst>(
      0, lit, ValueKind::UnaryTypeofInstKind);
  EXPECT_TRUE(typeof_inst->GetType()->IsStringType())
      << "UnaryTypeof should be String";

  // Create an Int64 value for testing
  auto* int64_val = func->CreateParam(1);
  int64_val->SetType(TypeOp::CreateInt64(builder));

  // UnaryBitNot (~) should return Int64
  auto* bit_not_inst = builder->Create<UnaryOperatorInst>(
      0, int64_val, ValueKind::UnaryBitNotInstKind);
  EXPECT_TRUE(bit_not_inst->GetType()->IsInt64Type())
      << "UnaryBitNot should be Int64";

  // UnaryNeg (-)
  // Int64 -> Int64
  auto* neg_int_inst = builder->Create<UnaryOperatorInst>(
      0, int64_val, ValueKind::UnaryNegInstKind);
  EXPECT_TRUE(neg_int_inst->GetType()->IsInt64Type())
      << "UnaryNeg with Int64 should return Int64";

  // Float64 -> Float64
  auto* float_lit = builder->GetLiteralFloat64(1.5);
  auto* neg_float_inst = builder->Create<UnaryOperatorInst>(
      0, float_lit, ValueKind::UnaryNegInstKind);
  EXPECT_TRUE(neg_float_inst->GetType()->IsFloat64Type())
      << "UnaryNeg with Float64 should return Float64";

  // UnaryPos (+)
  // Int64 -> Int64 (preserved)
  auto* pos_int_inst = builder->Create<UnaryOperatorInst>(
      0, int64_val, ValueKind::UnaryPosInstKind);
  EXPECT_TRUE(pos_int_inst->GetType()->IsInt64Type())
      << "UnaryPos with Int64 should return Int64";

  // String -> Any (conversion uncertainty)
  auto* str_val = func->CreateParam(0);
  str_val->SetType(TypeOp::CreateString(builder));
  auto* pos_str_inst = builder->Create<UnaryOperatorInst>(
      0, str_val, ValueKind::UnaryPosInstKind);
  EXPECT_TRUE(pos_str_inst->GetType()->IsAnyType())
      << "UnaryPos with String should return Any";

  // UnaryInc (++) / UnaryDec (--)
  // Int64 -> Int64
  auto* inc_int_inst = builder->Create<UnaryOperatorInst>(
      0, int64_val, ValueKind::UnaryIncInstKind);
  EXPECT_TRUE(inc_int_inst->GetType()->IsInt64Type())
      << "UnaryInc with Int64 should return Int64";

  auto* dec_int_inst = builder->Create<UnaryOperatorInst>(
      0, int64_val, ValueKind::UnaryDecInstKind);
  EXPECT_TRUE(dec_int_inst->GetType()->IsInt64Type())
      << "UnaryDec with Int64 should return Int64";

  // Float64 -> Float64
  auto* inc_float_inst = builder->Create<UnaryOperatorInst>(
      0, float_lit, ValueKind::UnaryIncInstKind);
  EXPECT_TRUE(inc_float_inst->GetType()->IsFloat64Type())
      << "UnaryInc with Float64 should return Float64";

  // Any -> Any
  auto* any_val = func->CreateParam(2);
  any_val->SetType(TypeOp::CreateAnyType(builder));
  auto* inc_any_inst = builder->Create<UnaryOperatorInst>(
      0, any_val, ValueKind::UnaryIncInstKind);
  EXPECT_TRUE(inc_any_inst->GetType()->IsAnyType())
      << "UnaryInc with Any should return Any";

  builder->Create<ReturnInst>(0, not_inst);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
