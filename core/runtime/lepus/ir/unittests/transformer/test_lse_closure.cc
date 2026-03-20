// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/construct_ssa_ir.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestLSEClosure : public IRTestBase {
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
};

TEST_F(LEPUSIRTestLSEClosure, ContextSlotRedundancy) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* depth = builder.GetLiteralUint8(0);
  auto* index = builder.GetLiteralUint8(1);
  auto* get1 = builder.Create<GetContextSlotInst>(0, (LiteralUint8*)depth,
                                                  (LiteralUint8*)index);
  auto* get2 = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth, (LiteralUint8*)index);  // Redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get1);
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLSEClosure, ContextSlotForwarding) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* depth = builder.GetLiteralUint8(0);
  auto* index = builder.GetLiteralUint8(1);
  auto* val = builder.GetLiteralInt32(42);

  builder.Create<SetContextSlotInst>(0, (LiteralUint8*)depth,
                                     (LiteralUint8*)index, val);
  auto* get = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth, (LiteralUint8*)index);  // Forwarded
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get) found_get = true;
  }
  ASSERT_FALSE(found_get);
}

TEST_F(LEPUSIRTestLSEClosure, GetContextSlotMovRedundancy) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* ctx = builder.GetLiteralInt32(0);  // Mock context value
  auto* index = builder.GetLiteralUint8(1);
  auto* get1 =
      builder.Create<GetContextSlotMovInst>(0, ctx, (LiteralUint8*)index);
  auto* get2 = builder.Create<GetContextSlotMovInst>(
      0, ctx, (LiteralUint8*)index);  // Redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get1);
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLSEClosure, ContextSlotInvalidationByCall) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* depth = builder.GetLiteralUint8(0);
  auto* index = builder.GetLiteralUint8(1);
  builder.Create<GetContextSlotInst>(0, (LiteralUint8*)depth,
                                     (LiteralUint8*)index);

  // Call invalidates mutable state including context slots
  ArgList args;
  builder.Create<CallInst>(0, builder.GetLiteralInt32(0), args);

  auto* get2 = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth, (LiteralUint8*)index);  // NOT redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);
}

TEST_F(LEPUSIRTestLSEClosure, ContextSlotSafeAcrossAllocation) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* depth = builder.GetLiteralUint8(0);
  auto* index = builder.GetLiteralUint8(1);
  [[maybe_unused]] auto* get1 = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth, (LiteralUint8*)index);

  // NewTable should NOT invalidate context slots
  builder.Create<NewTableInst>(0);

  auto* get2 = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth, (LiteralUint8*)index);  // Redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLSEClosure, NestedContextDepth) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* depth0 = builder.GetLiteralUint8(0);
  auto* depth1 = builder.GetLiteralUint8(1);
  auto* index = builder.GetLiteralUint8(1);

  builder.Create<GetContextSlotInst>(0, (LiteralUint8*)depth0,
                                     (LiteralUint8*)index);
  builder.Create<GetContextSlotInst>(0, (LiteralUint8*)depth1,
                                     (LiteralUint8*)index);

  auto* get3 = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth0, (LiteralUint8*)index);  // Redundant
  auto* get4 = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth1, (LiteralUint8*)index);  // Redundant

  builder.Create<ReturnInst>(0, get3);
  builder.Create<ReturnInst>(0, get4);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get3 = false;
  bool found_get4 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get3) found_get3 = true;
    if (inst == get4) found_get4 = true;
  }
  ASSERT_FALSE(found_get3);
  ASSERT_FALSE(found_get4);
}

TEST_F(LEPUSIRTestLSEClosure, JSClosureRedundancy) {
  std::string source = R"(
    let x = 1;
    function inner() {
        return x + x;
    }
    inner();
  )";

  std::string error =
      BytecodeGenerator::GenerateBytecode(vm_ctx.get(), source, "3.8", "");
  ASSERT_TRUE(error.empty());

  auto root = vm_ctx->GetRootFunction();
  ASSERT_NE(root, nullptr);

  ASSERT_GE(root->GetChildFunction().size(), 1);
  auto inner_lepus = root->GetChildFunction()[0];

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  FuncOp* func_op = builder.Create<FuncOp>(0, inner_lepus->GetFunctionName());
  func_op->Init(inner_lepus);

  ConstructSSAIRPass ssa_pass(ir_ctx.get());
  ssa_pass.RunOnFunction(func_op);

  auto count_loads = [](FuncOp* f) {
    int count = 0;
    for (auto& block : *f) {
      for (auto& inst : block) {
        if (llvh::isa<GetContextSlotInst>(&inst) ||
            llvh::isa<GetContextSlotMovInst>(&inst) ||
            llvh::isa<GetUpvalueInst>(&inst)) {
          count++;
        }
      }
    }
    return count;
  };

  int get_count_before = count_loads(func_op);
  ASSERT_GE(get_count_before, 2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  int get_count_after = count_loads(func_op);
  EXPECT_EQ(get_count_after, 1);
}

TEST_F(LEPUSIRTestLSEClosure, JSClosureForwarding) {
  std::string source = R"(
    let x = 1;
    function inner() {
        x = 42;
        return x;
    }
    inner();
  )";

  std::string error =
      BytecodeGenerator::GenerateBytecode(vm_ctx.get(), source, "3.8", "");
  ASSERT_TRUE(error.empty());

  auto root = vm_ctx->GetRootFunction();
  auto inner_lepus = root->GetChildFunction()[0];

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  FuncOp* func_op = builder.Create<FuncOp>(0, inner_lepus->GetFunctionName());
  func_op->Init(inner_lepus);

  ConstructSSAIRPass ssa_pass(ir_ctx.get());
  ssa_pass.RunOnFunction(func_op);

  auto count_loads = [](FuncOp* f) {
    int count = 0;
    for (auto& block : *f) {
      for (auto& inst : block) {
        if (llvh::isa<GetContextSlotInst>(&inst) ||
            llvh::isa<GetContextSlotMovInst>(&inst) ||
            llvh::isa<GetUpvalueInst>(&inst)) {
          count++;
        }
      }
    }
    return count;
  };

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func_op);

  int get_count_after = count_loads(func_op);
  EXPECT_EQ(get_count_after, 0);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
