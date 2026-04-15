// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/transformer/mir/cse.h"
#include "core/runtime/lepus/ir/transformer/mir/dce.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestEnhancedOpts : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    lepus_func = lepus::Function::Create();
  }
  virtual void TearDown(void) {}

  fml::RefPtr<lepus::Function> lepus_func;
};

TEST_F(LEPUSIRTestEnhancedOpts, DCECreateClosure) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  std::string child_name = "child";
  auto* child_func = builder.Create<FuncOp>(0, child_name);
  auto child_lepus_func = lepus::Function::Create();
  child_func->Init(child_lepus_func);
  // Add a block to child_func
  auto* child_entry = builder.CreateBlock(child_func->GetSingleRegion(),
                                          BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(child_entry);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  // Back to parent func
  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Create an unused closure
  auto* closure = builder.Create<CreateClosureInst>(0, 0);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  DCE pass(ir_ctx.get());
  pass.RunOnModule(mod);

  bool found_closure = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == closure) found_closure = true;
  }
  EXPECT_FALSE(found_closure);
}

TEST_F(LEPUSIRTestEnhancedOpts, CSELoadConstAndBuiltin) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Redundant LoadConst
  auto* const_val = builder.GetLiteralInt32(42);
  auto* lc1 = builder.Create<LoadConstInst>(0, const_val,
                                            TypeOp::CreateInt32(&builder));
  auto* lc2 = builder.Create<LoadConstInst>(0, const_val,
                                            TypeOp::CreateInt32(&builder));

  // Redundant GetBuiltin
  uint32_t math_idx = lepus_func->AddConstString(constants::kGlobalMath);
  auto* math_lit_idx = builder.GetLiteralUint32(math_idx);
  auto* gb1 = builder.Create<GetBuiltinInst>(0, math_lit_idx,
                                             TypeOp::CreateAnyType(&builder));
  auto* gb2 = builder.Create<GetBuiltinInst>(0, math_lit_idx,
                                             TypeOp::CreateAnyType(&builder));

  // Use them to avoid simple DCE
  auto* add1 = builder.Create<BinaryOperatorInst>(
      0, lc1, lc2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(&builder));
  auto* add2 = builder.Create<BinaryOperatorInst>(
      0, gb1, gb2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(&builder));
  // Return sum of both to keep everything alive
  auto* final_val = builder.Create<BinaryOperatorInst>(
      0, add1, add2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, final_val);

  CSE pass(ir_ctx.get());
  pass.RunOnFunction(func);

  int lc_count = 0;
  int gb_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadConstInst>(inst)) lc_count++;
    if (llvh::isa<GetBuiltinInst>(inst)) gb_count++;
  }

  EXPECT_EQ(lc_count, 1);
  EXPECT_EQ(gb_count, 1);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
