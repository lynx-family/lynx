// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/pass_manager/pipeline.h"
#include "core/runtime/lepus/parser.h"
#include "core/runtime/lepus/scanner.h"
#include "core/runtime/lepus/semantic_analysis.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

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
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
