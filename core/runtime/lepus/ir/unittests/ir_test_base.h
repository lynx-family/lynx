// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_UNITTESTS_IR_TEST_BASE_H_
#define CORE_RUNTIME_LEPUS_IR_UNITTESTS_IR_TEST_BASE_H_

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/target_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class IRTestBase : public ::testing::Test {
 public:
  IRTestBase() = default;

  virtual void SetUp(void) {
    ir_ctx = std::make_unique<IRContext>(nullptr);
    std::unique_ptr<TargetContext> target_ctx =
        std::make_unique<TargetContext>();
    ir_ctx->SetTargetContext(target_ctx);
    mod = ir_ctx->GetMainMod();
  }
  virtual void TearDown(void) {
    ir_ctx.reset();
    mod = nullptr;
  }

  void CreateTestFuncOp(std::string& name, FuncOp*& func_op) {
    auto mod = ir_ctx->GetMainMod();
    ASSERT_NE(nullptr, mod);

    auto builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

    func_op = builder->Create<FuncOp>(0, name);
    if (!func_op) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: IRTestBase::CreateTestFuncOp failed to create "
          "FuncOp");
    }

    auto region = builder->CreateRegion(func_op);
    ASSERT_NE(nullptr, region);
    auto block = builder->CreateBlock(region, BlockType::BT_INST, {});
    ASSERT_NE(nullptr, block);

    builder->SetInsertionPointToEnd(block);
    builder->Create<ReturnInst>(1, builder->GetLiteralInt32(0));
    ASSERT_EQ(1, block->size());
  }

  void RegisterBuiltin(lepus::VMContext* context);

 protected:
  ModuleOp* mod;
  std::unique_ptr<IRContext> ir_ctx;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_UNITTESTS_IR_TEST_BASE_H_
