// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestLSERedundantStore : public IRTestBase {
 public:
  virtual void SetUp(void) {
    vm_ctx = std::make_unique<VMContext>();
    vm_ctx->Initialize();
    ir_ctx = std::make_unique<IRContext>(vm_ctx.get());
    mod = ir_ctx->GetMainMod();
  }

  std::unique_ptr<VMContext> vm_ctx;
};

TEST_F(LEPUSIRTestLSERedundantStore, RedundantContextStore) {
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
  auto* get = builder.Create<GetContextSlotInst>(0, (LiteralUint8*)depth,
                                                 (LiteralUint8*)index);
  auto* set = builder.Create<SetContextSlotInst>(
      0, (LiteralUint8*)depth, (LiteralUint8*)index, get);  // Redundant store
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_set = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == set) found_set = true;
  }
  EXPECT_FALSE(found_set) << "Redundant SetContextSlotInst should be removed";
}

TEST_F(LEPUSIRTestLSERedundantStore, RedundantContextMovStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* ctx = builder.GetLiteralInt32(0);
  auto* index = builder.GetLiteralUint8(1);
  auto* get =
      builder.Create<GetContextSlotMovInst>(0, ctx, (LiteralUint8*)index);
  auto* set = builder.Create<SetContextSlotMovInst>(
      0, ctx, (LiteralUint8*)index, get);  // Redundant store
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_set = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == set) found_set = true;
  }
  EXPECT_FALSE(found_set)
      << "Redundant SetContextSlotMovInst should be removed";
}

TEST_F(LEPUSIRTestLSERedundantStore, RedundantUpvalueStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* index = builder.GetLiteralUint8(1);
  auto* get = builder.Create<GetUpvalueInst>(0, func, (LiteralUint8*)index);
  auto* set = builder.Create<SetUpvalueInst>(0, func, (LiteralUint8*)index,
                                             get);  // Redundant store
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_set = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == set) found_set = true;
  }
  EXPECT_FALSE(found_set) << "Redundant SetUpvalueInst should be removed";
}

TEST_F(LEPUSIRTestLSERedundantStore, RedundantTableStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test_table";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val = builder.GetLiteralInt32(100);

  builder.Create<SetTableInst>(0, obj, key, val);
  auto* redundant_set =
      builder.Create<SetTableInst>(0, obj, key, val);  // Redundant
  builder.Create<ReturnInst>(0, obj);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_set = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == redundant_set) found_set = true;
  }
  EXPECT_FALSE(found_set) << "Redundant SetTableInst should be removed";
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
