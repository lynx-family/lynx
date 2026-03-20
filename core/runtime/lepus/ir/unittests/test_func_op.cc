// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/ir/pass_manager/pipeline.h"
#include "core/runtime/lepus/ir/transformer/mir/ssa_ir_verify.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/parser.h"
#include "core/runtime/lepus/scanner.h"
#include "core/runtime/lepus/semantic_analysis.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestFuncOp : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestFuncOp, createFuncOp) {
  auto source_code = (R"code(
)code");

  auto vm_ctx = lepus::VMContext();
  auto context = &vm_ctx;
  context->Initialize();

  auto sdk_version = "2.6";
  RegisterBuiltin(context);
  parser::InputStream input;
  input.Write(source_code);
  Scanner scanner(&input);
  scanner.SetSdkVersion(sdk_version);
  Parser parser(&scanner);
  parser.SetSdkVersion(sdk_version);
  SemanticAnalysis semantic_analysis;
  semantic_analysis.SetInput(&scanner);
  semantic_analysis.SetSdkVersion(sdk_version);
  semantic_analysis.SetClosureFix(true);

  CodeGenerator code_generator(context, &semantic_analysis);
  std::unique_ptr<ASTree> root;
  root.reset(parser.Parse());
  root->Accept(&semantic_analysis, nullptr);
  root->Accept(&code_generator, nullptr);

  auto root_func = context->GetRootFunction();
  if (root_func) {
    root_func->SetSource(source_code);
    root_func->SetTopLevelFunction(true);
  }

  if (true) {
    ir_ctx->Init(root_func, context);
    auto mod = ir_ctx->GetMainMod();
    RunO1OptimizationPasses(*mod);
  }

  auto mod = ir_ctx->GetMainMod();
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* func_bb = mod->GetFunctionBlock();
  ASSERT_NE(nullptr, func_bb);

  auto main_func = mod->GetRootFunction();
  ASSERT_NE(nullptr, main_func);
  ASSERT_TRUE(main_func->IsToplevelFunc());

  auto iter = mod->begin();
  ASSERT_EQ(mod->FuncSize(), 1);
  ASSERT_EQ(main_func, *iter);
  auto root_lepus_function = main_func->GetLepusFunction();
  ASSERT_NE(nullptr, root_lepus_function);
  ASSERT_TRUE(root_lepus_function->IsToplevelFunction());
  ASSERT_TRUE(root_lepus_function->GetFunctionName() == "<anonymous>");

  ASSERT_EQ(1, main_func->GetRegionSize());
  auto root_func_region = main_func->GetSingleRegion();
  ASSERT_NE(nullptr, root_func_region);
}

TEST_F(LEPUSIRTestFuncOp, createFuncOp2) {
  auto source_code = (R"code(
    function test() {
        return 1;
    }
    console.log(test());
)code");

  auto vm_ctx = lepus::VMContext();
  auto context = &vm_ctx;
  context->Initialize();

  auto sdk_version = "2.6";
  parser::InputStream input;
  input.Write(source_code);
  Scanner scanner(&input);
  scanner.SetSdkVersion(sdk_version);
  Parser parser(&scanner);
  parser.SetSdkVersion(sdk_version);
  SemanticAnalysis semantic_analysis;
  semantic_analysis.SetInput(&scanner);
  semantic_analysis.SetSdkVersion(sdk_version);
  semantic_analysis.SetClosureFix(true);

  CodeGenerator code_generator(context, &semantic_analysis);
  std::unique_ptr<ASTree> root;
  root.reset(parser.Parse());
  root->Accept(&semantic_analysis, nullptr);
  root->Accept(&code_generator, nullptr);

  auto root_func = context->GetRootFunction();
  if (root_func) {
    root_func->SetSource(source_code);
    root_func->SetTopLevelFunction(true);
  }

  if (true) {
    ir_ctx->Init(root_func, context);
    auto mod = ir_ctx->GetMainMod();
    RunO1OptimizationPasses(*mod);
  }

  auto mod = ir_ctx->GetMainMod();
  ASSERT_EQ(1, mod->GetRegionSize());

  auto region = mod->GetSingleRegion();
  ASSERT_NE(nullptr, region);

  auto* func_bb = mod->GetFunctionBlock();
  ASSERT_NE(nullptr, func_bb);

  auto main_func = mod->GetRootFunction();
  ASSERT_NE(nullptr, main_func);
  ASSERT_TRUE(main_func->IsToplevelFunc());

  auto iter = mod->begin();
  ASSERT_EQ(mod->FuncSize(), 2);
  ASSERT_EQ(main_func, *iter);
  auto root_lepus_function = main_func->GetLepusFunction();
  ASSERT_NE(nullptr, root_lepus_function);
  ASSERT_TRUE(root_lepus_function->IsToplevelFunction());
  ASSERT_TRUE(root_lepus_function->GetFunctionName() == "<anonymous>");

  ASSERT_EQ(1, main_func->GetRegionSize());
  auto root_func_region = main_func->GetSingleRegion();
  ASSERT_NE(nullptr, root_func_region);

  ++iter;
  auto test_func = *iter;
  ASSERT_NE(nullptr, test_func);
  ASSERT_FALSE(test_func->IsToplevelFunc());
  auto test_lepus_function = test_func->GetLepusFunction();
  ASSERT_NE(nullptr, test_lepus_function);
  ASSERT_FALSE(test_lepus_function->IsToplevelFunction());
  ASSERT_TRUE(test_lepus_function->GetFunctionName() == "test");
}

TEST_F(LEPUSIRTestFuncOp, SSAIRVerifyCoversToplevelAndAttributes) {
  auto* m = ir_ctx->GetMainMod();
  ASSERT_NE(nullptr, m);
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  // Construct an IR that satisfies SSAIRVerify constraints and run only
  // SSAIRVerifyPass.
  // Purpose: cover the validation logic for the toplevel var side-table and
  // special attributes without depending on other pipeline passes.

  builder->SetInsertionPointToEnd(m->GetFunctionBlock());
  std::string toplevel_name = "toplevel";
  auto* toplevel_func = builder->Create<FuncOp>(1, toplevel_name);
  ASSERT_NE(nullptr, toplevel_func);

  auto* toplevel_region = builder->CreateRegion(toplevel_func);
  ASSERT_NE(nullptr, toplevel_region);
  auto* entry =
      builder->CreateBlock(toplevel_region, BlockType::BT_INST, 0, "entry");
  ASSERT_NE(nullptr, entry);
  builder->SetInsertionPointToEnd(entry);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  ASSERT_NE(nullptr, v);

  // SetToplevelVarInst carries the special attributes.
  auto* set =
      builder->Create<SetToplevelVarInst>(1, builder->GetLiteralUint32(10), v);
  ASSERT_NE(nullptr, set);
  set->SetToplevelVarReg(10);
  set->SetClosureVarReg(10);
  ir_ctx->InsertToplevelValue(set, 10);

  builder->Create<ReturnInst>(1, v);

  // Also create a second function to make sure the verifier can walk multiple
  // functions in a module.
  builder->SetInsertionPointToEnd(m->GetFunctionBlock());
  std::string child_name = "child";
  auto* child_func = builder->Create<FuncOp>(1, child_name);
  ASSERT_NE(nullptr, child_func);
  auto* child_region = builder->CreateRegion(child_func);
  auto* child_entry =
      builder->CreateBlock(child_region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(child_entry);
  auto* cv = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(2),
                                            TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(1, cv);

  SSAIRVerifyPass verify(ir_ctx.get());
  ASSERT_TRUE(verify.RunOnModule(m));
}

TEST_F(LEPUSIRTestFuncOp, SSAIRVerifyFailsOnMissingTerminator) {
  auto* m = ir_ctx->GetMainMod();
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToEnd(m->GetFunctionBlock());
  std::string name = "bad";
  auto* f = builder->Create<FuncOp>(1, name);
  auto* region = builder->CreateRegion(f);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(bb);
  // Intentionally no terminator.

  SSAIRVerifyPass verify(ir_ctx.get());
  EXPECT_THROW(verify.RunOnModule(m), ::lynx::lepus::CompileException);
}

TEST_F(LEPUSIRTestFuncOp, SSAIRVerifyFailsOnMultipleTerminators) {
  auto* m = ir_ctx->GetMainMod();
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToEnd(m->GetFunctionBlock());
  std::string name = "bad";
  auto* f = builder->Create<FuncOp>(1, name);
  auto* region = builder->CreateRegion(f);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(bb);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(1, v);
  builder->Create<ReturnInst>(1, v);

  SSAIRVerifyPass verify(ir_ctx.get());
  EXPECT_THROW(verify.RunOnModule(m), ::lynx::lepus::CompileException);
}

TEST_F(LEPUSIRTestFuncOp, SSAIRVerifyFailsOnMovInstInSSA) {
  auto* m = ir_ctx->GetMainMod();
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToEnd(m->GetFunctionBlock());
  std::string name = "bad";
  auto* f = builder->Create<FuncOp>(1, name);
  auto* region = builder->CreateRegion(f);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(bb);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  builder->Create<MovInst>(1, v);
  builder->Create<ReturnInst>(1, v);

  SSAIRVerifyPass verify(ir_ctx.get());
  EXPECT_THROW(verify.RunOnModule(m), ::lynx::lepus::CompileException);
}

TEST_F(LEPUSIRTestFuncOp, SSAIRVerifyFailsOnLoadConstAnyType) {
  auto* m = ir_ctx->GetMainMod();
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToEnd(m->GetFunctionBlock());
  std::string name = "bad";
  auto* f = builder->Create<FuncOp>(1, name);
  auto* region = builder->CreateRegion(f);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(bb);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(1),
                                           TypeOp::CreateAnyType(builder));
  builder->Create<ReturnInst>(1, v);

  SSAIRVerifyPass verify(ir_ctx.get());
  EXPECT_THROW(verify.RunOnModule(m), ::lynx::lepus::CompileException);
}

TEST_F(LEPUSIRTestFuncOp, SSAIRVerifyFailsOnIllegalSpecialAttributes) {
  auto* m = ir_ctx->GetMainMod();
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToEnd(m->GetFunctionBlock());
  std::string name = "bad";
  auto* f = builder->Create<FuncOp>(1, name);
  auto* region = builder->CreateRegion(f);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(bb);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  // Illegal: LoadConstInst must not carry toplevelVarReg/closureVarReg.
  v->SetToplevelVarReg(1);
  v->SetClosureVarReg(1);
  builder->Create<ReturnInst>(1, v);

  SSAIRVerifyPass verify(ir_ctx.get());
  EXPECT_THROW(verify.RunOnModule(m), ::lynx::lepus::CompileException);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
