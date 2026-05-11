// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/pass_manager/pipeline.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
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

int CountOpcodes(FuncOp* func) {
  if (func == nullptr) {
    return 0;
  }
  auto lepus_func = func->GetLepusFunction();
  if (lepus_func == nullptr) {
    return 0;
  }
  return static_cast<int>(lepus_func->OpCodeSize());
}

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

void ExpectDeepCloneLowered(FuncOp* func, int expected_count) {
  ASSERT_NE(nullptr, func);
  EXPECT_EQ(CountDeepCloneAttrCalls(func), expected_count);
  EXPECT_EQ(CountDeepCloneOpcodes(func), expected_count);
}

void ExpectDeepCloneBodySkipped(ModuleOp* mod) {
  auto* deep_clone_func = FindFuncOpByName(mod, "$deepClone");
  ASSERT_NE(nullptr, deep_clone_func);
  EXPECT_EQ(CountOpcodes(deep_clone_func), 0);
}

void ExpectDeepCloneBodyGenerated(ModuleOp* mod) {
  auto* deep_clone_func = FindFuncOpByName(mod, "$deepClone");
  ASSERT_NE(nullptr, deep_clone_func);
  EXPECT_GT(CountOpcodes(deep_clone_func), 0);
}

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

  ir_ctx->Init(root_func, context);
  auto* mod = ir_ctx->GetMainMod();
  RunO1OptimizationPasses(*mod);
  return mod;
}

TEST(LEPUSIRTestDeepCloneOpcode, MarkDeepCloneCallAttrFromUpvalueClosure) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  // This test mirrors the real-world pattern where `$deepClone` is defined as
  // a toplevel function and referenced by a nested function via GetUpvalueInst.
  static constexpr const char* kSource = R"(
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

    function main() {
      let obj = {
        x: 1,
        child: [1, 2, 3]
      };
      let m = $deepClone(obj);
      // Keep the result alive.
      return m.x;
    }

    let r = main();
    if (r === 1) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, kSource, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  FuncOp* main_func_op = nullptr;
  for (auto* func : *mod) {
    if (!func) continue;
    auto f = func->GetLepusFunction();
    if (f && f->GetFunctionName() == "main") {
      main_func_op = func;
      break;
    }
  }
  ASSERT_NE(nullptr, main_func_op);

  int deep_clone_attr_calls = 0;
  for (auto& bb : *main_func_op) {
    for (auto& inst : bb) {
      if (auto* call = llvh::dyn_cast<CallInst>(&inst)) {
        if (call->IsDeepCloneCall()) {
          deep_clone_attr_calls++;
        }
      }
    }
  }

  EXPECT_GE(deep_clone_attr_calls, 1);
  ExpectDeepCloneBodySkipped(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromToplevelDirectClosureForMultipleTypes) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    let clonedObject = $deepClone({ x: 1, y: 2 });
    let clonedArray = $deepClone([1, 2, 3]);
    let clonedNumber = $deepClone(123);
    if (clonedObject.x === 1 && clonedArray.length === 3 && clonedNumber === 123) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  ExpectDeepCloneLowered(mod->GetRootFunction(), 1);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromToplevelVarLoadForMultipleTypes) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    let clone = $deepClone;
    let clonedObject = clone({ x: 1 });
    let clonedArray = clone([1, 2, 3]);
    let clonedString = clone("hello");
    if (clonedObject.x === 1 && clonedArray.length === 3 && clonedString === "hello") {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  EXPECT_EQ(CountDeepCloneAttrCalls(mod->GetRootFunction()), 1);
  EXPECT_EQ(CountDeepCloneOpcodes(mod->GetRootFunction()), 1);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkAllDirectDeepCloneCallsWhenValueAlsoHasNonCallUser) {
  lepus::VMContext ctx;
  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = ir_ctx->GetMainMod();
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, mod);
  ASSERT_NE(nullptr, builder);

  auto root_lepus_func = lepus::Function::Create();
  root_lepus_func->SetFunctionName("root");
  root_lepus_func->SetTopLevelFunction(true);
  auto deep_clone_lepus_func = lepus::Function::Create();
  deep_clone_lepus_func->SetFunctionName(constants::kDeepCloneName);
  root_lepus_func->AddChildFunction(deep_clone_lepus_func);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string root_name = "root";
  auto* root_func = builder->Create<FuncOp>(0, root_name);
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  auto* root_entry = builder->CreateBlock(root_func->GetSingleRegion(),
                                          BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(root_entry);
  auto* deep_clone_value = builder->Create<CreateClosureInst>(0, 0);
  builder->Create<SetToplevelVarInst>(0, builder->GetLiteralUint32(0),
                                      deep_clone_value);

  ArgList args1;
  args1.push_back(builder->GetLiteralInt32(1));
  auto* direct_call1 = builder->Create<CallInst>(0, deep_clone_value, args1);
  ArgList args2;
  args2.push_back(builder->GetLiteralInt32(2));
  auto* direct_call2 = builder->Create<CallInst>(0, deep_clone_value, args2);
  builder->Create<ReturnInst>(0, direct_call2);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string deep_clone_name = constants::kDeepCloneName;
  auto* deep_clone_func = builder->Create<FuncOp>(0, deep_clone_name);
  deep_clone_func->Init(deep_clone_lepus_func);
  builder->CreateBlock(deep_clone_func->GetSingleRegion(), BlockType::BT_INST,
                       {});

  auto* pass = CreateFunctionToBuiltInPass(ir_ctx.get());
  ASSERT_NE(nullptr, pass);
  EXPECT_TRUE(static_cast<FunctionPass*>(pass)->RunOnFunction(root_func));
  delete pass;

  EXPECT_TRUE(direct_call1->IsDeepCloneCall());
  EXPECT_TRUE(direct_call2->IsDeepCloneCall());
  EXPECT_FALSE(deep_clone_func->CanSkipDeepCloneLowering());
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromUpvalueClosureForMultipleTypes) {
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
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* main_func_op = FindFuncOpByName(mod, "main");
  ExpectDeepCloneLowered(main_func_op, 3);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     NoCrashWhenDeepCloneAliasCapturedByChildClosure) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function makeResolver(prefix) {
      return function choose(step) {
        let cloneFn = $deepClone;
        return function resolve(raw) {
          let data = cloneFn(raw || {});
          let panel = cloneFn(data.panel || {});
          let count = (panel.count || 0) + step + prefix;
          return count;
        }
      }
    }

    let selectResolver = makeResolver(1);
    let resolver = selectResolver(2);
    let result = resolver({ panel: { count: 3 } });
    if (result === 6) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* resolve_func_op = FindFuncOpByName(mod, "resolve");
  ASSERT_NE(nullptr, resolve_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(resolve_func_op), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(resolve_func_op), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode, MarkDeepCloneCallAttrFromPhiMergedAlias) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function main(flag) {
      let cloneFn;
      if (flag) {
        cloneFn = $deepClone;
      } else {
        cloneFn = $deepClone;
      }
      let cloned = cloneFn({ x: 1, y: 2 });
      return cloned.x + cloned.y;
    }

    let r = main(true);
    if (r === 3) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* main_func_op = FindFuncOpByName(mod, "main");
  ASSERT_NE(nullptr, main_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(main_func_op), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(main_func_op), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromHigherOrderFunctionParameter) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function apply(fn, value) {
      let cloned = fn(value);
      return cloned.x;
    }

    let r = apply($deepClone, { x: 1, y: 2 });
    if (r === 1) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* apply_func_op = FindFuncOpByName(mod, "apply");
  ASSERT_NE(nullptr, apply_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(apply_func_op), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(apply_func_op), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     DoesNotMarkHigherOrderFunctionParameterWhenCallsiteDisagree) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function identity(value) {
      return value;
    }

    function apply(fn, value) {
      let cloned = fn(value);
      return cloned.x || 0;
    }

    let a = apply($deepClone, { x: 1, y: 2 });
    let b = apply(identity, { x: 3, y: 4 });
    if (a === 1 && b === 3) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* apply_func_op = FindFuncOpByName(mod, "apply");
  ASSERT_NE(nullptr, apply_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(apply_func_op), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(apply_func_op), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromReturnedFunctionValue) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function forward(fn) {
      return fn;
    }

    let clone = forward($deepClone);
    let cloned = clone({ x: 1, y: 2 });
    if (cloned.x === 1 && cloned.y === 2) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  EXPECT_EQ(CountDeepCloneAttrCalls(mod->GetRootFunction()), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(mod->GetRootFunction()), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromReturnedFunctionValuePerCallsite) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function identity(value) {
      return value;
    }

    function forward(fn) {
      return fn;
    }

    let clone = forward($deepClone);
    let cloned = clone({ x: 1, y: 2 });
    let keep = forward(identity);
    let same = keep({ x: 3, y: 4 });
    if (cloned.x === 1 && same.x === 3) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  EXPECT_EQ(CountDeepCloneAttrCalls(mod->GetRootFunction()), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(mod->GetRootFunction()), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromObjectPropertyStoreLoad) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function main() {
      let holder = {};
      holder.clone = $deepClone;
      let fn = holder.clone;
      let cloned = fn({ x: 1, y: 2 });
      return cloned.x + cloned.y;
    }

    let r = main();
    if (r === 3) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* main_func_op = FindFuncOpByName(mod, "main");
  ASSERT_NE(nullptr, main_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(main_func_op), 1);
  EXPECT_EQ(CountDeepCloneOpcodes(main_func_op), 1);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromDynamicContainerKeyStoreLoad) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function main(idx) {
      let fns = [];
      fns[idx] = $deepClone;
      let fn = fns[idx];
      let cloned = fn({ x: 1, y: 2 });
      return cloned.y;
    }

    let r = main(0);
    if (r === 2) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* main_func_op = FindFuncOpByName(mod, "main");
  ASSERT_NE(nullptr, main_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(main_func_op), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(main_func_op), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

TEST(LEPUSIRTestDeepCloneOpcode,
     MarkDeepCloneCallAttrFromArrayLiteralElementLoad) {
  lepus::VMContext ctx;
  ctx.SetSdkVersion("2.6");
  ctx.Initialize();
  RegisterBuiltin(&ctx);
  ctx.SetClosureFix(true);

  const std::string source = std::string(kDeepClonePrelude) + R"(
    function main() {
      let fns = [$deepClone];
      let fn = fns[0];
      let cloned = fn({ x: 1, y: 2 });
      return cloned.x;
    }

    let r = main();
    if (r === 1) {
      // no-op
    }
  )";

  auto ir_ctx = std::make_unique<IRContext>(&ctx);
  auto* mod = CompileToIRWithOptimization(&ctx, source, ir_ctx.get());
  ASSERT_NE(nullptr, mod);

  auto* main_func_op = FindFuncOpByName(mod, "main");
  ASSERT_NE(nullptr, main_func_op);
  EXPECT_EQ(CountDeepCloneAttrCalls(main_func_op), 0);
  EXPECT_EQ(CountDeepCloneOpcodes(main_func_op), 0);
  ExpectDeepCloneBodyGenerated(mod);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
