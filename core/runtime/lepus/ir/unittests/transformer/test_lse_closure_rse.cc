// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/construct_ssa_ir.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/type_specification.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestLSEClosureRSE : public IRTestBase {
 public:
  virtual void SetUp(void) {
    vm_ctx = std::make_unique<VMContext>();
    vm_ctx->Initialize();
    // These tests validate MIR passes (LSE) on IR built from *raw* bytecode.
    // Disable bytecode O1 optimization to avoid double-optimizing before
    // running the pass under test.
    vm_ctx->SetOptBytecode(false);
    ir_ctx = std::make_unique<IRContext>(vm_ctx.get());
    mod = ir_ctx->GetMainMod();
  }

  std::unique_ptr<VMContext> vm_ctx;

  int count_stores(FuncOp* f) {
    int count = 0;
    for (auto& block : *f) {
      for (auto& inst : block) {
        if (llvh::isa<SetContextSlotInst>(&inst) ||
            llvh::isa<SetContextSlotMovInst>(&inst) ||
            llvh::isa<SetUpvalueInst>(&inst)) {
          count++;
        }
      }
    }
    return count;
  };

  FuncOp* prepareFunc(const std::string& source, int child_idx = 0) {
    std::string error =
        BytecodeGenerator::GenerateBytecode(vm_ctx.get(), source, "3.8", "");
    if (!error.empty()) {
      std::cerr << "Bytecode Gen Error: " << error << std::endl;
      return nullptr;
    }

    auto root = vm_ctx->GetRootFunction();
    if (!root || root->GetChildFunction().size() <= child_idx) {
      return nullptr;
    }
    auto child_lepus = root->GetChildFunction()[child_idx];

    OpBuilder builder;
    builder.SetModuleOp(mod);
    builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
    FuncOp* func_op = builder.Create<FuncOp>(0, child_lepus->GetFunctionName());
    func_op->Init(child_lepus);

    ConstructSSAIRPass ssa_pass(ir_ctx.get());
    ssa_pass.RunOnFunction(func_op);
    return func_op;
  }
};

TEST_F(LEPUSIRTestLSEClosureRSE, JSClosureRedundantStore) {
  std::string source = R"(
    let x = 10;
    function inner() {
        x = 20;
        x = 20; 
    }
    inner();
  )";

  auto* func_op = prepareFunc(source);
  ASSERT_NE(func_op, nullptr);

  int store_count_before = count_stores(func_op);
  ASSERT_GE(store_count_before, 2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  int store_count_after = count_stores(func_op);
  EXPECT_EQ(store_count_after, 1);
}

TEST_F(LEPUSIRTestLSEClosureRSE, JSClosureReadThenWriteBack) {
  std::string source = R"(
    let x = 10;
    function inner() {
        let t = x;
        x = t;
    }
    inner();
  )";

  auto* func_op = prepareFunc(source);
  ASSERT_NE(func_op, nullptr);

  int store_count_before = count_stores(func_op);
  ASSERT_GE(store_count_before, 1);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  int store_count_after = count_stores(func_op);
  EXPECT_EQ(store_count_after, 0);
}

TEST_F(LEPUSIRTestLSEClosureRSE, JSClosureStoreSafetyAcrossCall) {
  std::string source = R"(
    let x = 10;
    function inner() {
        x = 20;
        Math.abs(x);
        x = 20; 
    }
    inner();
  )";

  auto* func_op = prepareFunc(source);
  ASSERT_NE(func_op, nullptr);

  int store_count_before = count_stores(func_op);
  ASSERT_GE(store_count_before, 2);

  TypeSpecification type_spec(ir_ctx.get());
  type_spec.RunOnFunction(func_op);
  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  int store_count_after = count_stores(func_op);
  // Math.abs is modeled as a read-only call (no heap/frame writes), so the
  // second identical store is redundant and can be eliminated safely.
  EXPECT_EQ(store_count_after, 1);
}

TEST_F(LEPUSIRTestLSEClosureRSE, JSClosureOuterReadInnerWrite) {
  std::string source = R"(
    let x = 1;
    function inner() {
        x = 2;
    }
    inner();
    let y = x;
  )";

  auto* func_op = prepareFunc(source);
  ASSERT_NE(func_op, nullptr);

  int store_count_before = count_stores(func_op);
  ASSERT_GE(store_count_before, 1);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  int store_count_after = count_stores(func_op);
  // Inner store must be kept!
  EXPECT_EQ(store_count_after, 1);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
