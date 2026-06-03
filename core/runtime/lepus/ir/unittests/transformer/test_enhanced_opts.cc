// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/transformer/mir/cse.h"
#include "core/runtime/lepus/ir/transformer/mir/dce.h"
#include "core/runtime/lepus/ir/transformer/mir/load_null_elimination.h"
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

TEST_F(LEPUSIRTestEnhancedOpts, LoadNullEliminationHoistToDominator) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  // CFG:
  //   entry -> dom -> {b, c} -> join
  // Both b/c have redundant LoadNullOrUndefinedInst(type=1). The pass should
  // merge them into the dominator block, not the entry block.
  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* dom =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* b =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* c =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  builder.Create<BranchInst>(0, dom);

  builder.SetInsertionPointToStart(dom);
  auto* cond = builder.GetLiteralBool(true);
  builder.Create<CondBranchInst>(0, cond, b, c);

  builder.SetInsertionPointToStart(b);
  auto* t1 = builder.GetLiteralInt8(1);
  auto* ln1 = builder.Create<LoadNullOrUndefinedInst>(0, t1);
  // Add a non-terminator use so b is not a sole-LoadNull trampoline block.
  builder.Create<BinaryOperatorInst>(0, ln1, builder.GetLiteralInt32(0),
                                     ValueKind::BinaryAddInstKind,
                                     TypeOp::CreateAnyType(&builder));
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(c);
  auto* t2 = builder.GetLiteralInt8(1);
  auto* ln2 = builder.Create<LoadNullOrUndefinedInst>(0, t2);
  // Add a non-terminator use so c is not a sole-LoadNull trampoline block.
  builder.Create<BinaryOperatorInst>(0, ln2, builder.GetLiteralInt32(0),
                                     ValueKind::BinaryAddInstKind,
                                     TypeOp::CreateAnyType(&builder));
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(join);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  LoadNullEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  int type1_total = 0;
  int type1_in_entry = 0;
  int type1_in_dom = 0;
  for (auto& bb : *func) {
    for (auto* inst : bb.InstRange()) {
      auto* load = llvh::dyn_cast<LoadNullOrUndefinedInst>(inst);
      if (!load) continue;
      if (load->GetLoadNilType()->GetValue() != 1) continue;
      type1_total++;
      if (&bb == entry) type1_in_entry++;
      if (&bb == dom) type1_in_dom++;
    }
  }

  EXPECT_EQ(type1_total, 1);
  EXPECT_EQ(type1_in_entry, 0);
  EXPECT_EQ(type1_in_dom, 1);
}

TEST_F(LEPUSIRTestEnhancedOpts, LoadNullEliminationRespectsFirstInBlockPrefix) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_load_null_respects_first_in_block";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  // Build a minimal diamond so we can place a PhiInst (FirstInBlock) in join.
  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* b =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* c =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), b, c);

  builder.SetInsertionPointToStart(b);
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(c);
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(join);
  // PhiInst is marked FirstInBlock; LoadNullElimination must not insert before
  // it.
  auto* phi = builder.Create<PhiInst>(
      0,
      PhiInst::ValueListType{builder.GetLiteralInt32(1),
                             builder.GetLiteralInt32(2)},
      PhiInst::BlockListType{b, c});
  (void)phi;

  auto* t = builder.GetLiteralInt8(1);
  auto* n1 = builder.Create<LoadNullOrUndefinedInst>(0, t);
  auto* n2 = builder.Create<LoadNullOrUndefinedInst>(0, t);
  // Keep both loads alive pre-pass so the pass has something to rewrite.
  auto* keep = builder.Create<BinaryOperatorInst>(
      0, n1, n2, ValueKind::BinaryAddInstKind, TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, keep);

  LoadNullEliminationPass pass(ir_ctx.get());
  ASSERT_TRUE(pass.RunOnFunction(func));

  // Verify: only one load remains, and it is inserted AFTER the phi.
  Instruction* first_non_phi = nullptr;
  int nil_count = 0;
  for (auto* inst : join->InstRange()) {
    if (llvh::isa<PhiInst>(inst)) continue;
    if (!first_non_phi) first_non_phi = inst;
    if (llvh::isa<LoadNullOrUndefinedInst>(inst)) nil_count++;
  }
  ASSERT_NE(first_non_phi, nullptr);
  EXPECT_TRUE(llvh::isa<LoadNullOrUndefinedInst>(first_non_phi));
  EXPECT_EQ(nil_count, 1);
}

TEST_F(LEPUSIRTestEnhancedOpts, LoadNullEliminationIsNoOpWithSingleLoad) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_load_null_single_noop";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Single LoadNullOrUndefined - nothing to eliminate.
  auto* t = builder.GetLiteralInt8(0);
  auto* load = builder.Create<LoadNullOrUndefinedInst>(0, t);
  builder.Create<ReturnInst>(0, load);

  LoadNullEliminationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnFunction(func));

  // The single load should remain in place.
  int nil_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadNullOrUndefinedInst>(inst)) nil_count++;
  }
  EXPECT_EQ(nil_count, 1);
}

TEST_F(LEPUSIRTestEnhancedOpts, LoadNullEliminationDeduplicatesSameBlockLoads) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_load_null_same_block_deduplicate";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Three loads of the same type in the same block should be merged into one.
  auto* t = builder.GetLiteralInt8(0);
  auto* n1 = builder.Create<LoadNullOrUndefinedInst>(0, t);
  auto* n2 = builder.Create<LoadNullOrUndefinedInst>(0, t);
  auto* n3 = builder.Create<LoadNullOrUndefinedInst>(0, t);
  // Keep all alive.
  auto* sum = builder.Create<BinaryOperatorInst>(
      0, n1, n2, ValueKind::BinaryAddInstKind, TypeOp::CreateAnyType(&builder));
  auto* sum2 = builder.Create<BinaryOperatorInst>(
      0, sum, n3, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, sum2);

  LoadNullEliminationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnFunction(func));

  // Only one LoadNullOrUndefinedInst should remain.
  int nil_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadNullOrUndefinedInst>(inst)) nil_count++;
  }
  EXPECT_EQ(nil_count, 1);
}

TEST_F(LEPUSIRTestEnhancedOpts,
       LoadNullEliminationHandlesNullAndUndefinedIndependently) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_load_null_vs_undefined";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Two type-0 (null) loads and two type-1 (undefined) loads.
  auto* type_null = builder.GetLiteralInt8(0);
  auto* type_undef = builder.GetLiteralInt8(1);
  auto* null1 = builder.Create<LoadNullOrUndefinedInst>(0, type_null);
  auto* null2 = builder.Create<LoadNullOrUndefinedInst>(0, type_null);
  auto* undef1 = builder.Create<LoadNullOrUndefinedInst>(0, type_undef);
  auto* undef2 = builder.Create<LoadNullOrUndefinedInst>(0, type_undef);

  // Keep all alive.
  auto* sum_null = builder.Create<BinaryOperatorInst>(
      0, null1, null2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(&builder));
  auto* sum_undef = builder.Create<BinaryOperatorInst>(
      0, undef1, undef2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(&builder));
  auto* final_val = builder.Create<BinaryOperatorInst>(
      0, sum_null, sum_undef, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, final_val);

  LoadNullEliminationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnFunction(func));

  // Each type should have exactly one instance remaining.
  int type0_count = 0;
  int type1_count = 0;
  for (auto* inst : entry->InstRange()) {
    auto* load = llvh::dyn_cast<LoadNullOrUndefinedInst>(inst);
    if (!load) continue;
    if (load->GetLoadNilType()->GetValue() == 0) type0_count++;
    if (load->GetLoadNilType()->GetValue() == 1) type1_count++;
  }
  EXPECT_EQ(type0_count, 1);
  EXPECT_EQ(type1_count, 1);
}

TEST_F(LEPUSIRTestEnhancedOpts, LoadNullEliminationPreservesTrampolineBlocks) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_load_null_preserves_trampolines";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  // CFG: entry -> {nil_bb, get_bb} -> merge
  // nil_bb is a trampoline (sole LoadNull + Branch). The pass must NOT merge
  // its LoadNull away, because later passes rely on the block structure.
  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* nil_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* get_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* merge =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond = builder.GetLiteralBool(true);
  builder.Create<CondBranchInst>(0, cond, nil_bb, get_bb);

  // nil_bb: sole LoadNull + Branch (trampoline pattern).
  builder.SetInsertionPointToStart(nil_bb);
  auto* t = builder.GetLiteralInt8(1);
  auto* nil_load = builder.Create<LoadNullOrUndefinedInst>(0, t);
  builder.Create<BranchInst>(0, merge);

  // get_bb: has other instructions besides LoadNull.
  builder.SetInsertionPointToStart(get_bb);
  auto* ln_other = builder.Create<LoadNullOrUndefinedInst>(0, t);
  builder.Create<BinaryOperatorInst>(0, ln_other, builder.GetLiteralInt32(0),
                                     ValueKind::BinaryAddInstKind,
                                     TypeOp::CreateAnyType(&builder));
  builder.Create<BranchInst>(0, merge);

  // merge: Phi uses nil_load from nil_bb (mirrors real optional-chaining CFG).
  builder.SetInsertionPointToStart(merge);
  PhiInst::ValueListType phi_vals{nil_load, ln_other};
  PhiInst::BlockListType phi_blocks{nil_bb, get_bb};
  builder.Create<PhiInst>(0, phi_vals, phi_blocks);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  LoadNullEliminationPass pass(ir_ctx.get());
  // The pass should NOT merge (only one eligible load in get_bb; the
  // trampoline load in nil_bb is skipped).
  EXPECT_FALSE(pass.RunOnFunction(func));

  // Verify the trampoline's LoadNull is still in nil_bb.
  int nil_bb_count = 0;
  for (auto* inst : nil_bb->InstRange()) {
    if (llvh::isa<LoadNullOrUndefinedInst>(inst)) nil_bb_count++;
  }
  EXPECT_EQ(nil_bb_count, 1);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
