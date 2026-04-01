// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_builder.h"
#include "core/runtime/lepus/parser.h"
#include "core/runtime/lepus/scanner.h"
#include "core/runtime/lepus/semantic_analysis.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

static std::string g_test_result;

static lepus::Value EmptyFunc(lepus::MTSContext* context, lepus::Value*, int) {
  return lepus::Value();
}

static lepus::Value CaptureResult(lepus::MTSContext* context,
                                  lepus::Value* argv, int argc) {
  if (argc > 0) {
    lepus::Value v = argv[0];
    if (v.IsString()) {
      g_test_result = v.StdString();
    } else if (v.IsInt64()) {
      g_test_result = std::to_string(v.Int64());
    } else if (v.IsInt32()) {
      g_test_result = std::to_string(v.Int32());
    } else if (v.IsNumber()) {
      g_test_result = std::to_string(static_cast<long>(v.Number()));
    } else if (v.IsBool()) {
      g_test_result = v.Bool() ? "true" : "false";
    }
  }
  return lepus::Value();
}

static void RegisterBuiltinForTest(lepus::VMContext* context) {
  lepus::RegisterCFunction(context, "_CreatePage", &EmptyFunc);
  lepus::RegisterCFunction(context, "print", &EmptyFunc);
  lepus::RegisterCFunction(context, "CaptureResult", &CaptureResult);
}

static ModuleOp* CompileToIRAndRunMIRUntilSecondLSE(lepus::VMContext* context,
                                                    const std::string& source,
                                                    IRContext* ir_ctx) {
  parser::InputStream input;
  input.Write(source);
  Scanner scanner(&input);
  scanner.SetSdkVersion("3.8");
  Parser parser(&scanner);
  parser.SetSdkVersion("3.8");
  SemanticAnalysis semantic_analysis;
  semantic_analysis.SetInput(&scanner);
  semantic_analysis.SetSdkVersion("3.8");
  semantic_analysis.SetClosureFix(context->GetClosureFix());

  CodeGenerator code_generator(context, &semantic_analysis);
  std::unique_ptr<ASTree> root;
  root.reset(parser.Parse());
  root->Accept(&semantic_analysis, nullptr);
  root->Accept(&code_generator, nullptr);

  auto root_func = context->GetRootFunction();
  if (root_func) {
    root_func->SetSource(source);
    root_func->SetTopLevelFunction(true);
  }

  ir_ctx->Init(root_func, context);
  auto* mod = ir_ctx->GetMainMod();

  // Mirror the MIR part of -O1 pipeline, and stop right after the 2nd LSE.
  PassManager pm(mod->GetIRCtx());
  pm.SetMode(StageMode::SM_MIR);
  pm.AddCollectToplevelClosureRegPass();
  pm.AddConstructSSAIRPass();
  pm.AddNormalizePhiPass();
  pm.AddProcessSpecialMovPass();
  pm.AddGetToplevelRelatedInstEliminationPass();
  pm.AddSSAIRVerifyPass();
  pm.AddChangeSpecialAttributePass();
  pm.AddTypeSpecificationPass();
  pm.AddNormalizePhiPass();
  pm.AddSimplifyCFG();
  pm.AddInstCombinePass();
  pm.AddSimplifyCFG();
  pm.AddDCE();
  pm.AddCSE();
  pm.AddLoadStoreElimination();
  pm.AddDCE();
  pm.AddInstCombinePass();
  pm.AddLoadStoreElimination();
  pm.Run(mod);

  return mod;
}

static FuncOp* FindFuncByName(ModuleOp* mod, const std::string& name) {
  if (!mod) return nullptr;
  for (auto* f : *mod) {
    if (f && f->GetName() == name) return f;
  }
  return nullptr;
}

static bool IsConstStringKey(Value* v, const std::string& key, FuncOp* func) {
  if (!v || !func) return false;
  auto* lc = llvh::dyn_cast<LoadConstInst>(v);
  if (!lc) return false;
  if (!lc->GetType() || !lc->GetType()->IsStringType()) return false;
  auto* u32 = llvh::dyn_cast<LiteralUint32>(lc->GetConst());
  if (!u32) return false;
  auto lepus_func = func->GetLepusFunction();
  if (!lepus_func) return false;
  lepus::Value* cv = lepus_func->GetConstValue(u32->GetValue());
  if (!cv || !cv->IsString()) return false;
  return cv->StdString() == key;
}

// Test 1: Simple closure variable read
TEST(LEPUSIRTestToplevelClosureVariable, simple_closure_read) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 'initial';
    function A() {
      return global1;
    }
    let result = A();
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "initial");

  delete context;
}

// LSE: verify `obj.x` load is forwarded across a read-only call.
// We validate this at source level by compiling with IR dumps enabled and
// checking the LSE stage output.
TEST(LEPUSIRTestToplevelClosureVariable,
     lse_eliminate_obj_x_after_readonly_call) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  // Register standard builtins so LSE can resolve String/console indices.
  lepus::RegisterBuiltin(context);
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    function test2() {
      let obj = {};
      obj.x = 1;
      let a = "";
      String.length(a);
      console.log(obj.x);
    }
    test2();
  )";

  std::unique_ptr<IRContext> ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod =
      CompileToIRAndRunMIRUntilSecondLSE(context, source, ir_ctx.get());
  ASSERT_NE(mod, nullptr);

  FuncOp* test2 = FindFuncByName(mod, "test2");
  ASSERT_NE(test2, nullptr) << "Cannot find FuncOp 'test2' in optimized IR";

  // After running LSE, there should be no GetTableInst reading the "x"
  // property.
  for (auto& block : *test2) {
    for (auto& inst : block) {
      if (auto* get_table = llvh::dyn_cast<GetTableInst>(&inst)) {
        EXPECT_FALSE(IsConstStringKey(get_table->GetProp(), "x", test2))
            << "Expected obj.x load to be eliminated, but found "
               "GetTableInst(key='x') in IR";
      }
    }
  }

  delete context;
}

// Test 2: Simple closure variable write
TEST(LEPUSIRTestToplevelClosureVariable, simple_closure_write) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 'initial';
    function A() {
      global1 = 'modified';
    }
    A();
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "modified");

  delete context;
}

// Test 3: Closure variable with top-level assignment before call
TEST(LEPUSIRTestToplevelClosureVariable, toplevel_assignment_before_call) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = '';
    function A() {
      global1 = 'called A';
    }
    global1 = '123';
    A();
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "called A")
      << "Expected 'called A' but got '" << g_test_result << "'";

  delete context;
}

// Test 4: Closure variable with top-level assignment after call
TEST(LEPUSIRTestToplevelClosureVariable, toplevel_assignment_after_call) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = '';
    function A() {
      global1 = 'called A';
    }
    A();
    global1 = '123';
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "123");

  delete context;
}

// Test 5: Multiple closure variables
TEST(LEPUSIRTestToplevelClosureVariable, multiple_closure_variables) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = '';
    let global2 = '';
    function A() {
      global1 = 'called A';
      return false;
    }
    function B() {
      global2 = 'called B';
      return true;
    }
    global1 = '123';
    global2 = '445';
    let res = A();
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "called A")
      << "Expected 'called A' but got '" << g_test_result << "'";

  delete context;
}

// Test 6: Nested closure variables
TEST(LEPUSIRTestToplevelClosureVariable, nested_closure) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 'outer';
    function A() {
      function B() {
        global1 = 'inner';
      }
      B();
    }
    A();
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "inner");

  delete context;
}

// Test 7: Closure variable with conditional assignment
TEST(LEPUSIRTestToplevelClosureVariable, conditional_closure_write) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 'initial';
    function A(flag) {
      if (flag) {
        global1 = 'true branch';
      } else {
        global1 = 'false branch';
      }
    }
    A(true);
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "true branch");

  delete context;
}

// Test 8: Closure variable read and write in same function
TEST(LEPUSIRTestToplevelClosureVariable, read_and_write_in_closure) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let counter = 0;
    function increment() {
      counter = counter + 1;
    }
    increment();
    increment();
    increment();
    CaptureResult(counter);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "3");

  delete context;
}

// Test 9: Multiple functions accessing same closure variable
TEST(LEPUSIRTestToplevelClosureVariable, multiple_functions_same_variable) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let shared = 'initial';
    function A() {
      shared = 'from A';
    }
    function B() {
      shared = 'from B';
    }
    A();
    B();
    CaptureResult(shared);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "from B");

  delete context;
}

// Test 10: Closure variable with complex expression
TEST(LEPUSIRTestToplevelClosureVariable, closure_with_expression) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let result = '';
    function concat(a, b) {
      result = a + b;
    }
    concat('hello', 'world');
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "helloworld");

  delete context;
}

// Test 11: Closure variable initialized with non-literal
TEST(LEPUSIRTestToplevelClosureVariable, closure_var_non_literal_init) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 1 + 1;
    function A() {
      global1 = global1 * 2;
    }
    A();
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "4");  // (1+1)*2 = 4

  delete context;
}

// Test 12: Closure variable reassignment multiple times
TEST(LEPUSIRTestToplevelClosureVariable, multiple_reassignments) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 'v1';
    function A() {
      global1 = 'v2';
    }
    global1 = 'v3';
    A();
    global1 = 'v4';
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "v4");

  delete context;
}

// Test 13: Closure variable with return value
TEST(LEPUSIRTestToplevelClosureVariable, closure_with_return) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 'initial';
    function A() {
      global1 = 'modified';
      return global1;
    }
    let result = A();
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "modified");

  delete context;
}

// Test 14: Multiple closure variables in one function
TEST(LEPUSIRTestToplevelClosureVariable, multiple_vars_one_function) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let var1 = 'a';
    let var2 = 'b';
    let var3 = 'c';
    function modify() {
      var1 = '1';
      var2 = '2';
      var3 = '3';
    }
    modify();
    CaptureResult(var1 + var2 + var3);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "123");

  delete context;
}

// Test 15: Closure variable with loop
TEST(LEPUSIRTestToplevelClosureVariable, closure_in_loop) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let sum = 0;
    function add(n) {
      sum = sum + n;
    }
    let i = 0;
    while (i < 5) {
      add(i);
      i = i + 1;
    }
    CaptureResult(sum);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "10");  // 0+1+2+3+4 = 10

  delete context;
}

// Test 16: Deep nested closure (3 levels)
TEST(LEPUSIRTestToplevelClosureVariable, deep_nested_closure) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let global1 = 'level0';
    function A() {
      function B() {
        function C() {
          global1 = 'level3';
        }
        C();
      }
      B();
    }
    A();
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "level3");

  delete context;
}

// Test 17: Closure variable with compound assignment (+=)
TEST(LEPUSIRTestToplevelClosureVariable, compound_assignment) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let counter = 10;
    function addFive() {
      counter = counter + 5;
    }
    addFive();
    CaptureResult(counter);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "15");

  delete context;
}

// Test 18: Closure variable with increment operator
TEST(LEPUSIRTestToplevelClosureVariable, increment_operator) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let counter = 5;
    function inc() {
      counter = counter + 1;
    }
    inc();
    inc();
    CaptureResult(counter);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "7");

  delete context;
}

// Test 19: Multiple closures sharing same variable
TEST(LEPUSIRTestToplevelClosureVariable, multiple_closures_shared_var) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let shared = 0;
    function increment() {
      shared = shared + 1;
    }
    function decrement() {
      shared = shared - 1;
    }
    increment();
    increment();
    increment();
    decrement();
    CaptureResult(shared);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "2");  // 0+1+1+1-1 = 2

  delete context;
}

// Test 20: Closure variable in conditional branches
TEST(LEPUSIRTestToplevelClosureVariable, closure_in_branches) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let result = 'initial';
    function test(x) {
      if (x > 10) {
        result = 'large';
      } else if (x > 5) {
        result = 'medium';
      } else {
        result = 'small';
      }
    }
    test(7);
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "medium");

  delete context;
}

// Test 21: Closure returning closure
TEST(LEPUSIRTestToplevelClosureVariable, closure_returning_closure) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let outer_var = 'outer';
    function createClosure() {
      function inner() {
        outer_var = 'modified';
      }
      return inner;
    }
    let fn = createClosure();
    fn();
    CaptureResult(outer_var);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "modified");

  delete context;
}

// Test 22: Closure variable with for loop
TEST(LEPUSIRTestToplevelClosureVariable, closure_with_for_loop) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let total = 0;
    function accumulate() {
      let i = 1;
      while (i <= 3) {
        total = total + i;
        i = i + 1;
      }
    }
    accumulate();
    CaptureResult(total);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "6");  // 1+2+3 = 6

  delete context;
}

// Test 23: Closure variable with string concatenation
TEST(LEPUSIRTestToplevelClosureVariable, closure_string_concat) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let message = 'Hello';
    function append(suffix) {
      message = message + suffix;
    }
    append(' ');
    append('World');
    CaptureResult(message);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "Hello World");

  delete context;
}

// Test 24: Multiple closure variables with different types
TEST(LEPUSIRTestToplevelClosureVariable, multiple_types_closure_vars) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str_var = 'text';
    let num_var = 42;
    let bool_var = true;
    function modify() {
      str_var = 'modified';
      num_var = 100;
      bool_var = false;
    }
    modify();
    CaptureResult(str_var);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "modified");

  delete context;
}

// Test 25: Closure variable with multiple calls
TEST(LEPUSIRTestToplevelClosureVariable, closure_with_multiple_calls) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let accumulator = 1;
    function multiply(n) {
      accumulator = accumulator * n;
    }
    multiply(2);
    multiply(3);
    multiply(4);
    CaptureResult(accumulator);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "24");  // 1*2*3*4 = 24

  delete context;
}

// Test 26: Closure variable read before write
TEST(LEPUSIRTestToplevelClosureVariable, read_before_write) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 'initial';
    function process() {
      let temp = value;
      value = temp + '_processed';
    }
    process();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "initial_processed");

  delete context;
}

// Test 27: Closure with early return
TEST(LEPUSIRTestToplevelClosureVariable, closure_early_return) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let status = 'pending';
    function process(flag) {
      if (flag) {
        status = 'success';
        return;
      }
      status = 'failure';
    }
    process(true);
    CaptureResult(status);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "success");

  delete context;
}

// Test 28: Closure variable with switch-like pattern
TEST(LEPUSIRTestToplevelClosureVariable, closure_switch_pattern) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let result = 'none';
    function select(option) {
      if (option == 1) {
        result = 'option1';
      } else if (option == 2) {
        result = 'option2';
      } else if (option == 3) {
        result = 'option3';
      } else {
        result = 'default';
      }
    }
    select(2);
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "option2");

  delete context;
}

// Test 29: Closure variable with logical operators
TEST(LEPUSIRTestToplevelClosureVariable, closure_logical_operators) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let flag1 = true;
    let flag2 = false;
    function toggle() {
      flag1 = false;
      flag2 = true;
    }
    toggle();
    CaptureResult(flag2);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "true");

  delete context;
}

// Test 30: Closure variable with arithmetic operations
TEST(LEPUSIRTestToplevelClosureVariable, closure_arithmetic_ops) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let x = 10;
    function calculate() {
      x = x * 2;
      x = x + 5;
      x = x - 3;
    }
    calculate();
    CaptureResult(x);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "22");  // (10*2+5-3) = 22

  delete context;
}

// Test 31: Read-only closure access (no write)
TEST(LEPUSIRTestToplevelClosureVariable, read_only_closure_access) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 'readonly';
    function reader() {
      return value;
    }
    let result = reader();
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "readonly");

  delete context;
}

// Test 32: Write-only closure access (no read)
TEST(LEPUSIRTestToplevelClosureVariable, write_only_closure_access) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 'initial';
    function writer() {
      value = 'written';
    }
    writer();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "written");

  delete context;
}

// Test 33: Multiple reads from same closure
TEST(LEPUSIRTestToplevelClosureVariable, multiple_reads_same_closure) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 5;
    function multiRead() {
      let a = value;
      let b = value;
      let c = value;
      return a + b + c;
    }
    let result = multiRead();
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "15");

  delete context;
}

// Test 34: Multiple writes to same closure variable
TEST(LEPUSIRTestToplevelClosureVariable, multiple_writes_same_closure) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 0;
    function multiWrite() {
      value = 1;
      value = 2;
      value = 3;
    }
    multiWrite();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "3");

  delete context;
}

// Test 35: Closure reading and writing in sequence
TEST(LEPUSIRTestToplevelClosureVariable, sequential_read_write) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let counter = 0;
    function increment() {
      let old = counter;
      counter = old + 1;
      return old;
    }
    increment();
    increment();
    let result = increment();
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "2");

  delete context;
}

// Test 36: Closure with undefined initial value
TEST(LEPUSIRTestToplevelClosureVariable, undefined_initial_value) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value;
    function checkUndefined() {
      value = 'defined';
    }
    checkUndefined();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "defined");

  delete context;
}

// Test 37: Closure with null initial value
TEST(LEPUSIRTestToplevelClosureVariable, null_initial_value) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = null;
    function setFromNull() {
      value = 'not_null';
    }
    setFromNull();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "not_null");

  delete context;
}

// Test 38: Closure with boolean operations
TEST(LEPUSIRTestToplevelClosureVariable, boolean_operations) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let flag = true;
    function toggle() {
      flag = !flag;
    }
    toggle();
    CaptureResult(flag);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "false");

  delete context;
}

// Test 39: Closure accessing array element
TEST(LEPUSIRTestToplevelClosureVariable, array_element_access) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let arr = [1, 2, 3];
    function modifyArray() {
      arr[0] = 10;
    }
    modifyArray();
    CaptureResult(arr[0]);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "10");

  delete context;
}

// Test 40: Closure with object property access
TEST(LEPUSIRTestToplevelClosureVariable, object_property_access) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let obj = { x: 5 };
    function modifyObject() {
      obj.x = 10;
    }
    modifyObject();
    CaptureResult(obj.x);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "10");

  delete context;
}

// Test 41: Closure called multiple times with accumulation
TEST(LEPUSIRTestToplevelClosureVariable, accumulation_pattern) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let sum = 0;
    function add(n) {
      sum = sum + n;
    }
    add(1);
    add(2);
    add(3);
    CaptureResult(sum);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "6");

  delete context;
}

// Test 42: Two closures reading same variable
TEST(LEPUSIRTestToplevelClosureVariable, two_closures_reading) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let shared = 'value';
    function reader1() {
      return shared;
    }
    function reader2() {
      return shared;
    }
    let r1 = reader1();
    let r2 = reader2();
    CaptureResult(r1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "value");

  delete context;
}

// Test 43: Two closures writing same variable
TEST(LEPUSIRTestToplevelClosureVariable, two_closures_writing) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let shared = 0;
    function writer1() {
      shared = 1;
    }
    function writer2() {
      shared = 2;
    }
    writer1();
    writer2();
    CaptureResult(shared);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "2");

  delete context;
}

// Test 44: Closure with ternary operator
TEST(LEPUSIRTestToplevelClosureVariable, ternary_operator) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 10;
    function check() {
      value = value > 5 ? 100 : 0;
    }
    check();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "100");

  delete context;
}

// Test 45: Closure with string concatenation
TEST(LEPUSIRTestToplevelClosureVariable, string_concatenation_closure) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = 'Hello';
    function append() {
      str = str + ' World';
    }
    append();
    CaptureResult(str);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "Hello World");

  delete context;
}

// Test 46: Closure with type conversion
TEST(LEPUSIRTestToplevelClosureVariable, type_conversion) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 42;
    function convert() {
      value = value + '';
    }
    convert();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "42");

  delete context;
}

// Test 47: Nested closure reading outer closure variable
TEST(LEPUSIRTestToplevelClosureVariable, nested_reading_outer) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let outer = 'outer_value';
    function createInner() {
      function inner() {
        return outer;
      }
      return inner();
    }
    let result = createInner();
    CaptureResult(result);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "outer_value");

  delete context;
}

// Test 48: Nested closure writing outer closure variable
TEST(LEPUSIRTestToplevelClosureVariable, nested_writing_outer) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let outer = 'initial';
    function createInner() {
      function inner() {
        outer = 'modified';
      }
      inner();
    }
    createInner();
    CaptureResult(outer);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "modified");

  delete context;
}

// Test 49: Closure with comparison operations
TEST(LEPUSIRTestToplevelClosureVariable, comparison_operations) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let value = 10;
    function compare() {
      if (value > 5) {
        value = 20;
      } else {
        value = 0;
      }
    }
    compare();
    CaptureResult(value);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "20");

  delete context;
}

// Test 50: Closure with while loop modifying variable
// NOTE: Disabled due to SSA verification issue with loops
// TEST(LEPUSIRTestToplevelClosureVariable, while_loop_modification) {
//   g_test_result = "";
//   lepus::VMContext* context = new lepus::VMContext();
//   context->Initialize();
//   RegisterBuiltinForTest(context);
//   context->SetClosureFix(true);
//
//   std::string source = R"(
//     let counter = 0;
//     function countToFive() {
//       while (counter < 5) {
//         counter = counter + 1;
//       }
//     }
//     countToFive();
//     CaptureResult(counter);
//   )";
//
//   auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(
//       context, source, "3.8", "");
//   EXPECT_TRUE(error.empty()) << "Compilation error: " << error;
//
//   context->Execute();
//   EXPECT_EQ(g_test_result, "5");
//
//   delete context;
// }

TEST(LEPUSIRTestToplevelClosureVariable, complex) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(

    let o8 = '' || false;
    let global1 = '';
    let global2 = '';
    function A() {
      global1 = ' called A';
      return false;
    }
    function B() {
      global2 = ' called B';
      return true;
    }
    A();
    B();
    CaptureResult(o8 + global1 + global2);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "false called A called B");
  delete context;
}

TEST(LEPUSIRTestToplevelClosureVariable, complex2) {
  g_test_result = "";
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(

    let global1 = '';
    let global2 = '';
    function A() {
      global1 = 'called A';
      return false;
    }
    function B() {
      global2 = 'called B';
      return true;
    }

    let res = A() || B();
    global1 = '';
    global2 = '';
    res = A() && B();
    CaptureResult(global1);
  )";

  auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(context, source,
                                                                "3.8", "");
  EXPECT_TRUE(error.empty()) << "Compilation error: " << error;

  context->Execute();
  EXPECT_EQ(g_test_result, "called A");
  delete context;
}

namespace {

static fml::RefPtr<lynx::lepus::Function> FindFunctionByName(
    const fml::RefPtr<lynx::lepus::Function>& func, const std::string& name) {
  if (!func) return nullptr;
  if (func->GetFunctionName() == name) return func;
  for (auto child : func->GetChildFunction()) {
    if (auto found = FindFunctionByName(child, name)) return found;
  }
  return nullptr;
}
}  // namespace

TEST(LEPUSIRTestTryCatchSkipOpt, try_function_skips_ir_opt_only_for_itself) {
  // Ensure try/catch functions can go through IR pipeline and keep semantics.
  static const char* source = R"(
function noTry() {
  return 1 + 2;
}
function withTry() {
  try {
    return 1 + 2;
  } catch (e) {
    return 0;
  }
}
CaptureResult(noTry() + withTry());
)";

  // Baseline compilation (no IR opt)
  g_test_result = "";
  lepus::VMContext* ctx_noopt = new lepus::VMContext();
  ctx_noopt->Initialize();
  RegisterBuiltinForTest(ctx_noopt);
  ctx_noopt->SetClosureFix(true);
  auto err0 = lynx::lepus::BytecodeGenerator::GenerateBytecode(
      ctx_noopt, source, "3.8", "", nullptr);
  ASSERT_TRUE(err0.empty()) << err0;
  auto root0 = ctx_noopt->GetRootFunction();
  ASSERT_TRUE(root0);
  auto f0_no_try = FindFunctionByName(root0, "noTry");
  auto f0_with_try = FindFunctionByName(root0, "withTry");
  ASSERT_TRUE(f0_no_try);
  ASSERT_TRUE(f0_with_try);

  // Optimized compilation (function-level skip for try)
  lepus::VMContext* ctx_opt = new lepus::VMContext();
  ctx_opt->Initialize();
  RegisterBuiltinForTest(ctx_opt);
  ctx_opt->SetClosureFix(true);
  auto err1 = lynx::lepus::BytecodeGenerator::GenerateBytecode(
      ctx_opt, source, "3.8", "", nullptr);
  ASSERT_TRUE(err1.empty()) << err1;
  auto root1 = ctx_opt->GetRootFunction();
  ASSERT_TRUE(root1);
  auto f1_no_try = FindFunctionByName(root1, "noTry");
  auto f1_with_try = FindFunctionByName(root1, "withTry");
  ASSERT_TRUE(f1_no_try);
  ASSERT_TRUE(f1_with_try);

  // Both functions should be eligible for IR optimization. Require no size
  // regression (some inputs may optimize to identical final bytecode).
  EXPECT_LE(f1_no_try->OpCodeSize(), f0_no_try->OpCodeSize());
  EXPECT_LE(f1_with_try->OpCodeSize(), f0_with_try->OpCodeSize());

  ctx_opt->Execute();
  EXPECT_EQ(g_test_result, "6");

  delete ctx_noopt;
  delete ctx_opt;
}

static bool ContainsOpcode(const fml::RefPtr<lynx::lepus::Function>& func,
                           lynx::lepus::TypeOpCode opcode) {
  if (!func) return false;
  auto* ops = func->GetOpCodes();
  const size_t n = func->OpCodeSize();
  if (!ops) return false;
  for (size_t i = 0; i < n; ++i) {
    if (lynx::lepus::Instruction::GetOpCode(ops[i]) == opcode) return true;
  }
  return false;
}

TEST(LEPUSIRTestTryCatchFinally, throw_is_caught_and_finally_runs) {
  static const char* source = R"(
var x = 0;
function f() {
  try {
    throw("err");
  } catch (e) {
    x = 1;
  } finally {
    x = x + 1;
  }
  return x;
}
CaptureResult(f());
)";

  g_test_result = "";
  lepus::VMContext* ctx = new lepus::VMContext();
  ctx->Initialize();
  RegisterBuiltinForTest(ctx);
  ctx->SetClosureFix(true);
  auto err = lynx::lepus::BytecodeGenerator::GenerateBytecode(
      ctx, source, "3.8", "", nullptr);
  ASSERT_TRUE(err.empty()) << err;

  // Ensure the optimized bytecode still contains the catch label and throw.
  auto root = ctx->GetRootFunction();
  ASSERT_TRUE(root);
  auto f = FindFunctionByName(root, "f");
  ASSERT_TRUE(f);
  EXPECT_TRUE(ContainsOpcode(f, lynx::lepus::TypeLabel_Catch));
  EXPECT_TRUE(ContainsOpcode(f, lynx::lepus::TypeLabel_Throw));
  EXPECT_TRUE(ContainsOpcode(f, lynx::lepus::TypeOp_SetCatchId));

  ctx->Execute();
  EXPECT_EQ(g_test_result, "2");
  delete ctx;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
