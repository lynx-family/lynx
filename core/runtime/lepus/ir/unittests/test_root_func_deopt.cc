// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/pass_manager/pipeline.h"
#include "core/runtime/lepus/ir/transformer/mir/update_toplevel_closure_var.h"
#include "core/runtime/lepus/ir/transformer/vm/instruction_selection.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/op_code.h"
#include "core/runtime/lepus/parser.h"
#include "core/runtime/lepus/scanner.h"
#include "core/runtime/lepus/semantic_analysis.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

constexpr const char* kDeepClonePrelude = R"(
  function __IsArray(a) {
    if (a) {
      if (a.push === [].push) {
        return true;
      }
    }
    return false;
  }

  function $getDataType(data) {
    let type = typeof data;
    if (type !== "object") return type;
    if (__IsArray(data)) return "array";
    if (data == null) return "null";
    return "object";
  }

  function $deepClone(src) {
    let type = $getDataType(src);
    if (type === "array") {
      let array = [];
      src.forEach(function (item) {
        array.push(item);
      });
      return array;
    } else if (type === "object") {
      let keys = Object.keys(src);
      let dic = {};
      keys.forEach(function (key) {
        dic[key] = src[key];
      });
      return dic;
    } else {
      return src;
    }
  }
)";

static int CountDeepCloneAttrCalls(FuncOp* func) {
  if (func == nullptr) {
    return 0;
  }
  int count = 0;
  for (auto& bb : *func) {
    for (auto& inst : bb) {
      if (auto* call = llvh::dyn_cast<CallInst>(&inst);
          call && call->IsDeepCloneCall()) {
        ++count;
      }
    }
  }
  return count;
}

static int CountDeepCloneOpcodes(FuncOp* func) {
  if (func == nullptr) {
    return 0;
  }
  auto lepus_func = func->GetLepusFunction();
  if (lepus_func == nullptr) {
    return 0;
  }
  int count = 0;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    if (::lynx::lepus::Instruction::GetOpCode(*lepus_func->GetInstruction(i)) ==
        ::lynx::lepus::TypeOp_DeepClone) {
      ++count;
    }
  }
  return count;
}

ModuleOp* CompileToIRWithOptimization(lepus::VMContext* context,
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

  ir_ctx->Init(root_func, context);
  auto* mod = ir_ctx->GetMainMod();
  RunO1OptimizationPasses(*mod);
  return mod;
}

RegisterAllocator* BuildTestRegisterAllocation(FuncOp* func) {
  auto* ra = new RegisterAllocator(func);
  PostOrderAnalysis po(func);
  llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
  ra->Preallocate();
  ra->Allocate(order);
  return ra;
}

class LEPUSIRRootFuncDeoptTest : public IRTestBase {
 public:
  struct RootChildFunctions {
    FuncOp* root_func{nullptr};
    FuncOp* child_func{nullptr};
    fml::RefPtr<Function> root_lepus_func;
    fml::RefPtr<Function> child_lepus_func;
  };

  void SetUp() override {
    vm_ctx_ = std::make_unique<VMContext>();
    ir_ctx = std::make_unique<IRContext>(vm_ctx_.get());
    std::unique_ptr<TargetContext> target_ctx =
        std::make_unique<TargetContext>();
    ir_ctx->SetTargetContext(target_ctx);
    mod = ir_ctx->GetMainMod();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }

  void TearDown() override {
    IRTestBase::TearDown();
    vm_ctx_.reset();
  }

  FuncOp* CreateFunctionForLepus(const std::string& name,
                                 const fml::RefPtr<Function>& lepus_func,
                                 bool is_root = false) {
    auto* builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

    auto* func_op = builder->Create<FuncOp>(0, const_cast<std::string&>(name));
    EXPECT_NE(nullptr, func_op);
    auto func = lepus_func;
    func_op->Init(func);
    if (is_root) {
      func_op->SetTopLevelFunction();
      mod->SetRootFunction(func_op);
    }
    builder->CreateBlock(func_op->GetSingleRegion(), BlockType::BT_INST, {});
    return func_op;
  }

  RootChildFunctions CreateRootAndChildFunctions(
      const std::string& root_name, const std::string& child_name) {
    RootChildFunctions funcs;
    funcs.root_lepus_func = Function::Create();
    funcs.root_lepus_func->SetFunctionName(root_name);
    funcs.root_lepus_func->SetTopLevelFunction(true);
    funcs.child_lepus_func = Function::Create();
    funcs.child_lepus_func->SetFunctionName(child_name);
    funcs.root_lepus_func->AddChildFunction(funcs.child_lepus_func);
    funcs.root_func =
        CreateFunctionForLepus(root_name, funcs.root_lepus_func, true);
    funcs.child_func =
        CreateFunctionForLepus(child_name, funcs.child_lepus_func);
    return funcs;
  }

  Instruction* AddAnchorInstruction(FuncOp* func, int value) {
    auto* builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(&func->Front());
    auto* inst = builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(value), TypeOp::CreateInt32(builder));
    builder->Create<ReturnInst>(0, inst);
    return inst;
  }

  NopInst* AddRecordedRootClosureAnchor(FuncOp* root_func, int closure_reg) {
    auto* builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToStart(&root_func->Front());
    auto* root_anchor = builder->Create<NopInst>(0);
    root_anchor->SetType(TypeOp::CreateAnyType(builder));
    root_anchor->SetClosureVarReg(closure_reg);
    root_func->RecordClosureVarRegAndValue(closure_reg, root_anchor);
    root_func->InsertToplevelClosureVarReg(closure_reg);
    builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));
    return root_anchor;
  }

  NopInst* AddRootAnyAnchor(FuncOp* root_func) {
    auto* builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToStart(&root_func->Front());
    auto* root_anchor = builder->Create<NopInst>(0);
    root_anchor->SetType(TypeOp::CreateAnyType(builder));
    builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));
    return root_anchor;
  }

  void SetAllocatedRegister(FuncOp* func, Value* value, unsigned reg) {
    auto* target_ctx = ir_ctx->GetTargetContext();
    ASSERT_NE(nullptr, target_ctx);
    std::unique_ptr<RegisterAllocator> ra =
        std::make_unique<RegisterAllocator>(func);
    ra->InsertRegister(value, Register(reg));
    target_ctx->SetRegisterAllocAnalysis(func, ra);
  }

  void SetEmptyRegisterAllocation(FuncOp* func) {
    auto* target_ctx = ir_ctx->GetTargetContext();
    ASSERT_NE(nullptr, target_ctx);
    std::unique_ptr<RegisterAllocator> ra =
        std::make_unique<RegisterAllocator>(func);
    target_ctx->SetRegisterAllocAnalysis(func, ra);
  }

  void SetBuiltRegisterAllocation(FuncOp* func) {
    std::unique_ptr<RegisterAllocator> ra(BuildTestRegisterAllocation(func));
    ir_ctx->GetTargetContext()->SetRegisterAllocAnalysis(func, ra);
  }

  bool RunUpdateToplevelClosureVarPass() {
    auto* opt_pass = CreateUpdateToplevelClosureVarPass(ir_ctx.get());
    bool changed = static_cast<ModulePass*>(opt_pass)->RunOnModule(mod);
    delete opt_pass;
    return changed;
  }

  void ExpectToplevelClosureOps(FuncOp* func, uint32_t expected_reg) {
    auto lepus_func = func->GetLepusFunction();
    ASSERT_NE(lepus_func.get(), nullptr);
    bool saw_get = false;
    bool saw_set = false;
    for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
      auto* inst = lepus_func->GetInstruction(i);
      ASSERT_NE(inst, nullptr);
      const long op = lepus::Instruction::GetOpCode(*inst);
      if (op == lepus::TypeOp_GetToplevelClosureVar) {
        saw_get = true;
        EXPECT_EQ(lepus::Instruction::GetParamB(*inst), expected_reg);
      }
      if (op == lepus::TypeOp_SetToplevelClosureVar) {
        saw_set = true;
        EXPECT_EQ(lepus::Instruction::GetParamB(*inst), expected_reg);
      }
    }
    EXPECT_TRUE(saw_get);
    EXPECT_TRUE(saw_set);
  }

 private:
  std::unique_ptr<VMContext> vm_ctx_;
};

TEST_F(LEPUSIRRootFuncDeoptTest,
       NonToplevelRegisterOverflowThrowsCompileError) {
  auto root_lepus = Function::Create();
  root_lepus->SetFunctionName("root");
  root_lepus->SetTopLevelFunction(true);
  auto child_lepus = Function::Create();
  child_lepus->SetFunctionName("overflow_child");
  root_lepus->AddChildFunction(child_lepus);

  auto* root_func = CreateFunctionForLepus("root", root_lepus, true);
  auto* child_func = CreateFunctionForLepus("overflow_child", child_lepus);

  auto* root_anchor = AddAnchorInstruction(root_func, 1);
  auto* child_anchor = AddAnchorInstruction(child_func, 2);

  SetAllocatedRegister(root_func, root_anchor, 1);
  SetAllocatedRegister(child_func, child_anchor, Register::kMaxRegistersLimit);

  auto* target_ctx = ir_ctx->GetTargetContext();
  ASSERT_NE(nullptr, target_ctx);
  try {
    target_ctx->PlanRootFuncDeopt(mod);
    FAIL() << "Expected PlanRootFuncDeopt to reject non-toplevel overflow";
  } catch (const ::lynx::lepus::CompileException& e) {
    const std::string msg = e.message();
    ASSERT_NE(msg.find("only top-level function register overflow can use root-"
                       "function deopt"),
              std::string::npos)
        << msg;
    ASSERT_NE(msg.find("overflow_child"), std::string::npos) << msg;
  }
}

TEST_F(LEPUSIRRootFuncDeoptTest, RootOverflowFallsBackOnlyRootFunction) {
  auto root_lepus = Function::Create();
  root_lepus->SetFunctionName("root");
  root_lepus->SetTopLevelFunction(true);
  auto child_lepus = Function::Create();
  child_lepus->SetFunctionName("child");
  child_lepus->AddUpvalue("captured", 0, true);
  root_lepus->AddChildFunction(child_lepus);

  auto* root_func = CreateFunctionForLepus("root", root_lepus, true);
  auto* child_func = CreateFunctionForLepus("child", child_lepus);

  auto* root_anchor = AddAnchorInstruction(root_func, 1);
  auto* child_anchor = AddAnchorInstruction(child_func, 2);

  SetAllocatedRegister(root_func, root_anchor, Register::kMaxRegistersLimit);
  SetAllocatedRegister(child_func, child_anchor, 1);

  auto* target_ctx = ir_ctx->GetTargetContext();
  ASSERT_NE(nullptr, target_ctx);
  target_ctx->PlanRootFuncDeopt(mod);

  EXPECT_TRUE(target_ctx->IsRootFuncDeopt());
}

TEST(LEPUSIRRootFuncDeoptTargetContextTest,
     ForcedRootOnlyDeoptMarksOnlyRootFunction) {
  VMContext vm_ctx;
  auto local_ir_ctx = std::make_unique<IRContext>(&vm_ctx);
  local_ir_ctx->GetTargetContext()->SetForceRootFuncDeopt(true);
  std::unique_ptr<TargetContext> target_ctx = std::make_unique<TargetContext>();
  local_ir_ctx->SetTargetContext(target_ctx);
  auto* local_mod = local_ir_ctx->GetMainMod();
  auto* builder = local_ir_ctx->GetOpBuilder();

  auto root_lepus = Function::Create();
  root_lepus->SetFunctionName("root");
  root_lepus->SetTopLevelFunction(true);
  auto child_lepus = Function::Create();
  child_lepus->SetFunctionName("child");
  child_lepus->AddUpvalue("captured", 0, true);
  root_lepus->AddChildFunction(child_lepus);

  builder->SetInsertionPointToEnd(local_mod->GetFunctionBlock());
  auto root_name = std::string("root");
  auto* root_func = builder->Create<FuncOp>(0, root_name);
  root_func->Init(root_lepus);
  root_func->SetTopLevelFunction();
  local_mod->SetRootFunction(root_func);
  builder->CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});

  auto child_name = std::string("child");
  auto* child_func = builder->Create<FuncOp>(0, child_name);
  child_func->Init(child_lepus);
  builder->CreateBlock(child_func->GetSingleRegion(), BlockType::BT_INST, {});

  local_ir_ctx->GetTargetContext()->PlanRootFuncDeopt(local_mod);

  EXPECT_TRUE(local_ir_ctx->GetTargetContext()->IsRootFuncDeopt());
}

TEST_F(LEPUSIRRootFuncDeoptTest,
       UpdateToplevelClosureVarUsesRootFuncDeoptSlotsWhenRootFallsBack) {
  auto funcs = CreateRootAndChildFunctions("root", "child");
  auto* root_anchor = AddRecordedRootClosureAnchor(funcs.root_func, 10);
  funcs.child_lepus_func->AddUpvalue("x", 10, true);

  // Real compiled programs populate VMContext's toplevel name->slot table
  // during code generation / deserialization. Mirror that here so root-function
  // deopt resolves by upvalue name instead of relying on test-only shortcuts.
  ir_ctx->GetVMContext()->UpdateToplevelVarReg("x", 10);

  SetAllocatedRegister(funcs.root_func, root_anchor,
                       Register::kMaxRegistersLimit);
  SetEmptyRegisterAllocation(funcs.child_func);

  RunUpdateToplevelClosureVarPass();

  EXPECT_EQ(funcs.child_func->GetClosureVarToplevelReg(0), 10);
  ASSERT_EQ(funcs.child_lepus_func->UpvaluesSize(), 1u);
  EXPECT_EQ(funcs.child_lepus_func->GetUpvalue(0)->register_, 10);
  EXPECT_TRUE(ir_ctx->GetTargetContext()->IsRootFuncDeopt());
}

TEST_F(
    LEPUSIRRootFuncDeoptTest,
    UpdateToplevelClosureVarDoesNotRewriteSerializedUpvaluesWhenRootFallsBack) {
  auto funcs = CreateRootAndChildFunctions("root", "child_named_slot");
  auto* root_anchor = AddRecordedRootClosureAnchor(funcs.root_func, 3);
  funcs.child_lepus_func->AddUpvalue("captured", 3, true);

  SetAllocatedRegister(funcs.root_func, root_anchor,
                       Register::kMaxRegistersLimit);
  SetEmptyRegisterAllocation(funcs.child_func);

  ir_ctx->GetVMContext()->UpdateToplevelVarReg("captured", 11);

  RunUpdateToplevelClosureVarPass();

  EXPECT_TRUE(ir_ctx->GetTargetContext()->IsRootFuncDeopt());
  EXPECT_EQ(funcs.child_func->GetClosureVarToplevelReg(0), 11);
  ASSERT_EQ(funcs.child_lepus_func->UpvaluesSize(), 1u);
  EXPECT_EQ(funcs.child_lepus_func->GetUpvalue(0)->register_, 3);
  EXPECT_TRUE(funcs.child_lepus_func->GetUpvalue(0)->in_parent_vars_);
}

TEST_F(LEPUSIRRootFuncDeoptTest,
       InstructionSelectionUsesToplevelClosureOpsWhenRootFallsBack) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  auto funcs = CreateRootAndChildFunctions("root", "child");
  auto* root_anchor = AddRecordedRootClosureAnchor(funcs.root_func, 7);
  funcs.child_lepus_func->AddUpvalue("captured", 7, true);

  // Keep the test aligned with the real pipeline: the root-function deopt slot
  // lookup is name-based and expects VMContext to already know this toplevel
  // variable.
  ir_ctx->GetVMContext()->UpdateToplevelVarReg("captured", 7);

  builder.SetInsertionPointToStart(&funcs.child_func->Front());
  auto* get_upvalue = builder.Create<GetUpvalueInst>(
      0, funcs.child_func, builder.GetLiteralUint8(0));
  builder.Create<SetUpvalueInst>(0, funcs.child_func,
                                 builder.GetLiteralUint8(0), get_upvalue);
  builder.Create<ReturnInst>(0, get_upvalue);

  SetAllocatedRegister(funcs.root_func, root_anchor,
                       Register::kMaxRegistersLimit);
  SetBuiltRegisterAllocation(funcs.child_func);

  ASSERT_TRUE(RunUpdateToplevelClosureVarPass());

  InstructionSelectionPass is_pass(ir_ctx.get());
  ASSERT_TRUE(is_pass.RunOnFunction(funcs.child_func));

  EXPECT_EQ(funcs.child_lepus_func->UpvaluesSize(), 0u);
  ExpectToplevelClosureOps(funcs.child_func, 7);
}

TEST_F(LEPUSIRRootFuncDeoptTest,
       InstructionSelectionThrowsWhenRootFallbackClosureMapIsMissing) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  auto funcs = CreateRootAndChildFunctions("root", "child_missing_map");
  auto* root_anchor = AddRecordedRootClosureAnchor(funcs.root_func, 7);
  funcs.child_lepus_func->AddUpvalue("captured", 7, true);

  builder.SetInsertionPointToStart(&funcs.child_func->Front());
  auto* get_upvalue = builder.Create<GetUpvalueInst>(
      0, funcs.child_func, builder.GetLiteralUint8(0));
  builder.Create<SetUpvalueInst>(0, funcs.child_func,
                                 builder.GetLiteralUint8(0), get_upvalue);
  builder.Create<ReturnInst>(0, get_upvalue);

  SetAllocatedRegister(funcs.root_func, root_anchor,
                       Register::kMaxRegistersLimit);
  SetBuiltRegisterAllocation(funcs.child_func);
  ir_ctx->GetTargetContext()->PlanRootFuncDeopt(mod);
  ASSERT_TRUE(ir_ctx->GetTargetContext()->IsRootFuncDeopt());
  EXPECT_EQ(funcs.child_func->GetClosureVarToplevelReg(0),
            constants::kInvalidSignedValue);

  InstructionSelectionPass is_pass(ir_ctx.get());
  try {
    is_pass.RunOnFunction(funcs.child_func);
    FAIL() << "Expected missing closure map to throw";
  } catch (const ::lynx::lepus::CompileException& e) {
    const std::string msg = e.message();
    ASSERT_NE(msg.find("requires resolved toplevel reg mapping"),
              std::string::npos)
        << msg;
  }
}

TEST_F(LEPUSIRRootFuncDeoptTest,
       InstructionSelectionExplicitToplevelClosureAccessUsesRootFuncDeoptSlot) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  auto funcs = CreateRootAndChildFunctions("root", "child_explicit");
  auto* root_anchor = AddRootAnyAnchor(funcs.root_func);

  builder.SetInsertionPointToStart(&funcs.child_func->Front());
  auto* get_closure = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(11), TypeOp::CreateAnyType(&builder));
  builder.Create<SetToplevelClosureVarInst>(0, builder.GetLiteralUint32(11),
                                            get_closure);
  builder.Create<ReturnInst>(0, get_closure);

  SetAllocatedRegister(funcs.root_func, root_anchor,
                       Register::kMaxRegistersLimit);
  SetBuiltRegisterAllocation(funcs.child_func);
  ir_ctx->GetTargetContext()->PlanRootFuncDeopt(mod);

  InstructionSelectionPass is_pass(ir_ctx.get());
  ASSERT_TRUE(is_pass.RunOnFunction(funcs.child_func));
  ExpectToplevelClosureOps(funcs.child_func, 11);
}

TEST(LEPUSIRRootFuncDeoptDeepCloneTest,
     LowerDeepCloneCallsInNestedFunctionsWhenRootFallsBack) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function main() {
      let clonedObject = $deepClone({ x: 1 });
      let clonedArray = $deepClone([1, 2, 3]);
      let clonedBool = $deepClone(true);
      if (clonedObject.x === 1 && clonedArray.length === 3 && clonedBool === true) {
        return 1;
      }
      return 0;
    }

    let r = main();
    if (r === 1) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  ir_ctx->GetTargetContext()->SetForceRootFuncDeopt(true);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* root_func_op = mod->GetRootFunction();
  ASSERT_NE(nullptr, root_func_op);
  EXPECT_TRUE(ir_ctx->GetTargetContext()->IsRootFuncDeopt());

  auto* main_func_op = FindFuncOpByName(mod, "main");
  ASSERT_NE(nullptr, main_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(main_func_op), 3);
  EXPECT_EQ(CountDeepCloneOpcodes(main_func_op), 3);
}

TEST(LEPUSIRRootFuncDeoptDeepCloneTest,
     PreserveDeepCloneFunctionBytecodeWhenRootFallsBack) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    let clonedObject = $deepClone({});
    let clonedArray = $deepClone([]);
    if (clonedObject == {} && clonedArray == []) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  ir_ctx->GetTargetContext()->SetForceRootFuncDeopt(true);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* root_func_op = mod->GetRootFunction();
  ASSERT_NE(nullptr, root_func_op);
  EXPECT_TRUE(ir_ctx->GetTargetContext()->IsRootFuncDeopt());

  auto* deep_clone_func_op = FindFuncOpByName(mod, constants::kDeepCloneName);
  ASSERT_NE(nullptr, deep_clone_func_op);
  auto deep_clone_func = deep_clone_func_op->GetLepusFunction();
  ASSERT_NE(nullptr, deep_clone_func.get());
  EXPECT_GT(deep_clone_func->OpCodeSize(), 0u);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
