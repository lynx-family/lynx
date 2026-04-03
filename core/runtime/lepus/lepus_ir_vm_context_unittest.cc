// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/runtime/lepus/builtin_function_table.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/op_code.h"
#include "core/runtime/lepus/vm_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {
namespace test {

namespace {

RestrictedValue BuiltinReturn7(VMContext*) { return RestrictedValue(7); }

Value RunFunctionAsClosure(const fml::RefPtr<Function>& fn,
                           const std::vector<Value>& args = {}) {
  VMContext ctx;
  auto closure = Closure::Create(fn);
  Value closure_val{fml::RefPtr<lepus::RefCounted>(closure)};

  std::vector<const Value*> argv;
  argv.reserve(args.size());
  for (const auto& arg : args) {
    argv.push_back(&arg);
  }
  return ctx.CallClosureArgs(closure_val, argv.data(), argv.size());
}

Instruction MakeRandomRegEncoding(uint8_t r0, uint8_t r1 = 0, uint8_t r2 = 0,
                                  uint8_t r3 = 0) {
  Instruction inst;
  inst.op_code_ = (static_cast<uint32_t>(r0) << 24) |
                  (static_cast<uint32_t>(r1) << 16) |
                  (static_cast<uint32_t>(r2) << 8) | static_cast<uint32_t>(r3);
  return inst;
}

}  // namespace

TEST(LEPUS_IR_VMContext, GetTableString_ReadTableStringKey) {
  auto fn = Function::Create();

  const auto k_idx = fn->AddConstString(base::String("k"));
  const auto v_idx = fn->AddConstNumber(42);

  // r0 = {}
  fn->AddInstruction(Instruction::ACode(TypeOp_NewTable, 0));
  // r1 = "k"
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, k_idx));
  // r2 = 42
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, v_idx));
  // r0["k"] = 42
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetTable, 0, 1, 2));
  // r3 = r0["k"]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetTableString, 3, 0, 1));
  // return r3
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 3));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 42);
}

TEST(LEPUS_IR_VMContext, GetTableConstString_ArrayLength) {
  auto fn = Function::Create();

  auto arr = CArray::Create();
  arr->push_back(Value(1));
  arr->push_back(Value(2));
  arr->push_back(Value(3));
  const auto arr_idx = fn->AddConstValue(Value(arr));
  const auto length_key_idx = fn->AddConstString(base::String("length"));

  // r0 = [1,2,3]
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, arr_idx));
  // r1 = r0["length"]
  fn->AddInstruction(
      Instruction::ABCCode(TypeOp_GetTableConstString, 1, 0, length_key_idx));
  // return r1
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 3);
}

TEST(LEPUS_IR_VMContext, GetTableNumber_TableNumberKeyStringified) {
  auto fn = Function::Create();

  const auto key_str_idx = fn->AddConstString(base::String("7"));
  const auto value_idx = fn->AddConstNumber(99);
  const auto key_num_idx = fn->AddConstNumber(7);

  // r0 = {}
  fn->AddInstruction(Instruction::ACode(TypeOp_NewTable, 0));
  // r1 = "7"
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, key_str_idx));
  // r2 = 99
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, value_idx));
  // r0["7"] = 99
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetTable, 0, 1, 2));
  // r3 = 7
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 3, key_num_idx));
  // r4 = r0[7]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetTableNumber, 4, 0, 3));
  // return r4
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 4));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 99);
}

TEST(LEPUS_IR_VMContext, DeepClone_ArrayIndependence) {
  auto fn = Function::Create();

  auto src_arr = CArray::Create();
  src_arr->push_back(Value(1));
  src_arr->push_back(Value(2));
  const auto src_arr_idx = fn->AddConstValue(Value(src_arr));
  const auto idx0_i64 = fn->AddConstValue(Value(static_cast<int64_t>(0)));
  const auto v99_idx = fn->AddConstNumber(99);

  // r0 = [1,2]
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, src_arr_idx));
  // r1 = clone(r0)
  fn->AddInstruction(Instruction::ABCode(TypeOp_DeepClone, 1, 0));
  // r2 = 0 (int64)
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, idx0_i64));
  // r3 = 99
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 3, v99_idx));
  // r1[0] = 99
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetTable, 1, 2, 3));
  // r4 = r0[0]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetArrayInt64, 4, 0, 2));
  // return r4
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 4));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 1);
}

TEST(LEPUS_IR_VMContext, LoadConstAndClone_ArrayIndependenceFromConstPool) {
  auto fn = Function::Create();

  auto arr = CArray::Create();
  arr->push_back(Value(1));
  arr->push_back(Value(2));
  const auto arr_idx = fn->AddConstValue(Value(arr));
  const auto idx0_i64 = fn->AddConstValue(Value(static_cast<int64_t>(0)));
  const auto v99_idx = fn->AddConstNumber(99);

  // r0 = const_arr
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, arr_idx));
  // r1 = clone(const_arr)
  fn->AddInstruction(
      Instruction::ABxCode(TypeOp_LoadConstAndClone, 1, arr_idx));
  // r2 = 0 (int64)
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, idx0_i64));
  // r3 = 99
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 3, v99_idx));
  // r1[0] = 99
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetTable, 1, 2, 3));
  // r4 = r1[0]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetArrayInt64, 4, 1, 2));
  // r5 = r0[0]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetArrayInt64, 5, 0, 2));
  // return r4 + r5
  fn->AddInstruction(Instruction::ABCCode(TypeOp_Add, 6, 4, 5));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 6));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 100);
}

TEST(LEPUS_IR_VMContext, LoadConstAndClone_TableIndependenceFromConstPool) {
  auto fn = Function::Create();

  auto tbl = Dictionary::Create({{base::String("k"), Value(1)}});
  const auto tbl_idx = fn->AddConstValue(Value(tbl));
  const auto k_idx = fn->AddConstString(base::String("k"));
  const auto v99_idx = fn->AddConstNumber(99);

  // r0 = const_tbl
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, tbl_idx));
  // r1 = clone(const_tbl)
  fn->AddInstruction(
      Instruction::ABxCode(TypeOp_LoadConstAndClone, 1, tbl_idx));
  // r2 = "k"
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, k_idx));
  // r3 = 99
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 3, v99_idx));
  // r1["k"] = 99
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetTable, 1, 2, 3));
  // r4 = r1["k"]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetTableString, 4, 1, 2));
  // r5 = r0["k"]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetTableString, 5, 0, 2));
  // return r4 + r5
  fn->AddInstruction(Instruction::ABCCode(TypeOp_Add, 6, 4, 5));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 6));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 100);
}

TEST(LEPUS_IR_VMContext, Inc2_Int64Increment) {
  auto fn = Function::Create();

  const auto v41_i64_idx = fn->AddConstValue(Value(static_cast<int64_t>(41)));
  // r0 = 41 (int64)
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v41_i64_idx));
  // r1 = r0 + 1
  fn->AddInstruction(Instruction::ABCode(TypeOp_Inc2, 1, 0));
  // return r1
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 42);
}

TEST(LEPUS_IR_VMContext, GetSetToplevelClosureVar_ByIndex) {
  auto fn = Function::Create();

  const auto v7_idx = fn->AddConstNumber(7);
  const auto v0_idx = fn->AddConstNumber(0);

  // r0 = 7
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v7_idx));
  // toplevel[5] = r0
  fn->AddInstruction(Instruction::ABCode(TypeOp_SetToplevelClosureVar, 0, 5));
  // r0 = 0
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v0_idx));
  // r1 = toplevel[5]
  fn->AddInstruction(Instruction::ABCode(TypeOp_GetToplevelClosureVar, 1, 5));
  // return r1
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 7);
}

TEST(LEPUS_IR_VMContext, CallRandom_CopyArgsAndCall) {
  // callee(args0..3) => (((arg0 + arg1) + arg2) + arg3)
  auto callee = Function::Create();
  callee->AddInstruction(Instruction::ABCCode(TypeOp_Add, 4, 0, 1));
  callee->AddInstruction(Instruction::ABCCode(TypeOp_Add, 4, 4, 2));
  callee->AddInstruction(Instruction::ABCCode(TypeOp_Add, 4, 4, 3));
  callee->AddInstruction(Instruction::ACode(TypeOp_Ret, 4));

  auto caller = Function::Create();
  caller->AddChildFunction(callee);

  const auto v1_idx = caller->AddConstNumber(1);
  const auto v2_idx = caller->AddConstNumber(2);
  const auto v3_idx = caller->AddConstNumber(3);
  const auto v4_idx = caller->AddConstNumber(4);

  // r0 = closure(callee)
  caller->AddInstruction(Instruction::ABxCode(TypeOp_Closure, 0, 0));
  // r20..r23 = 1..4
  caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 20, v1_idx));
  caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 21, v2_idx));
  caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 22, v3_idx));
  caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 23, v4_idx));

  // CallRandom will copy args into r1..r4 using the following "arg encoding"
  // instruction, then invoke r0 with argc=4 and store result in r5.
  caller->AddInstruction(Instruction::ABCCode(TypeOp_CallRandom, 0, 4, 5));
  Instruction arg_encoding;
  arg_encoding.op_code_ =
      (static_cast<uint32_t>(20) << 24) | (static_cast<uint32_t>(21) << 16) |
      (static_cast<uint32_t>(22) << 8) | static_cast<uint32_t>(23);
  caller->AddInstruction(arg_encoding);

  caller->AddInstruction(Instruction::ACode(TypeOp_Ret, 5));

  auto ret = RunFunctionAsClosure(caller);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 10);
}

TEST(LEPUS_IR_VMContext, CallRandomRemainderVariants_CopyArgsAndCall) {
  // callee sums args[0..N-1]
  auto make_callee = [](int n) {
    auto callee = Function::Create();
    // rN = r0 + r1
    callee->AddInstruction(Instruction::ABCCode(TypeOp_Add, n, 0, 1));
    for (int i = 2; i < n; ++i) {
      callee->AddInstruction(Instruction::ABCCode(TypeOp_Add, n, n, i));
    }
    callee->AddInstruction(Instruction::ACode(TypeOp_Ret, n));
    return callee;
  };

  // argc = 5 -> CallRandom1
  {
    auto callee = make_callee(5);
    auto caller = Function::Create();
    caller->AddChildFunction(callee);
    const auto v1 = caller->AddConstNumber(1);
    const auto v2 = caller->AddConstNumber(2);
    const auto v3 = caller->AddConstNumber(3);
    const auto v4 = caller->AddConstNumber(4);
    const auto v5 = caller->AddConstNumber(5);
    caller->AddInstruction(Instruction::ABxCode(TypeOp_Closure, 0, 0));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 20, v1));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 21, v2));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 22, v3));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 23, v4));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 24, v5));
    caller->AddInstruction(Instruction::ABCCode(TypeOp_CallRandom1, 0, 5, 6));
    caller->AddInstruction(MakeRandomRegEncoding(20, 21, 22, 23));
    caller->AddInstruction(MakeRandomRegEncoding(24));
    caller->AddInstruction(Instruction::ACode(TypeOp_Ret, 6));
    auto ret = RunFunctionAsClosure(caller);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 15);
  }

  // argc = 6 -> CallRandom2
  {
    auto callee = make_callee(6);
    auto caller = Function::Create();
    caller->AddChildFunction(callee);
    const auto v1 = caller->AddConstNumber(1);
    const auto v2 = caller->AddConstNumber(2);
    const auto v3 = caller->AddConstNumber(3);
    const auto v4 = caller->AddConstNumber(4);
    const auto v5 = caller->AddConstNumber(5);
    const auto v6 = caller->AddConstNumber(6);
    caller->AddInstruction(Instruction::ABxCode(TypeOp_Closure, 0, 0));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 20, v1));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 21, v2));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 22, v3));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 23, v4));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 24, v5));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 25, v6));
    caller->AddInstruction(Instruction::ABCCode(TypeOp_CallRandom2, 0, 6, 7));
    caller->AddInstruction(MakeRandomRegEncoding(20, 21, 22, 23));
    caller->AddInstruction(MakeRandomRegEncoding(24, 25));
    caller->AddInstruction(Instruction::ACode(TypeOp_Ret, 7));
    auto ret = RunFunctionAsClosure(caller);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 21);
  }

  // argc = 7 -> CallRandom3
  {
    auto callee = make_callee(7);
    auto caller = Function::Create();
    caller->AddChildFunction(callee);
    const auto v1 = caller->AddConstNumber(1);
    const auto v2 = caller->AddConstNumber(2);
    const auto v3 = caller->AddConstNumber(3);
    const auto v4 = caller->AddConstNumber(4);
    const auto v5 = caller->AddConstNumber(5);
    const auto v6 = caller->AddConstNumber(6);
    const auto v7 = caller->AddConstNumber(7);
    caller->AddInstruction(Instruction::ABxCode(TypeOp_Closure, 0, 0));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 20, v1));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 21, v2));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 22, v3));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 23, v4));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 24, v5));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 25, v6));
    caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 26, v7));
    caller->AddInstruction(Instruction::ABCCode(TypeOp_CallRandom3, 0, 7, 8));
    caller->AddInstruction(MakeRandomRegEncoding(20, 21, 22, 23));
    caller->AddInstruction(MakeRandomRegEncoding(24, 25, 26));
    caller->AddInstruction(Instruction::ACode(TypeOp_Ret, 8));
    auto ret = RunFunctionAsClosure(caller);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 28);
  }
}

TEST(LEPUS_IR_VMContext, Call1_CopySingleArgAndCall) {
  // callee(x) => x + 1
  auto callee = Function::Create();
  const auto one_idx = callee->AddConstNumber(1);
  callee->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, one_idx));
  callee->AddInstruction(Instruction::ABCCode(TypeOp_Add, 2, 0, 1));
  callee->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));

  auto caller = Function::Create();
  caller->AddChildFunction(callee);
  const auto forty_one = caller->AddConstNumber(41);
  caller->AddInstruction(Instruction::ABxCode(TypeOp_Closure, 0, 0));
  caller->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 10, forty_one));
  caller->AddInstruction(Instruction::ABCCode(TypeOp_Call1, 0, 10, 2));
  caller->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));

  auto ret = RunFunctionAsClosure(caller);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 42);
}

TEST(LEPUS_IR_VMContext, NewArrayConsecutive_BuildFromRegisterSpan) {
  auto fn = Function::Create();
  const auto v1 = fn->AddConstNumber(1);
  const auto v2 = fn->AddConstNumber(2);
  const auto v3 = fn->AddConstNumber(3);
  const auto idx2 = fn->AddConstValue(Value(static_cast<int64_t>(2)));

  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 5, v1));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 6, v2));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 7, v3));
  // r0 = [r5, r6, r7]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_NewArrayConsecutive, 0, 3, 5));
  // r1 = 2 (int64)
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, idx2));
  // r2 = r0[2]
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetArrayInt64, 2, 0, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 3);
}

TEST(LEPUS_IR_VMContext, NewArrayRandomVariants_BuildFromEncodedRegisters) {
  auto build_and_check = [](TypeOpCode op, int argc,
                            const std::vector<uint8_t>& regs) {
    auto fn = Function::Create();
    std::vector<size_t> const_idx;
    const_idx.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
      const_idx.push_back(fn->AddConstNumber(i + 1));
    }
    for (int i = 0; i < argc; ++i) {
      fn->AddInstruction(
          Instruction::ABxCode(TypeOp_LoadConst, regs[static_cast<size_t>(i)],
                               const_idx[static_cast<size_t>(i)]));
    }
    fn->AddInstruction(Instruction::ABCCode(op, 0, argc, 0));
    // encoding instructions
    if (argc >= 4) {
      fn->AddInstruction(
          MakeRandomRegEncoding(regs[0], regs[1], regs[2], regs[3]));
    }
    if (argc == 5) {
      fn->AddInstruction(MakeRandomRegEncoding(regs[4]));
    } else if (argc == 6) {
      fn->AddInstruction(MakeRandomRegEncoding(regs[4], regs[5]));
    } else if (argc == 7) {
      fn->AddInstruction(MakeRandomRegEncoding(regs[4], regs[5], regs[6]));
    }
    const auto idx_last =
        fn->AddConstValue(Value(static_cast<int64_t>(argc - 1)));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, idx_last));
    fn->AddInstruction(Instruction::ABCCode(TypeOp_GetArrayInt64, 2, 0, 1));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    auto ret = RunFunctionAsClosure(fn);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), argc);
  };

  // argc=4 -> NewArrayRandom
  build_and_check(TypeOp_NewArrayRandom, 4, {20, 21, 22, 23});
  // argc=5 -> NewArrayRandom1
  build_and_check(TypeOp_NewArrayRandom1, 5, {20, 21, 22, 23, 24});
  // argc=6 -> NewArrayRandom2
  build_and_check(TypeOp_NewArrayRandom2, 6, {20, 21, 22, 23, 24, 25});
  // argc=7 -> NewArrayRandom3
  build_and_check(TypeOp_NewArrayRandom3, 7, {20, 21, 22, 23, 24, 25, 26});
}

TEST(LEPUS_IR_VMContext, SetGetObject_StringNumberAndConstStringKeys) {
  auto fn = Function::Create();

  const auto k_idx = fn->AddConstString(base::String("k"));
  const auto v_idx = fn->AddConstNumber(1);
  const auto num_key_idx = fn->AddConstNumber(7);
  const auto num_val_idx = fn->AddConstNumber(2);
  const auto const_key_idx = fn->AddConstString(base::String("ck"));
  const auto const_val_idx = fn->AddConstNumber(3);

  fn->AddInstruction(Instruction::ACode(TypeOp_NewTable, 0));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, k_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, v_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetObjectString, 0, 1, 2));

  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 3, num_key_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 4, num_val_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetObjectNumber, 0, 3, 4));

  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 5, const_val_idx));
  fn->AddInstruction(
      Instruction::ABCCode(TypeOp_SetObjectConstString, 0, const_key_idx, 5));

  // Read back: r6 = obj["k"], r7 = obj[7], r8 = obj["ck"], return sum.
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetObjectString, 6, 0, 1));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetObjectNumber, 7, 0, 3));
  fn->AddInstruction(
      Instruction::ABCCode(TypeOp_GetTableConstString, 8, 0, const_key_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_Add, 9, 6, 7));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_Add, 9, 9, 8));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 9));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 6);
}

TEST(LEPUS_IR_VMContext, SetTableNumber_ArrayPath) {
  auto fn = Function::Create();
  auto arr = CArray::Create();
  arr->push_back(Value(0));
  arr->push_back(Value(0));
  const auto arr_idx = fn->AddConstValue(Value(arr));
  const auto idx1 = fn->AddConstValue(Value(static_cast<int64_t>(1)));
  const auto v42 = fn->AddConstNumber(42);

  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, arr_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, idx1));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, v42));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_SetTableNumber, 0, 1, 2));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetArrayInt64, 3, 0, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 3));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 42);
}

TEST(LEPUS_IR_VMContext, AddStringString_Concat) {
  auto fn = Function::Create();
  const auto a_idx = fn->AddConstString(base::String("a"));
  const auto b_idx = fn->AddConstString(base::String("b"));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, b_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_AddStringString, 2, 0, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsString());
  EXPECT_EQ(ret.StdString(), "ab");
}

TEST(LEPUS_IR_VMContext, AddStringAny_StringPlusNumber) {
  auto fn = Function::Create();
  const auto a_idx = fn->AddConstString(base::String("a"));
  const auto one_idx = fn->AddConstNumber(1);
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, one_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_AddStringAny, 2, 0, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsString());
  EXPECT_EQ(ret.StdString(), "a1");
}

TEST(LEPUS_IR_VMContext, AddAnyString_NumberPlusString) {
  auto fn = Function::Create();
  const auto one_idx = fn->AddConstNumber(1);
  const auto a_idx = fn->AddConstString(base::String("a"));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, one_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, a_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_AddAnyString, 2, 0, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsString());
  EXPECT_EQ(ret.StdString(), "1a");
}

TEST(LEPUS_IR_VMContext, EqualString_ReturnsTrue) {
  auto fn = Function::Create();
  const auto a_idx = fn->AddConstString(base::String("a"));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, a_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_EqualString, 2, 0, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsBool());
  EXPECT_TRUE(ret.Bool());
}

TEST(LEPUS_IR_VMContext, UnEqualString_ReturnsTrue) {
  auto fn = Function::Create();
  const auto a_idx = fn->AddConstString(base::String("a"));
  const auto b_idx = fn->AddConstString(base::String("b"));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, b_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_UnEqualString, 2, 0, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsBool());
  EXPECT_TRUE(ret.Bool());
}

TEST(LEPUS_IR_VMContext, GetStringLength_ReturnsUtf16Length) {
  auto fn = Function::Create();
  const auto ab_idx = fn->AddConstString(base::String("ab"));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, ab_idx));
  fn->AddInstruction(Instruction::ABCode(TypeOp_GetStringLength, 1, 0));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 2);
}

TEST(LEPUS_IR_VMContext, ToString_NumberToString) {
  auto fn = Function::Create();
  const auto v42 = fn->AddConstNumber(42);
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v42));
  fn->AddInstruction(Instruction::ABCode(TypeOp_ToString, 1, 0));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsString());
  EXPECT_EQ(ret.StdString(), "42");
}

TEST(LEPUS_IR_VMContext, Typeof2_Number) {
  auto fn = Function::Create();
  const auto v1 = fn->AddConstNumber(1);
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v1));
  fn->AddInstruction(Instruction::ABCode(TypeOp_Typeof2, 1, 0));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsString());
  EXPECT_EQ(ret.StdString(), "number");
}

TEST(LEPUS_IR_VMContext, Not2_BoolInversion) {
  auto fn = Function::Create();
  const auto v_true = fn->AddConstBoolean(true);
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v_true));
  fn->AddInstruction(Instruction::ABCode(TypeOp_Not2, 1, 0));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsBool());
  EXPECT_FALSE(ret.Bool());
}

TEST(LEPUS_IR_VMContext, Neg2Pos2_Int64) {
  auto fn = Function::Create();
  const auto v41_i64 = fn->AddConstValue(Value(static_cast<int64_t>(41)));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v41_i64));
  fn->AddInstruction(Instruction::ABCode(TypeOp_Neg2, 1, 0));
  fn->AddInstruction(Instruction::ABCode(TypeOp_Pos2, 2, 1));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), -41);
}

TEST(LEPUS_IR_VMContext, BitNot2_Int64) {
  auto fn = Function::Create();
  const auto v1_i64 = fn->AddConstValue(Value(static_cast<int64_t>(1)));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v1_i64));
  fn->AddInstruction(Instruction::ABCode(TypeOp_BitNot2, 1, 0));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), -2);
}

TEST(LEPUS_IR_VMContext, Dec2_Int64Decrement) {
  auto fn = Function::Create();
  const auto v41_i64 = fn->AddConstValue(Value(static_cast<int64_t>(41)));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v41_i64));
  fn->AddInstruction(Instruction::ABCode(TypeOp_Dec2, 1, 0));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 40);
}

TEST(LEPUS_IR_VMContext, JumpHandlers_EqualUnEqualBoolVariants) {
  // EqualJmpFalse: jump when not equal.
  {
    auto fn = Function::Create();
    const auto a_idx = fn->AddConstString(base::String("a"));
    const auto b_idx = fn->AddConstString(base::String("b"));
    const auto one = fn->AddConstNumber(1);
    const auto zero = fn->AddConstNumber(0);
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, b_idx));
    fn->AddInstruction(Instruction::ABCCode(TypeOp_EqualJmpFalse, 0, 3, 1));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, one));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, zero));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    auto ret = RunFunctionAsClosure(fn);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 0);
  }

  // EqualJmpTrue: jump when equal.
  {
    auto fn = Function::Create();
    const auto a_idx = fn->AddConstString(base::String("a"));
    const auto one = fn->AddConstNumber(1);
    const auto zero = fn->AddConstNumber(0);
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, a_idx));
    fn->AddInstruction(Instruction::ABCCode(TypeOp_EqualJmpTrue, 0, 3, 1));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, zero));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, one));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    auto ret = RunFunctionAsClosure(fn);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 1);
  }

  // UnEqualJmpFalse: jump when equal.
  {
    auto fn = Function::Create();
    const auto a_idx = fn->AddConstString(base::String("a"));
    const auto one = fn->AddConstNumber(1);
    const auto zero = fn->AddConstNumber(0);
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, a_idx));
    fn->AddInstruction(Instruction::ABCCode(TypeOp_UnEqualJmpFalse, 0, 3, 1));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, zero));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, one));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    auto ret = RunFunctionAsClosure(fn);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 1);
  }

  // UnEqualJmpTrue: jump when not equal.
  {
    auto fn = Function::Create();
    const auto a_idx = fn->AddConstString(base::String("a"));
    const auto b_idx = fn->AddConstString(base::String("b"));
    const auto one = fn->AddConstNumber(1);
    const auto zero = fn->AddConstNumber(0);
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, a_idx));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, b_idx));
    fn->AddInstruction(Instruction::ABCCode(TypeOp_UnEqualJmpTrue, 0, 3, 1));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, zero));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 2, one));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 2));
    auto ret = RunFunctionAsClosure(fn);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 1);
  }

  // BoolJmpFalse / BoolJmpTrue
  {
    auto fn = Function::Create();
    const auto v_false = fn->AddConstBoolean(false);
    const auto one = fn->AddConstNumber(1);
    const auto zero = fn->AddConstNumber(0);

    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v_false));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_BoolJmpFalse, 0, 3));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, zero));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, one));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));

    auto ret = RunFunctionAsClosure(fn);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 1);
  }

  {
    auto fn = Function::Create();
    const auto v_true = fn->AddConstBoolean(true);
    const auto one = fn->AddConstNumber(1);
    const auto zero = fn->AddConstNumber(0);
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, v_true));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_BoolJmpTrue, 0, 3));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, zero));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
    fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, one));
    fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 1));
    auto ret = RunFunctionAsClosure(fn);
    ASSERT_TRUE(ret.IsNumber());
    EXPECT_EQ(ret.Number(), 1);
  }
}

TEST(LEPUS_IR_VMContext, GetBuiltinFunc_FetchAndCallBuiltin) {
  static BuiltinFunctionTable kTable(BuiltinFunctionTable::Object,
                                     {{"foo", &BuiltinReturn7}});

  auto fn = Function::Create();
  const auto tbl_idx = fn->AddConstValue(Value(&kTable));
  const auto key_idx = fn->AddConstString(base::String("foo"));

  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 0, tbl_idx));
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadConst, 1, key_idx));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_GetBuiltinFunc, 2, 0, 1));
  fn->AddInstruction(Instruction::ABCCode(TypeOp_Call, 2, 0, 3));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 3));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 7);
}

TEST(LEPUS_IR_VMContext, LoadSmallInt_LoadsImmediateIntoDstReg) {
  auto fn = Function::Create();

  // r0 = 42 (imm16)
  fn->AddInstruction(Instruction::ABxCode(TypeOp_LoadSmallInt, 0, 42));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 0));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), 42);
}

TEST(LEPUS_IR_VMContext, LoadSmallInt_SignExtendsImm16) {
  auto fn = Function::Create();

  // r0 = -1 (imm16)
  fn->AddInstruction(
      Instruction::ABxCode(TypeOp_LoadSmallInt, 0, static_cast<uint16_t>(-1)));
  fn->AddInstruction(Instruction::ACode(TypeOp_Ret, 0));

  auto ret = RunFunctionAsClosure(fn);
  ASSERT_TRUE(ret.IsNumber());
  EXPECT_EQ(ret.Number(), -1);
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
