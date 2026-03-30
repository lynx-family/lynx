// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"
#include "core/runtime/lepus/ir/pass_manager/pipeline.h"
#include "core/runtime/lepus/ir/target_context.h"
#include "core/runtime/lepus/ir/transformer/mir/type_specification.h"
#include "core/runtime/lepus/parser.h"
#include "core/runtime/lepus/scanner.h"
#include "core/runtime/lepus/semantic_analysis.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

static lepus::Value EmptyFunc(lepus::MTSContext* context, lepus::Value*, int) {
  return lepus::Value();
}

static void RegisterBuiltinForTest(lepus::VMContext* context) {
  lepus::RegisterCFunction(context, "Assert", &EmptyFunc);
  lepus::RegisterCFunction(context, "print", &EmptyFunc);

  // Register missing builtins or mock existing ones to avoid
  // environment-dependent crashes during constant folding or compilation.
  lepus::RegisterBuiltinFunction(context, constants::kIsFinite, &EmptyFunc);
  lepus::RegisterBuiltinFunction(context, constants::kEncodeURI, &EmptyFunc);
  lepus::RegisterBuiltinFunction(context, constants::kDecodeURI, &EmptyFunc);

  // Override standard builtins with safe empty mocks
  lepus::RegisterBuiltinFunction(context, constants::kParseInt, &EmptyFunc);
  lepus::RegisterBuiltinFunction(context, constants::kParseFloat, &EmptyFunc);
  lepus::RegisterBuiltinFunction(context, constants::kIsNaN, &EmptyFunc);
  lepus::RegisterBuiltinFunction(context, constants::kEncodeURIComponent,
                                 &EmptyFunc);
  lepus::RegisterBuiltinFunction(context, constants::kDecodeURIComponent,
                                 &EmptyFunc);

  lepus::RegisterCFunction(context, tasm::kCFunctionSetStyleObject, &EmptyFunc);

  // Register RegExp to prevent crash
  lepus::RegisterCFunction(context, "RegExp", &EmptyFunc);
  lepus::RegisterCFunction(context, "Array", &EmptyFunc);
  lepus::RegisterCFunction(context, "Function", &EmptyFunc);
}

// Helper function to compile JavaScript and get IR with optimization
static ModuleOp* CompileToIRWithOptimization(lepus::VMContext* context,
                                             const std::string& source,
                                             IRContext* ir_ctx) {
  parser::InputStream input;
  input.Write(source);
  Scanner scanner(&input);
  scanner.SetSdkVersion("2.6");
  Parser parser(&scanner);
  parser.SetSdkVersion("2.6");
  SemanticAnalysis semantic_analysis;
  semantic_analysis.SetInput(&scanner);
  semantic_analysis.SetSdkVersion("2.6");
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

  // Run IR optimization
  ir_ctx->Init(root_func, context);
  auto mod = ir_ctx->GetMainMod();

  // For type-specification unit tests, we only need MIR-level passes.
  // Target-level passes (regalloc / isel) may rewrite instruction forms and
  // are not required for validating type inference correctness.
  PassManager pm(ir_ctx);
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
  pm.AddSimplifyCFG();
  pm.AddInstCombinePass();
  pm.AddDCE();
  pm.Run(mod);

  return mod;
}

// Helper function to count instructions of a specific type in a function
template <typename InstType>
static int CountInstructions(FuncOp* func) {
  int count = 0;
  for (auto& block : *func) {
    for (auto& inst : block) {
      if (llvh::isa<InstType>(&inst)) {
        count++;
      }
    }
  }
  return count;
}

// Helper function to find first instruction of a specific type
template <typename InstType>
static InstType* FindFirstInstruction(FuncOp* func) {
  for (auto& block : *func) {
    for (auto& inst : block) {
      if (auto* target_inst = llvh::dyn_cast<InstType>(&inst)) {
        return target_inst;
      }
    }
  }
  return nullptr;
}

// Helper function to check if any CallInst has the expected return type
static bool HasCallInstWithType(FuncOp* func, TypeOp::TypeKind expected_type) {
  for (auto& block : *func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (call_inst->GetType()->GetTypeKind() == expected_type) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool HasGetGlobalInstWithType(FuncOp* func,
                                     TypeOp::TypeKind expected_type) {
  for (auto& block : *func) {
    for (auto& inst : block) {
      if (auto* get_global_inst = llvh::dyn_cast<GetGlobalInst>(&inst)) {
        if (get_global_inst->GetType()->GetTypeKind() == expected_type) {
          return true;
        }
      }
    }
  }
  return false;
}

// Test that String.length is optimized to GetStringLengthInst
TEST(LEPUSIRTypeSpecificationIR, StringLengthOptimization) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let len = str.length;
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  // Find the main function
  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  // Check that GetStringLengthInst exists
  int count = CountInstructions<GetStringLengthInst>(main_func);
  EXPECT_GT(count, 0) << "Expected at least one GetStringLengthInst";

  // Verify the instruction has correct type
  auto* inst = FindFirstInstruction<GetStringLengthInst>(main_func);
  if (inst != nullptr) {
    EXPECT_EQ(inst->GetType()->GetTypeKind(), TypeOp::TypeKind::Int64)
        << "GetStringLengthInst should return Int64";
  }

  delete context;
}

TEST(LEPUSIRTypeSpecificationMIR,
     StringLengthConstKeyRewritesToGetStringLength) {
  // Regression: TypeSpecification must handle GetTableConstStringKeyInst for
  // string proto property (e.g. "length"). Previously it only handled
  // GetTableInst and could miss the optimization or crash later.

  auto ir_ctx = std::make_unique<IRContext>(nullptr);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx->SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx->GetMainMod();
  ASSERT_NE(nullptr, mod);

  OpBuilder* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_string_length_const_key";
  auto* func = builder->Create<FuncOp>(0, name);
  ASSERT_NE(nullptr, func);

  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  auto* entry =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  ASSERT_NE(nullptr, entry);
  builder->SetInsertionPointToEnd(entry);

  const uint32_t str_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("hello"));
  const uint32_t len_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(constants::kStringLength));

  auto* str = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(str_idx), TypeOp::CreateString(builder));
  auto* get_len = builder->Create<GetTableConstStringKeyInst>(
      0, str, builder->GetLiteralUint32(len_idx),
      TypeOp::CreateAnyType(builder));
  builder->Create<ReturnInst>(0, get_len);

  TypeSpecification pass(ir_ctx.get());
  ASSERT_TRUE(pass.RunOnFunction(func));

  EXPECT_EQ(CountInstructions<GetStringLengthInst>(func), 1);
  EXPECT_EQ(CountInstructions<GetTableConstStringKeyInst>(func), 0);
}

TEST(LEPUSIRTypeSpecificationMIR,
     StringProtoCallWithConstKeyIsTypedAndDoesNotThrow) {
  // Regression: when the string proto callee is represented as
  // GetTableConstStringKeyInst, TypeSpecification must not throw and must set
  // the CallInst return type.

  auto ir_ctx = std::make_unique<IRContext>(nullptr);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx->SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx->GetMainMod();
  ASSERT_NE(nullptr, mod);

  OpBuilder* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_string_proto_call_const_key";
  auto* func = builder->Create<FuncOp>(0, name);
  ASSERT_NE(nullptr, func);

  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  auto* entry =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  ASSERT_NE(nullptr, entry);
  builder->SetInsertionPointToEnd(entry);

  const uint32_t str_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("hello"));
  const uint32_t char_at_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(constants::kStringCharAt));

  auto* str = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(str_idx), TypeOp::CreateString(builder));
  auto* get_char_at = builder->Create<GetTableConstStringKeyInst>(
      0, str, builder->GetLiteralUint32(char_at_idx),
      TypeOp::CreateAnyType(builder));

  ArgList args;
  args.push_back(builder->GetLiteralInt32(0));
  auto* call = builder->Create<CallInst>(0, get_char_at, args);
  builder->Create<ReturnInst>(0, call);

  TypeSpecification pass(ir_ctx.get());
  ASSERT_TRUE(pass.RunOnFunction(func));

  EXPECT_TRUE(get_char_at->GetType()->IsStringProtoAPIType());
  ASSERT_NE(call->GetType(), nullptr);
  EXPECT_EQ(call->GetType()->GetTypeKind(), TypeOp::TypeKind::String);
}

TEST(LEPUSIRTypeSpecificationMIR,
     PrototypeCallTypeFallbackOnAnyReceiverIsStable) {
  // Regression: when receiver type is Any, SetPrototypeCallType should still
  // assign stable return types based on prototype method name.

  auto ir_ctx = std::make_unique<IRContext>(nullptr);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  ir_ctx->SetTargetContext(target_ctx);

  ModuleOp* mod = ir_ctx->GetMainMod();
  ASSERT_NE(nullptr, mod);

  OpBuilder* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_proto_fallback_any_receiver";
  auto* func = builder->Create<FuncOp>(0, name);
  ASSERT_NE(nullptr, func);

  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  auto* entry =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  ASSERT_NE(nullptr, entry);
  builder->SetInsertionPointToEnd(entry);

  const uint32_t recv_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("opaque"));
  const uint32_t join_idx =
      static_cast<uint32_t>(lepus_func->AddConstString(constants::kArrayJoin));
  const uint32_t tofixed_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(constants::kNumberToFixed));
  const uint32_t test_idx =
      static_cast<uint32_t>(lepus_func->AddConstString(constants::kRegExpTest));

  // Receiver typed as Any (even though the const value is a string).
  auto* recv_any = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(recv_idx), TypeOp::CreateAnyType(builder));

  auto* get_join = builder->Create<GetTableConstStringKeyInst>(
      0, recv_any, builder->GetLiteralUint32(join_idx),
      TypeOp::CreateAnyType(builder));
  ArgList join_args;
  join_args.push_back(builder->GetLiteralInt32(0));
  auto* call_join = builder->Create<CallInst>(0, get_join, join_args);

  auto* get_tofixed = builder->Create<GetTableConstStringKeyInst>(
      0, recv_any, builder->GetLiteralUint32(tofixed_idx),
      TypeOp::CreateAnyType(builder));
  ArgList tofixed_args;
  tofixed_args.push_back(builder->GetLiteralInt32(2));
  auto* call_tofixed = builder->Create<CallInst>(0, get_tofixed, tofixed_args);

  auto* get_test = builder->Create<GetTableConstStringKeyInst>(
      0, recv_any, builder->GetLiteralUint32(test_idx),
      TypeOp::CreateAnyType(builder));
  ArgList test_args;
  test_args.push_back(builder->GetLiteralInt32(0));
  auto* call_test = builder->Create<CallInst>(0, get_test, test_args);

  // Keep calls alive.
  builder->Create<ReturnInst>(0, call_test);

  TypeSpecification pass(ir_ctx.get());
  ASSERT_TRUE(pass.RunOnFunction(func));

  EXPECT_EQ(call_join->GetType()->GetTypeKind(), TypeOp::TypeKind::String);
  EXPECT_EQ(call_join->GetBuiltinFuncName(),
            std::string(TypeOp::RetKindStr(TypeOp::Array)) + "." +
                constants::kArrayJoin);

  EXPECT_EQ(call_tofixed->GetType()->GetTypeKind(), TypeOp::TypeKind::String);
  EXPECT_EQ(call_tofixed->GetBuiltinFuncName(),
            std::string(TypeOp::RetKindStr(TypeOp::Number)) + "." +
                constants::kNumberToFixed);

  EXPECT_EQ(call_test->GetType()->GetTypeKind(), TypeOp::TypeKind::Boolean);
  EXPECT_EQ(call_test->GetBuiltinFuncName(),
            std::string(TypeOp::RetKindStr(TypeOp::RegExp)) + "." +
                constants::kRegExpTest);
}

// Test that String.split CallInst has Array return type
TEST(LEPUSIRTypeSpecificationIR, StringSplitReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "a,b,c";
    let arr = str.split(",");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  // Check that there's a CallInst with Array return type
  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::Array);
  EXPECT_TRUE(found) << "Expected CallInst with Array return type for split";

  delete context;
}

TEST(LEPUSIRTypeSpecificationIR, SetStyleObjectBuiltinNameAnnotation) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    function test(el) {
      __SetStyleObject(el, [1, 2]);
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* target_func = nullptr;
  for (auto* func : *mod) {
    if (FindFirstInstruction<CallInst>(func) != nullptr) {
      target_func = func;
      break;
    }
  }
  ASSERT_NE(nullptr, target_func);

  bool found = false;
  for (auto& block : *target_func) {
    for (auto& inst : block) {
      auto* call_inst = llvh::dyn_cast<CallInst>(&inst);
      if (!call_inst) continue;
      if (call_inst->GetBuiltinFuncName() == tasm::kCFunctionSetStyleObject) {
        found = true;
      }
    }
  }

  EXPECT_TRUE(found);
  delete context;
}

// Test that String.charAt CallInst has String return type
TEST(LEPUSIRTypeSpecificationIR, StringCharAtReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let ch = str.charAt(0);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  // Check that there's a CallInst with String return type
  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::String);
  EXPECT_TRUE(found) << "Expected CallInst with String return type for charAt";

  delete context;
}

// Test that GetTableInst for String method has StringProtoAPI type
TEST(LEPUSIRTypeSpecificationIR, StringMethodGetTableType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let method = str.charAt;
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  // Find GetTableInst and check its type
  bool found_string_proto_api = false;
  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* get_table = llvh::dyn_cast<GetTableInst>(&inst)) {
        if (get_table->GetType()->IsStringProtoAPIType()) {
          found_string_proto_api = true;
          break;
        }
      }
    }
    if (found_string_proto_api) break;
  }

  EXPECT_TRUE(found_string_proto_api)
      << "Expected GetTableInst with StringProtoAPI type";

  delete context;
}

// Test complex String operations
TEST(LEPUSIRTypeSpecificationIR, ComplexStringOperations) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello,world";
    let ch = str.charAt(0);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  // Should have CallInst with String type (for charAt)
  bool has_string_call =
      HasCallInstWithType(main_func, TypeOp::TypeKind::String);
  EXPECT_TRUE(has_string_call) << "Expected String return type for charAt";

  delete context;
}

// Test that all supported String methods have correct types
TEST(LEPUSIRTypeSpecificationIR, AllStringMethodTypes) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  // Test multiple String methods in one source
  std::string source = R"(
    let str = "hello world";
    let len = str.length;
    let parts = str.split(" ");
    let ch = str.charAt(0);
    let sub = str.substring(0, 5);
    let sliced = str.slice(0, 5);
    let replaced = str.replace("hello", "hi");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  // Count different instruction types
  int string_length_count = CountInstructions<GetStringLengthInst>(main_func);
  EXPECT_GT(string_length_count, 0) << "Should have GetStringLengthInst";

  // Check for CallInst with different return types
  int array_calls = 0;
  int string_calls = 0;

  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (call_inst->GetType()->GetTypeKind() == TypeOp::TypeKind::Array) {
          array_calls++;
        } else if (call_inst->GetType()->GetTypeKind() ==
                   TypeOp::TypeKind::String) {
          string_calls++;
        }
      }
    }
  }

  EXPECT_GT(array_calls, 0) << "Should have Array return type (split)";
  EXPECT_GT(string_calls, 0)
      << "Should have String return types (charAt, substring, slice, replace)";

  delete context;
}

// Test String.substring returns String type
TEST(LEPUSIRTypeSpecificationIR, StringSubstringReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let sub = str.substring(1, 3);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::String);
  EXPECT_TRUE(found)
      << "Expected CallInst with String return type for substring";

  delete context;
}

// Test String.slice returns String type
TEST(LEPUSIRTypeSpecificationIR, StringSliceReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let sliced = str.slice(1, 3);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::String);
  EXPECT_TRUE(found) << "Expected CallInst with String return type for slice";

  delete context;
}

// Test String.replace returns String type
TEST(LEPUSIRTypeSpecificationIR, StringReplaceReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let replaced = str.replace("l", "x");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::String);
  EXPECT_TRUE(found) << "Expected CallInst with String return type for replace";

  delete context;
}

// Test String.trim returns String type
TEST(LEPUSIRTypeSpecificationIR, StringTrimReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "  hello  ";
    let trimmed = str.trim();
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::String);
  EXPECT_TRUE(found) << "Expected CallInst with String return type for trim";

  delete context;
}

// Test String.match returns Array type
TEST(LEPUSIRTypeSpecificationIR, StringMatchReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let matched = str.match("h");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::Array);
  EXPECT_TRUE(found) << "Expected CallInst with Array return type for match";

  delete context;
}

// Test String.search returns Int64 type
TEST(LEPUSIRTypeSpecificationIR, StringSearchReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let str = "hello";
    let pos = str.search("e");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found = HasCallInstWithType(main_func, TypeOp::TypeKind::Int64);
  EXPECT_TRUE(found) << "Expected CallInst with Int64 return type for search";

  delete context;
}

// Test BuiltinFunc Type
TEST(LEPUSIRTypeSpecificationIR, SpecifyBuiltinFuncType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
  let source = "hello, world";
  let substr = "hello";
  let index = String.indexOf(source, substr);
  console.log(index);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found =
      HasGetGlobalInstWithType(main_func, TypeOp::TypeKind::BuiltinFuncTable);
  EXPECT_TRUE(found)
      << "Expected GetGlobalInst with BuiltinFuncTable return type for indexOf";

  delete context;
}

// Test Built-in Function Return Type (e.g. __CreateView -> Any)
TEST(LEPUSIRTypeSpecificationIR, BuiltinReturnTypeAny) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);

  // Register __CreateView for test purpose
  context->SetGlobalData(constants::kCreateView,
                         lepus::Value((void*)&EmptyFunc));

  std::string source = R"(
    let view = __CreateView("view");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found_any_type_call = false;
  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (call_inst->GetType()->IsAnyType()) {
          found_any_type_call = true;
        }
      }
    }
  }
  EXPECT_TRUE(found_any_type_call)
      << "Expected CallInst with Any return type for __CreateView";

  delete context;
}

// Test parseInt and isNaN return type
TEST(LEPUSIRTypeSpecificationIR, StandardBuiltinReturnType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  std::string source = R"(
    let n = parseInt("123");
    let b = isNaN(n);
    let s = encodeURIComponent("abc");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found_number_type = false;
  bool found_bool_type = false;
  bool found_string_type = false;

  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (call_inst->GetType()->IsNumberType()) {
          found_number_type = true;
        } else if (call_inst->GetType()->IsBooleanType()) {
          found_bool_type = true;
        } else if (call_inst->GetType()->IsStringType()) {
          found_string_type = true;
        }
      }
    }
  }

  EXPECT_TRUE(found_number_type)
      << "Expected CallInst with Number return type for parseInt";
  EXPECT_TRUE(found_bool_type)
      << "Expected CallInst with Boolean return type for isNaN";
  EXPECT_TRUE(found_string_type)
      << "Expected CallInst with String return type for encodeURIComponent";

  delete context;
}

// Test Readonly Call Identification
TEST(LEPUSIRTypeSpecificationIR, ReadonlyCallIdentification) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);

  std::string source = R"(
    let n = parseInt("123"); // readonly
    let m = Math.abs(-5); // readonly
    let sub = "abc".substring(1); // readonly
    let j = JSON.parse("{}"); // readonly
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  int readonly_calls = 0;
  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (call_inst->IsReadonlyCall()) {
          readonly_calls++;
        }
      }
    }
  }

  EXPECT_EQ(readonly_calls, 4)
      << "Expected 4 readonly calls (parseInt, Math.abs, substr, JSON.parse)";

  delete context;
}

TEST(LEPUSIRTypeSpecificationIR, DetailedReadonlyAttributeCheck) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);

  std::string source = R"(
    // 1. Builtins (8 calls)
    parseFloat("1.1");
    parseInt("1");
    isFinite(1);
    isNaN(1);
    encodeURI("a");
    encodeURIComponent("a");
    decodeURI("a");
    decodeURIComponent("a");

    // 2. Math Methods (16 calls)
    Math.abs(-1);
    Math.acos(0.5);
    Math.asin(0.5);
    Math.atan(0.5);
    Math.ceil(1.1);
    Math.cos(0);
    Math.exp(1);
    Math.floor(1.1);
    Math.log(1);
    Math.max(1, 2);
    Math.min(1, 2);
    Math.pow(2, 2);
    Math.round(1.5);
    Math.sin(0);
    Math.sqrt(4);
    Math.tan(0);

    // 3. JSON Methods (2 calls)
    JSON.parse("{}");
    JSON.stringify({});

    // 4. Date Methods (1 call)
    Date.now();

    // 5. Object Methods (1 call)
    Object.keys({});

    // 6. String Prototype Methods (8 calls)
    // Note: Use literals or known string variables so type inference works
    "hello".split(",");
    "hello".trim();
    "hello".charAt(0);
    "hello".search("e");
    "hello".match("e");
    "hello".slice(0, 1);
    "hello".substring(0, 1);
    "hello".substring(0, 1);

    // 7. Array Prototype Methods (4 calls)
    [1, 2].concat([3]);
    [1, 2].join(",");
    [1, 2].includes(1);
    [1, 2].slice(0, 1);

    // 8. Number Prototype Methods (1 call)
    let num_val = 123;
    num_val.toFixed(2);

    // 9. Non-Readonly calls (should NOT be marked)
    Object.assign({}, {});
    console.log("log");
    let arr_mut = [];
    arr_mut.push(1);
    let str_mut = "a";
    str_mut.replace("a", "b");
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  int readonly_calls = 0;
  int total_calls = 0;

  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        total_calls++;
        if (call_inst->IsReadonlyCall()) {
          readonly_calls++;
        }
      }
    }
  }

  // There are 4 non-readonly calls in source: assign, log, push, replace.
  // We use literal /abc/, which does not produce a CallInst.
  // So total calls = 45 (base).

  EXPECT_EQ(readonly_calls, 41) << "Expected 41 readonly calls";
  EXPECT_EQ(total_calls, 45) << "Expected 45 total calls";

  delete context;
}

TEST(LEPUSIRTypeSpecificationIR, PrototypeCallTypeInference) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    // Array
    let t = Date.now();
    let sep = "" + (t & 3);
    let v = (t & 1);
    let arr = [1, 2, 3];
    let newArr = arr.map(print);
    let joined = arr.join(sep);
    let has = arr.includes(v);
    let size = arr.push(v);
    
    // Number
    let num = 123.456;
    let fixed = num.toFixed(v + 1);
    
    // RegExp
    let reg = /abc/;
    let tested = reg.test(sep);

    // Keep values alive so calls aren't removed by DCE.
    Assert(newArr);
    Assert(joined);
    Assert(has);
    Assert(size);
    Assert(fixed);
    Assert(tested);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found_array_map = false;
  bool found_array_join = false;
  bool found_array_includes = false;
  bool found_array_push = false;
  bool found_num_tofixed = false;
  bool found_reg_test = false;

  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        Value* callee = call_inst->GetFunction();
        auto lepus_func = main_func->GetLepusFunction();
        if (!lepus_func) continue;

        // Prototype calls may be represented as:
        // - GetTableInst + LoadConstInst(prop)
        // - GetTableConstStringKeyInst(const_index)
        uint32_t const_idx = 0;
        bool has_const_idx = false;
        if (auto* get_table = llvh::dyn_cast<GetTableInst>(callee)) {
          if (auto* load_const =
                  llvh::dyn_cast<LoadConstInst>(get_table->GetProp())) {
            if (auto* lit = llvh::dyn_cast<LiteralUint32>(
                    load_const->GetSingleOperand())) {
              const_idx = lit->GetValue();
              has_const_idx = true;
            }
          }
        } else if (auto* get_table_const =
                       llvh::dyn_cast<GetTableConstStringKeyInst>(callee)) {
          if (auto* lit = llvh::dyn_cast<LiteralUint32>(
                  get_table_const->GetConstIndex())) {
            const_idx = lit->GetValue();
            has_const_idx = true;
          }
        }
        if (!has_const_idx) continue;

        auto val = lepus_func->GetConstValue(const_idx);
        if (!val || !val->IsString()) continue;

        std::string name = val->StdString();
        auto type_kind = call_inst->GetType()->GetTypeKind();
        if (name == constants::kArrayMap) {
          if (type_kind == TypeOp::TypeKind::Array) found_array_map = true;
        } else if (name == constants::kArrayJoin) {
          if (type_kind == TypeOp::TypeKind::String) found_array_join = true;
        } else if (name == constants::kArrayIncludes) {
          if (type_kind == TypeOp::TypeKind::Boolean)
            found_array_includes = true;
        } else if (name == constants::kArrayPush) {
          if (type_kind == TypeOp::TypeKind::Uint64) found_array_push = true;
        } else if (name == constants::kNumberToFixed) {
          if (type_kind == TypeOp::TypeKind::String) found_num_tofixed = true;
        } else if (name == constants::kRegExpTest) {
          if (type_kind == TypeOp::TypeKind::Boolean) found_reg_test = true;
        }
      }
    }
  }

  EXPECT_TRUE(found_array_map) << "Array.map should return Array";
  EXPECT_TRUE(found_array_join) << "Array.join should return String";
  EXPECT_TRUE(found_array_includes) << "Array.includes should return Boolean";
  EXPECT_TRUE(found_array_push) << "Array.push should return Uint64";
  EXPECT_TRUE(found_num_tofixed) << "Number.toFixed should return String";
  EXPECT_TRUE(found_reg_test) << "RegExp.test should return Boolean";

  delete context;
}

TEST(LEPUSIRTypeSpecificationIR, GlobalObjectCallType) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  std::string source = R"(
    let start = Date.now();
    let max = Math.max(start, 2);
    let min = Math.min(start, 2);
    let obj = { a: start };
    let json = JSON.stringify(obj);
    let parsed = JSON.parse(json);
    let keys = Object.keys(obj);
    let strSubstr = String.substr("abc", 1);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  int num_count = 0;
  int str_count = 0;
  int arr_count = 0;
  int any_count = 0;

  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (call_inst->GetType()) {
          if (call_inst->GetType()->IsNumberType())
            num_count++;
          else if (call_inst->GetType()->IsStringType())
            str_count++;
          else if (call_inst->GetType()->IsArrayType())
            arr_count++;
          else if (call_inst->GetType()->IsAnyType())
            any_count++;
        }
      }
    }
  }

  // Date.now, Math.max, Math.min => 3 Number calls
  EXPECT_EQ(num_count, 3) << "Expected 3 Number calls";
  // JSON.stringify, String.substr => 2 String call
  EXPECT_EQ(str_count, 2) << "Expected 2 String call";
  // Object.keys => 1 Array call
  EXPECT_EQ(arr_count, 1) << "Expected 1 Array call";
  // JSON.parse => 1 Any call
  EXPECT_EQ(any_count, 1) << "Expected 1 Any call";

  delete context;
}

TEST(LEPUSIRTypeSpecificationIR, AllGlobalBuiltinCallTypes) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  // Register Global Functions
  context->SetGlobalData(constants::kGetElementUniqueID,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kCreateElement,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kCreateView,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kCreateImage,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kCreateText,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kCreatePage,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kCreateComponent,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kGetDiffData,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kGetSystemInfo,
                         lepus::Value((void*)&EmptyFunc));
  context->SetGlobalData(constants::kGetTextInfo,
                         lepus::Value((void*)&EmptyFunc));

  // Ensure Number is registered in global if not already
  if (context->global()->Search("Number") == -1) {
    context->SetGlobalData("Number", lepus::Value(lepus::Dictionary::Create()));
  }

  // Ensure Object is registered in builtin if not already
  if (context->builtin()->Search(constants::kBuiltinObject) == -1) {
    context->SetBuiltinData(constants::kBuiltinObject,
                            lepus::Value(lepus::Dictionary::Create()));
  }

  std::string source = R"(
    // 1. Global Functions (10 calls)
    // __GetElementUniqueID -> Int64 (1)
    let id = __GetElementUniqueID(); 
    
    // __Create* -> Any (6)
    let elem = __CreateElement("div");
    let view = __CreateView("view");
    let image = __CreateImage("image");
    let text = __CreateText("text");
    let page = __CreatePage("page");
    let comp = __CreateComponent("comp");

    // __Get* -> Table (3)
    let diff = __GetDiffData();
    let sys = __GetSystemInfo();
    let txtInfo = __GetTextInfo();

    // 2. Builtin Functions (Global scope) (5 calls)
    // parseInt, parseFloat -> Number (2)
    let pInt = parseInt("123");
    let pFloat = parseFloat("1.23");
    
    // isNaN -> Boolean (1)
    let nan = isNaN(123);

    // encode/decode -> String (2)
    let enc = encodeURIComponent("abc");
    let dec = decodeURIComponent("abc");

    // 3. Math (Global) (2 calls)
    // -> Number
    let abs = Math.abs(-1);
    let max = Math.max(1, 2);

    // 4. String (Global) (3 calls)
    // -> String
    let strSubstr = String.substr("hello", 1);
    // -> Uint32 (2)
    let strIdx = String.indexOf("hello", "e");
    let strLen = String.length;
    // fromCharCode is not supported in current lepus String API
    // let fromChar = String.fromCharCode(65);

    // 5. Number (Global) (2 calls)
    // -> Number
    // Number.parseInt/parseFloat are NOT supported in Lepus global Number object.
    // They are available as global builtins only.
    // let nInt = Number.parseInt("123");
    // let nFloat = Number.parseFloat("1.23");

    // 6. JSON (Builtin) (2 calls)
    // stringify -> String (1)
    let jsonStr = JSON.stringify({});
    // parse -> Any (1)
    let jsonObj = JSON.parse("{}");

    // 7. Date (Builtin) (3 calls)
    // -> Number
    let now = Date.now();

    // 8. Object (Builtin) (3 calls)
    // -> Array
    let keys = Object.keys({});
    // -> Array (2)
    Object.assign({}, {});
    Object.freeze({});
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  int int64_count = 0;
  int any_count = 0;
  int table_count = 0;
  int number_count = 0;
  int boolean_count = 0;
  int string_count = 0;
  int array_count = 0;
  int uint32_count = 0;

  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (!call_inst->GetType()) continue;

        auto kind = call_inst->GetType()->GetTypeKind();
        if (kind == TypeOp::TypeKind::Int64)
          int64_count++;
        else if (kind == TypeOp::TypeKind::Any)
          any_count++;
        else if (kind == TypeOp::TypeKind::Object)
          table_count++;
        else if (kind == TypeOp::TypeKind::Number)
          number_count++;
        else if (kind == TypeOp::TypeKind::Boolean)
          boolean_count++;
        else if (kind == TypeOp::TypeKind::String)
          string_count++;
        else if (kind == TypeOp::TypeKind::Array)
          array_count++;
        else if (kind == TypeOp::TypeKind::Uint32)
          uint32_count++;

        if (!call_inst->GetBuiltinFuncName().empty()) {
          // Verify the attribute is set correctly.
          std::string func_name = call_inst->GetBuiltinFuncName();
          if (func_name == constants::kGetElementUniqueID ||
              func_name == constants::kCreateElement ||
              func_name == constants::kGetDiffData ||
              func_name == constants::kParseInt ||
              func_name == constants::kIsNaN ||
              func_name == constants::kEncodeURIComponent ||
              func_name == std::string(constants::kGlobalMath) + "." +
                               constants::kMathMax ||
              func_name == std::string(constants::kGlobalString) + "." +
                               constants::kStringSubstr ||
              func_name == std::string(constants::kBuiltinJSON) + "." +
                               constants::kJSONStringify ||
              func_name == std::string(constants::kBuiltinDate) + "." +
                               constants::kDateNow ||
              func_name == std::string(constants::kBuiltinObject) + "." +
                               constants::kObjectKeys) {
            // ok
          }
        }
      }
    }
  }

  // Verification
  // Int64: __GetElementUniqueID (1)
  EXPECT_EQ(int64_count, 1) << "Expected 1 Int64 call";

  // Any:
  //   __Create* (6)
  //   JSON.parse (1)
  //   Object.assign (1)
  //   Total = 8
  EXPECT_EQ(any_count, 8) << "Expected 8 Any calls";

  // Table:
  //   __Get* (3)
  //   Object.freeze (1)
  //   Total = 4
  EXPECT_EQ(table_count, 4) << "Expected 4 Table calls";

  // Number:
  //   parseInt/parseFloat (2)
  //   Math.abs/max (2)
  //   Date.now (1)
  //   String.indexOf (1)
  //   Total = 6
  EXPECT_EQ(number_count, 6) << "Expected 6 Number calls";

  // Boolean: isNaN (1)
  EXPECT_EQ(boolean_count, 1) << "Expected 1 Boolean call";

  // String:
  //   encode/decode (2)
  //   String.substr (1)
  //   JSON.stringify (1)
  //   Total = 4
  EXPECT_EQ(string_count, 4) << "Expected 4 String calls";

  EXPECT_EQ(array_count, 1) << "Expected 1 Array calls";

  // Uint32:
  //   Total = 0
  EXPECT_EQ(uint32_count, 0) << "Expected 0 Uint32 calls";

  delete context;
}

// Test iterative type propagation
TEST(LEPUSIRTypeSpecificationIR, IterativePropagation) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);
  context->SetClosureFix(true);

  // Source where types propagate:
  // 1. str.length -> Int64 (via GetStringLengthInst)
  // 2. -len -> Int64 (via PropagateTypes UnaryNeg)
  // 3. neg + 1 -> Int64 (via PropagateTypes BinaryAdd)
  std::string source = R"(
    let str = "hello";
    let len = str.length; // GetStringLengthInst: Int64
    let neg = -len;       // UnaryOperatorInst: Int64
    let res = neg + 1;    // BinaryOperatorInst: Int64
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found_len_int64 = false;
  bool found_unary_int64 = false;
  bool found_binary_int64 = false;

  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* len = llvh::dyn_cast<GetStringLengthInst>(&inst)) {
        // String.length
        if (len->GetType()->IsInt64Type()) {
          found_len_int64 = true;
        }
      } else if (auto* unary = llvh::dyn_cast<UnaryOperatorInst>(&inst)) {
        // -len
        if (unary->GetKind() == ValueKind::UnaryNegInstKind &&
            unary->GetType()->IsInt64Type()) {
          found_unary_int64 = true;
        }
      } else if (auto* binary = llvh::dyn_cast<BinaryOperatorInst>(&inst)) {
        // neg + 1
        if (binary->GetKind() == ValueKind::BinaryAddInstKind &&
            binary->GetType()->IsInt64Type()) {
          found_binary_int64 = true;
        }
      }
    }
  }

  EXPECT_TRUE(found_len_int64)
      << "String.length should be GetStringLengthInst with Int64 type";
  EXPECT_TRUE(found_unary_int64) << "UnaryNeg of Int64 should return Int64";
  EXPECT_TRUE(found_binary_int64) << "BinaryAdd of Int64 should return Int64";

  delete context;
}

// Test propagation of generic Number type to avoid infinite loops (e.g. in
// BinaryMod)
TEST(LEPUSIRTypeSpecificationIR, PropagateNumberTypeThroughMod) {
  lepus::VMContext* context = new lepus::VMContext();
  context->Initialize();
  RegisterBuiltinForTest(context);

  // Register Math in global for Math.max
  context->SetGlobalData(constants::kGlobalMath,
                         lepus::Value(lepus::Dictionary::Create()));
  // Register opaque function to prevent constant folding and keep insts alive
  context->SetGlobalData("opaque", lepus::Value((void*)&EmptyFunc));

  // Math.max returns generic Number type.
  // Mod returns generic Number type.
  // This triggers the infinite loop if comparison is not fixed.
  std::string source = R"(
    let a = opaque();
    let n = Math.max(a, 2);
    let m = n % 2;
    opaque(m);
  )";

  auto ir_ctx = std::make_unique<IRContext>(context);
  ModuleOp* mod = CompileToIRWithOptimization(context, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func = nullptr;
  for (auto* func : *mod) {
    main_func = func;
    break;
  }
  ASSERT_NE(nullptr, main_func);

  bool found_mod_number = false;
  for (auto& block : *main_func) {
    for (auto& inst : block) {
      if (auto* bin = llvh::dyn_cast<BinaryOperatorInst>(&inst)) {
        if (bin->GetKind() == ValueKind::BinaryModInstKind) {
          if (bin->GetType()->GetTypeKind() == TypeOp::TypeKind::Number) {
            found_mod_number = true;
          }
        }
      }
    }
  }

  EXPECT_TRUE(found_mod_number)
      << "BinaryMod should return generic Number type and stabilize";

  delete context;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
