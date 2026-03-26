// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/op_code.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

static std::string g_log;

static lepus::Value EmptyFunc(lepus::MTSContext* context, lepus::Value*, int) {
  return lepus::Value();
}

static lepus::Value AppendResult(lepus::MTSContext* context, lepus::Value* argv,
                                 int argc) {
  if (argc <= 0) {
    return lepus::Value();
  }

  lepus::Value v = argv[0];
  std::string s;
  if (v.IsString()) {
    s = v.StdString();
  } else if (v.IsInt64()) {
    s = std::to_string(v.Int64());
  } else if (v.IsInt32()) {
    s = std::to_string(v.Int32());
  } else if (v.IsNumber()) {
    s = std::to_string(static_cast<long>(v.Number()));
  } else if (v.IsBool()) {
    s = v.Bool() ? "true" : "false";
  } else {
    s = "<non-primitive>";
  }

  if (!g_log.empty()) {
    g_log.append("|");
  }
  g_log.append(s);
  return lepus::Value();
}

static void RegisterBuiltinForTest(lepus::VMContext* context) {
  lepus::RegisterCFunction(context, "_CreatePage", &EmptyFunc);
  lepus::RegisterCFunction(context, "print", &EmptyFunc);
  lepus::RegisterCFunction(context, "AppendResult", &AppendResult);
}

static std::string CompileAndRun(const std::string& source, bool opt_bytecode) {
  g_log.clear();
  auto* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);
  context->SetOptBytecode(opt_bytecode);

  auto error =
      lepus::BytecodeGenerator::GenerateBytecode(context, source, "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;
  context->Execute();
  const std::string log = g_log;
  delete context;
  return log;
}

static fml::RefPtr<lepus::Function> FindChildFunctionByName(
    const fml::RefPtr<lepus::Function>& root, const std::string& name) {
  if (!root) return nullptr;
  for (const auto& child : root->GetChildFunction()) {
    if (!child) continue;
    if (child->GetFunctionName() == name) {
      return child;
    }
  }
  return nullptr;
}

static std::vector<size_t> CollectOpcodeIndices(const lepus::Function* func,
                                                int32_t opcode) {
  std::vector<size_t> out;
  if (!func) return out;
  const auto* codes = func->GetOpCodes();
  const size_t n = const_cast<lepus::Function*>(func)->OpCodeSize();
  for (size_t i = 0; i < n; ++i) {
    lepus::Instruction inst = codes[i];
    if (lepus::Instruction::GetOpCode(inst) == opcode) {
      out.push_back(i);
    }
  }
  return out;
}

static std::optional<size_t> FindFirstOpcodeAfter(const lepus::Function* func,
                                                  int32_t opcode,
                                                  size_t start) {
  if (!func) return std::nullopt;
  const auto* codes = func->GetOpCodes();
  const size_t n = const_cast<lepus::Function*>(func)->OpCodeSize();
  for (size_t i = start; i < n; ++i) {
    lepus::Instruction inst = codes[i];
    if (lepus::Instruction::GetOpCode(inst) == opcode) {
      return i;
    }
  }
  return std::nullopt;
}

static std::optional<size_t> FindFirstLoadConstStringAfter(
    const lepus::Function* func, const std::string& s, size_t start) {
  if (!func) return std::nullopt;
  const auto* codes = func->GetOpCodes();
  const size_t n = const_cast<lepus::Function*>(func)->OpCodeSize();
  for (size_t i = start; i < n; ++i) {
    lepus::Instruction inst = codes[i];
    if (lepus::Instruction::GetOpCode(inst) != TypeOp_LoadConst) continue;
    const auto const_idx = lepus::Instruction::GetParamBx(inst);
    auto* v = const_cast<lepus::Function*>(func)->GetConstValue(const_idx);
    if (v && v->IsString() && v->StdString() == s) {
      return i;
    }
  }
  return std::nullopt;
}

static bool IsCallOpcode(int32_t opcode) {
  switch (opcode) {
    case TypeOp_Call:
    case TypeOp_Call1:
    case TypeOp_CallRandom:
    case TypeOp_CallRandom1:
    case TypeOp_CallRandom2:
    case TypeOp_CallRandom3:
      return true;
    default:
      return false;
  }
}

static std::optional<size_t> FindLastCallBefore(const lepus::Function* func,
                                                size_t end_exclusive) {
  if (!func) return std::nullopt;
  const auto* codes = func->GetOpCodes();
  const size_t n = const_cast<lepus::Function*>(func)->OpCodeSize();
  if (end_exclusive > n) end_exclusive = n;
  for (size_t i = end_exclusive; i > 0; --i) {
    const size_t idx = i - 1;
    lepus::Instruction inst = codes[idx];
    const auto op = lepus::Instruction::GetOpCode(inst);
    if (IsCallOpcode(static_cast<int32_t>(op))) {
      return idx;
    }
  }
  return std::nullopt;
}

TEST(LEPUSIRTryCatchFinallyLayout, try_catch_throw_layout_throw_before_catch) {
  g_log.clear();
  auto* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  const std::string source = R"(
    function f() {
      try {
        throw ("E");
      } catch (e) {
        AppendResult("C");
      }
    }
    f();
  )";

  auto error =
      lepus::BytecodeGenerator::GenerateBytecode(context, source, "3.8", "");
  ASSERT_TRUE(error.empty()) << "Compilation error: " << error;

  auto root = context->GetRootFunction();
  ASSERT_TRUE(root);
  auto f = FindChildFunctionByName(root, "f");
  ASSERT_TRUE(f);

  auto throws = CollectOpcodeIndices(f.get(), TypeLabel_Throw);
  ASSERT_FALSE(throws.empty());
  for (auto throw_idx : throws) {
    auto catch_idx =
        FindFirstOpcodeAfter(f.get(), TypeLabel_Catch, throw_idx + 1);
    ASSERT_TRUE(catch_idx.has_value())
        << "未在 Throw 之后找到 Catch Label，throw_idx=" << throw_idx;
    EXPECT_GT(*catch_idx, throw_idx);
  }

  context->Execute();
  EXPECT_EQ(g_log, "C");

  delete context;
}

TEST(LEPUSIRTryCatchFinallyLayout,
     nested_try_catch_first_catch_is_inner_and_ordered) {
  g_log.clear();
  auto* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  const std::string source = R"(
    function nested() {
      try {
        try {
          throw ("INNER");
        } catch (e) {
          AppendResult("C_IN");
        }
        throw ("OUTER");
      } catch (e) {
        AppendResult("C_OUT");
      }
    }
    nested();
  )";

  auto error =
      lepus::BytecodeGenerator::GenerateBytecode(context, source, "3.8", "");
  ASSERT_TRUE(error.empty()) << "Compilation error: " << error;

  auto root = context->GetRootFunction();
  ASSERT_TRUE(root);
  auto f = FindChildFunctionByName(root, "nested");
  ASSERT_TRUE(f);

  auto throws = CollectOpcodeIndices(f.get(), TypeLabel_Throw);
  ASSERT_GE(throws.size(), 2u);
  const size_t inner_throw = throws[0];
  const size_t outer_throw = throws[1];
  ASSERT_LT(inner_throw, outer_throw);

  auto inner_catch =
      FindFirstOpcodeAfter(f.get(), TypeLabel_Catch, inner_throw + 1);
  auto outer_catch =
      FindFirstOpcodeAfter(f.get(), TypeLabel_Catch, outer_throw + 1);
  ASSERT_TRUE(inner_catch.has_value());
  ASSERT_TRUE(outer_catch.has_value());

  // VM 的“向后扫描第一个 catch”语义要求：inner_throw 之后先遇到 inner catch。
  EXPECT_GT(*inner_catch, inner_throw);
  EXPECT_LT(*inner_catch, outer_throw)
      << "inner catch 必须位于 outer throw 之前，才能保证嵌套 try 的扫描顺序";

  EXPECT_GT(*outer_catch, outer_throw);
  EXPECT_GT(*outer_catch, *inner_catch);

  context->Execute();
  EXPECT_EQ(g_log, "C_IN|C_OUT");

  delete context;
}

TEST(LEPUSIRTryCatchFinallyLayout,
     try_catch_finally_layout_catch_then_finally_and_positions) {
  g_log.clear();
  auto* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  const std::string source = R"(
    function f(x) {
      try {
        AppendResult("T");
        x();
      } catch (e) {
        AppendResult("C");
      } finally {
        AppendResult("F");
      }
    }
    f(1);
  )";

  auto error =
      lepus::BytecodeGenerator::GenerateBytecode(context, source, "3.8", "");
  ASSERT_TRUE(error.empty()) << "Compilation error: " << error;

  auto root = context->GetRootFunction();
  ASSERT_TRUE(root);
  auto f = FindChildFunctionByName(root, "f");
  ASSERT_TRUE(f);

  // 结构验证：try 内潜在抛异常的 call(x()) 之后，必须能“向后扫描”到 catch
  // label。 由于 AppendResult("T") 也是 call，使用“第一个 catch label
  // 之前最后一个 call”作为锚点。
  auto catches = CollectOpcodeIndices(f.get(), TypeLabel_Catch);
  ASSERT_FALSE(catches.empty());
  const size_t catch_idx = catches.front();
  auto call_in_try = FindLastCallBefore(f.get(), catch_idx);
  ASSERT_TRUE(call_in_try.has_value());
  EXPECT_LT(*call_in_try, catch_idx);

  // catch/finally 代码块应当在 catch label 之后出现，并且 catch 的内容在
  // finally 之前。
  auto load_c = FindFirstLoadConstStringAfter(f.get(), "C", catch_idx + 1);
  auto load_f = FindFirstLoadConstStringAfter(f.get(), "F", catch_idx + 1);
  ASSERT_TRUE(load_c.has_value());
  ASSERT_TRUE(load_f.has_value());
  EXPECT_GT(*load_c, catch_idx);
  EXPECT_GT(*load_f, catch_idx);
  EXPECT_LT(*load_c, *load_f);

  context->Execute();
  EXPECT_EQ(g_log, "T|C|F");

  delete context;
}

TEST(LEPUSIRTryCatchFinallyLayout,
     nested_try_catch_finally_inner_before_outer_and_finally_ordered) {
  g_log.clear();
  auto* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  const std::string source = R"(
    function nested(x) {
      try {
        try {
          AppendResult("T");
          x();
        } catch (e) {
          AppendResult("C_IN");
          throw (e);
        } finally {
          AppendResult("F_IN");
        }
      } catch (e) {
        AppendResult("C_OUT");
      } finally {
        AppendResult("F_OUT");
      }
    }
    nested(1);
  )";

  auto error =
      lepus::BytecodeGenerator::GenerateBytecode(context, source, "3.8", "");
  ASSERT_TRUE(error.empty()) << "Compilation error: " << error;

  auto root = context->GetRootFunction();
  ASSERT_TRUE(root);
  auto fn = FindChildFunctionByName(root, "nested");
  ASSERT_TRUE(fn);

  // 结构验证：嵌套 try 的 inner catch label 必须出现在 outer catch label 之前。
  auto catches = CollectOpcodeIndices(fn.get(), TypeLabel_Catch);
  ASSERT_GE(catches.size(), 2u);
  const size_t inner_catch = catches[0];
  const size_t outer_catch = catches[1];
  EXPECT_LT(inner_catch, outer_catch);

  // inner catch/finally 的代码锚点顺序：C_IN 在 F_IN 之前，并且都在 inner catch
  // label 之后。
  auto t = FindFirstLoadConstStringAfter(fn.get(), "T", 0);
  auto c_in = FindFirstLoadConstStringAfter(fn.get(), "C_IN", 0);
  auto f_in = FindFirstLoadConstStringAfter(fn.get(), "F_IN", 0);
  ASSERT_TRUE(c_in.has_value());
  ASSERT_TRUE(f_in.has_value());
  ASSERT_TRUE(t.has_value());
  EXPECT_GT(inner_catch, *t);
  EXPECT_GT(*f_in, *t) << "finally 必须在 try 之后落位";
  EXPECT_GT(*c_in, inner_catch) << "catch 代码必须在 catch label 之后";

  // rethrow 的 Throw Label 应当位于 inner try handler 之后、outer catch label
  // 之前， 从而保证 VM 向后扫描先命中 outer catch。
  auto throws = CollectOpcodeIndices(fn.get(), TypeLabel_Throw);
  ASSERT_FALSE(throws.empty());
  bool has_rethrow_between = false;
  for (auto t : throws) {
    if (t > inner_catch && t < outer_catch) {
      has_rethrow_between = true;
      break;
    }
  }
  EXPECT_TRUE(has_rethrow_between);

  // outer catch/finally 的代码锚点顺序：C_OUT 在 F_OUT 之前，并且都在 outer
  // catch label 之后。
  auto c_out = FindFirstLoadConstStringAfter(fn.get(), "C_OUT", 0);
  auto f_out = FindFirstLoadConstStringAfter(fn.get(), "F_OUT", 0);
  ASSERT_TRUE(c_out.has_value());
  ASSERT_TRUE(f_out.has_value());
  EXPECT_GT(*c_out, outer_catch);
  EXPECT_GT(*f_out, *t) << "outer finally 必须在 outer try 之后落位";

  context->Execute();
  const std::string opt_log = g_log;

  // 行为对齐验证：打开 IR 优化后（opt=true）不应改变 try/catch/finally
  // 的执行序。 这里以 baseline(opt=false) 的结果作为真值。
  const std::string baseline = CompileAndRun(source, false);
  EXPECT_EQ(opt_log, baseline);

  delete context;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
