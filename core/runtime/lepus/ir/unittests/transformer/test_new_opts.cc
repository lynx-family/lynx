// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/dce.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/simplify_cfg.h"
#include "core/runtime/lepus/ir/transformer/mir/type_specification.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

// ============================================================================
// Test fixture for SimplifyCFG optimizations (PhiNot threading, correlated
// branches).
// ============================================================================
class LEPUSIRTestNewCFGOpts : public IRTestBase {
 public:
  virtual void SetUp(void) { IRTestBase::SetUp(); }
  virtual void TearDown(void) {}
};

// ============================================================================
// Test fixture for LSE dead store elimination and never-written upvalue
// preservation.
// ============================================================================
class LEPUSIRTestNewLSEOpts : public ::testing::Test {
 protected:
  void SetUp() override {
    vm_ctx_ = std::make_unique<lepus::VMContext>();
    vm_ctx_->Initialize();
    ir_ctx_ = std::make_unique<IRContext>(vm_ctx_.get());
    mod_ = ir_ctx_->GetMainMod();
  }

  void TearDown() override {
    ir_ctx_.reset();
    vm_ctx_.reset();
    mod_ = nullptr;
  }

  std::unique_ptr<lepus::VMContext> vm_ctx_;
  std::unique_ptr<IRContext> ir_ctx_;
  ModuleOp* mod_ = nullptr;
};

// ============================================================================
// OptPhiNotCondBranchJumpThreading tests
// ============================================================================

TEST_F(LEPUSIRTestNewCFGOpts, PhiNotJumpThreading_NullIncoming) {
  // Pattern:
  //   null_bb: Branch(mid)
  //   call_bb: Branch(mid)
  //   mid: %phi = Phi(%null from null_bb, %val from call_bb)
  //        %not = Not(%phi)
  //        CondBranch(%not, create_bb, use_bb)
  // Expected: null_bb threads directly to create_bb (Not(null) = true).
  //           call_bb threads directly to use_bb (Not(truthy) = false).
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_phi_not_threading";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* null_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* call_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* mid =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* create_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* use_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry: CondBranch to null_bb or call_bb
  builder.SetInsertionPointToStart(entry);
  auto* param = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, param, call_bb, null_bb);

  // null_bb: Branch mid
  builder.SetInsertionPointToStart(null_bb);
  auto* null_val =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, mid);

  // call_bb: some non-null value, Branch mid
  builder.SetInsertionPointToStart(call_bb);
  auto* call_val = builder.Create<NewTableInst>(0);
  builder.Create<BranchInst>(0, mid);

  // mid: phi + Not + CondBranch
  builder.SetInsertionPointToStart(mid);
  PhiInst::ValueListType vals = {null_val, call_val};
  PhiInst::BlockListType blks = {null_bb, call_bb};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  auto* not_inst =
      builder.Create<UnaryOperatorInst>(0, phi, ValueKind::UnaryNotInstKind);
  builder.Create<CondBranchInst>(0, not_inst, create_bb, use_bb);

  // create_bb: Return 1
  builder.SetInsertionPointToStart(create_bb);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  // use_bb: Return 0
  builder.SetInsertionPointToStart(use_bb);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  const int before_blocks = static_cast<int>(func->GetBlockSize());
  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // After threading both edges and block merging:
  //   null_bb → create_bb (Not(null)=true), then create_bb slurped into null_bb
  //   call_bb → use_bb (Not(truthy)=false), then use_bb slurped into call_bb
  //   mid is erased (unreachable)
  // Final: entry(CondBranch) → null_bb(Ret 1) / call_bb(Ret 0)
  EXPECT_LT(static_cast<int>(func->GetBlockSize()), before_blocks);

  // entry should still be a CondBranch.
  auto* entry_term = llvh::dyn_cast<CondBranchInst>(entry->GetTerminator());
  ASSERT_NE(entry_term, nullptr);

  // The null path (false edge) should lead to a block returning 1 (create_bb's
  // content), and the truthy path (true edge) should lead to a block returning
  // 0 (use_bb's content). The exact blocks may have been merged.
  Block* false_dest = entry_term->GetFalseDest();
  Block* true_dest = entry_term->GetTrueDest();

  // false_dest (was null path) should return 1.
  auto* false_ret = llvh::dyn_cast<ReturnInst>(false_dest->GetTerminator());
  ASSERT_NE(false_ret, nullptr);
  auto* false_val = llvh::dyn_cast<LiteralInt32>(false_ret->GetValue());
  ASSERT_NE(false_val, nullptr);
  EXPECT_EQ(false_val->GetValue(), 1);

  // true_dest (was call/truthy path) should return 0.
  auto* true_ret = llvh::dyn_cast<ReturnInst>(true_dest->GetTerminator());
  ASSERT_NE(true_ret, nullptr);
  auto* true_val = llvh::dyn_cast<LiteralInt32>(true_ret->GetValue());
  ASSERT_NE(true_val, nullptr);
  EXPECT_EQ(true_val->GetValue(), 0);
}

TEST_F(LEPUSIRTestNewCFGOpts, PhiNotJumpThreading_UnknownNotThreaded) {
  // If the phi incoming is an unknown value (e.g. parameter), we can't
  // statically determine Not(unknown) → threading should NOT occur.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_phi_not_no_threading";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_a =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_b =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* mid =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* t =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* f =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* param0 = func->CreateParam(0);
  auto* param1 = func->CreateParam(1);
  builder.Create<CondBranchInst>(0, param0, pred_a, pred_b);

  // Both predecessors provide unknown values (parameters).
  builder.SetInsertionPointToStart(pred_a);
  builder.Create<BranchInst>(0, mid);

  builder.SetInsertionPointToStart(pred_b);
  builder.Create<BranchInst>(0, mid);

  builder.SetInsertionPointToStart(mid);
  PhiInst::ValueListType vals = {param0, param1};
  PhiInst::BlockListType blks = {pred_a, pred_b};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  auto* not_inst =
      builder.Create<UnaryOperatorInst>(0, phi, ValueKind::UnaryNotInstKind);
  builder.Create<CondBranchInst>(0, not_inst, t, f);

  builder.SetInsertionPointToStart(t);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));
  builder.SetInsertionPointToStart(f);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // No threading should happen — block count unchanged for mid.
  // mid should still exist with its CondBranch.
  bool mid_exists = false;
  for (auto& b : *func) {
    if (&b == mid) {
      mid_exists = true;
      break;
    }
  }
  EXPECT_TRUE(mid_exists);
  EXPECT_NE(llvh::dyn_cast<CondBranchInst>(mid->GetTerminator()), nullptr);
}

TEST_F(LEPUSIRTestNewCFGOpts, PhiNotJumpThreading_BailOnExtraInstructions) {
  // If mid has instructions other than phi(s) + Not + CondBranch, pass should
  // bail out. Use two predecessors to avoid single-entry phi elimination.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_phi_not_bail_extra_inst";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_a =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_b =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* mid =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* t =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* f =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* param = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, param, pred_a, pred_b);

  builder.SetInsertionPointToStart(pred_a);
  auto* null_val =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, mid);

  builder.SetInsertionPointToStart(pred_b);
  auto* val_b = builder.Create<NewTableInst>(0);
  builder.Create<BranchInst>(0, mid);

  builder.SetInsertionPointToStart(mid);
  PhiInst::ValueListType vals = {null_val, val_b};
  PhiInst::BlockListType blks = {pred_a, pred_b};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  // Extra instruction between phi and Not should prevent threading.
  auto* extra = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                              TypeOp::CreateInt32(&builder));
  auto* not_inst =
      builder.Create<UnaryOperatorInst>(0, phi, ValueKind::UnaryNotInstKind);
  builder.Create<CondBranchInst>(0, not_inst, t, f);

  builder.SetInsertionPointToStart(t);
  builder.Create<ReturnInst>(0, extra);
  builder.SetInsertionPointToStart(f);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // The PhiNot threading should NOT fire because of the extra LoadConstInst.
  // mid should still exist with a CondBranch.
  bool mid_exists = false;
  for (auto& b : *func) {
    if (&b == mid) {
      mid_exists = true;
      break;
    }
  }
  EXPECT_TRUE(mid_exists);
  if (mid_exists) {
    auto* term = llvh::dyn_cast<CondBranchInst>(mid->GetTerminator());
    EXPECT_NE(term, nullptr);
  }
}

TEST_F(LEPUSIRTestNewCFGOpts, PhiNotJumpThreading_BailOnExternalPhiUser) {
  // If the phi in mid has a non-phi external user (e.g. used by a ReturnInst
  // in a successor block), threading must bail to preserve SSA dominance.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_phi_not_bail_ssa_dominance";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_a =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_b =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* mid =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry: CondBranch to pred_a or pred_b
  builder.SetInsertionPointToStart(entry);
  auto* param = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, param, pred_a, pred_b);

  // pred_a: null value → Branch mid
  builder.SetInsertionPointToStart(pred_a);
  auto* null_val =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, mid);

  // pred_b: truthy value → Branch mid
  builder.SetInsertionPointToStart(pred_b);
  auto* obj_val = builder.Create<NewTableInst>(0);
  builder.Create<BranchInst>(0, mid);

  // mid: phi + Not + CondBranch
  builder.SetInsertionPointToStart(mid);
  PhiInst::ValueListType vals = {null_val, obj_val};
  PhiInst::BlockListType blks = {pred_a, pred_b};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  auto* not_inst =
      builder.Create<UnaryOperatorInst>(0, phi, ValueKind::UnaryNotInstKind);
  builder.Create<CondBranchInst>(0, not_inst, true_bb, false_bb);

  // true_bb: Return %phi — this is the non-phi external user of phi
  builder.SetInsertionPointToStart(true_bb);
  builder.Create<ReturnInst>(0, phi);

  // false_bb: Return 0
  builder.SetInsertionPointToStart(false_bb);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // Threading should NOT occur because phi has a non-phi external user in
  // true_bb. mid must still exist with its CondBranch terminator.
  bool mid_exists = false;
  for (auto& b : *func) {
    if (&b == mid) {
      mid_exists = true;
      break;
    }
  }
  EXPECT_TRUE(mid_exists);
  if (mid_exists) {
    auto* term = llvh::dyn_cast<CondBranchInst>(mid->GetTerminator());
    EXPECT_NE(term, nullptr);
  }
}

// ============================================================================
// OptCorrelatedBranches tests
// ============================================================================

TEST_F(LEPUSIRTestNewCFGOpts, CorrelatedBranches_TrueEdgeFold) {
  // Pattern:
  //   entry: CondBranch(%cond, true_path, false_succ)
  //   true_path: CondBranch(%other, inner, join)
  //   inner: CondBranch(%cond, T, F)   ← should be folded to Branch(T)
  // inner is dominated by true_path which is dominated by entry's true edge.
  // Since %cond is known true there, the inner CondBranch should fold.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_correlated_true";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_path =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_succ =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* inner =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* t =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* f =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  auto* other_cond = func->CreateParam(1);
  builder.Create<CondBranchInst>(0, cond, true_path, false_succ);

  // true_path: branch on a different condition to prevent merge.
  builder.SetInsertionPointToStart(true_path);
  builder.Create<CondBranchInst>(0, other_cond, inner, join);

  // inner: redundant CondBranch on same %cond (dominated by entry's true edge).
  builder.SetInsertionPointToStart(inner);
  builder.Create<CondBranchInst>(0, cond, t, f);

  builder.SetInsertionPointToStart(false_succ);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(2));

  builder.SetInsertionPointToStart(join);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(3));

  builder.SetInsertionPointToStart(t);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  builder.SetInsertionPointToStart(f);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // After correlated branch elimination, no CondBranch on %cond should target
  // both t and f from the true path. The inner CondBranch should have been
  // folded (its false edge to f eliminated).
  bool inner_cond_branch_exists = false;
  for (auto& b : *func) {
    auto* cbr = llvh::dyn_cast<CondBranchInst>(b.GetTerminator());
    if (!cbr) continue;
    // Look for a CondBranch that tests %cond and has f as false target.
    if (cbr->GetCondition() == cond && cbr->GetFalseDest() == f) {
      inner_cond_branch_exists = true;
    }
  }
  EXPECT_FALSE(inner_cond_branch_exists)
      << "Correlated branch should have been folded (cond known true)";
}

TEST_F(LEPUSIRTestNewCFGOpts, CorrelatedBranches_FalseEdgeFold) {
  // Pattern:
  //   entry: CondBranch(%cond, true_succ, false_path)
  //   false_path: CondBranch(%other, inner, join)
  //   inner: CondBranch(%cond, T, F)   ← should be folded to Branch(F)
  // inner is dominated by false_path which is entry's false target.
  // Since %cond is known false there, the inner CondBranch should fold.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_correlated_false";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_succ =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_path =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* inner =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* t =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* f =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  auto* other_cond = func->CreateParam(1);
  builder.Create<CondBranchInst>(0, cond, true_succ, false_path);

  builder.SetInsertionPointToStart(true_succ);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(2));

  // false_path: branch on a different condition to prevent merge.
  builder.SetInsertionPointToStart(false_path);
  builder.Create<CondBranchInst>(0, other_cond, inner, join);

  // inner: redundant CondBranch on same %cond (dominated by entry's false
  // edge).
  builder.SetInsertionPointToStart(inner);
  builder.Create<CondBranchInst>(0, cond, t, f);

  builder.SetInsertionPointToStart(join);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(3));

  builder.SetInsertionPointToStart(t);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));
  builder.SetInsertionPointToStart(f);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // After correlated branch elimination, no CondBranch on %cond should target
  // both t and f from the false path. The inner CondBranch should have been
  // folded (its true edge to t eliminated).
  bool inner_cond_branch_exists = false;
  for (auto& b : *func) {
    auto* cbr = llvh::dyn_cast<CondBranchInst>(b.GetTerminator());
    if (!cbr) continue;
    // Look for a CondBranch that tests %cond and has t as true target.
    if (cbr->GetCondition() == cond && cbr->GetTrueDest() == t) {
      inner_cond_branch_exists = true;
    }
  }
  EXPECT_FALSE(inner_cond_branch_exists)
      << "Correlated branch should have been folded (cond known false)";
}

TEST_F(LEPUSIRTestNewCFGOpts, CorrelatedBranches_DifferentCondNotFolded) {
  // If the inner CondBranch tests a DIFFERENT condition, no folding occurs.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_correlated_different_cond";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_succ =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_succ =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* t =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* f =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond1 = func->CreateParam(0);
  auto* cond2 = func->CreateParam(1);
  builder.Create<CondBranchInst>(0, cond1, true_succ, false_succ);

  // true_succ: branches on cond2 (different from cond1) — should NOT fold.
  builder.SetInsertionPointToStart(true_succ);
  builder.Create<CondBranchInst>(0, cond2, t, f);

  builder.SetInsertionPointToStart(false_succ);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(2));

  builder.SetInsertionPointToStart(t);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));
  builder.SetInsertionPointToStart(f);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // true_succ should still have a CondBranch (not folded).
  auto* term = llvh::dyn_cast<CondBranchInst>(true_succ->GetTerminator());
  EXPECT_NE(term, nullptr)
      << "Different condition should not trigger correlated branch folding";
}

// ============================================================================
// Intra-block Dead Store Elimination tests
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_OverwrittenTableStore) {
  // Pattern: SetTable(obj, key, val1); SetTable(obj, key, val2)
  // First store is dead (never read between stores).
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableInst>(0, obj, key, val1);
  auto* store2 = builder.Create<SetTableInst>(0, obj, key, val2);
  builder.Create<ReturnInst>(0, obj);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // store1 should be eliminated (dead store), store2 should remain.
  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_FALSE(found_store1) << "First store should be eliminated (dead store)";
  EXPECT_TRUE(found_store2) << "Second store should remain";
}

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_NotEliminatedWhenRead) {
  // Pattern: SetTable(obj, key, val1); GetTable(obj, key); SetTable(obj, key,
  // val2)
  // First store is NOT dead because there's a read in between.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_with_read";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableInst>(0, obj, key, val1);
  auto* load = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  auto* store2 = builder.Create<SetTableInst>(0, obj, key, val2);
  builder.Create<ReturnInst>(0, load);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // store1 should NOT be eliminated because there's a read before store2.
  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_TRUE(found_store1) << "First store should remain (read in between)";
  EXPECT_TRUE(found_store2) << "Second store should remain";
}

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_ClearedByReadonlyCall) {
  // Pattern: SetTable(obj, key, val1); ReadonlyCall(); SetTable(obj, key, val2)
  // First store is NOT dead because readonly call may have read it.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_readonly_call";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableInst>(0, obj, key, val1);

  // Create a call that will be readonly.
  // Use parseFloat as the readonly builtin.
  int parse_float_idx =
      vm_ctx_->builtin()->Search(base::String(constants::kParseFloat));
  if (parse_float_idx >= 0) {
    auto* builtin_idx_lit =
        builder.GetLiteralUint32(static_cast<uint32_t>(parse_float_idx));
    auto* get_builtin = builder.Create<GetBuiltinInst>(
        0, (Literal*)builtin_idx_lit, TypeOp::CreateAnyType(&builder));
    ArgList args;
    builder.Create<CallInst>(0, get_builtin, args);
  }

  auto* store2 = builder.Create<SetTableInst>(0, obj, key, val2);
  builder.Create<ReturnInst>(0, obj);

  // Run TypeSpecification to mark the builtin call as readonly.
  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // store1 should NOT be eliminated because readonly call clears
  // pending_stores.
  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_TRUE(found_store1)
      << "First store should remain (readonly call may have read it)";
  EXPECT_TRUE(found_store2) << "Second store should remain";
}

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_ConstStringKey) {
  // Test dead store elimination with SetTableConstStringKeyInst.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_const_key";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  lepus_func->AddConstString("x");  // index 0
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* const_idx = builder.GetLiteralUint32(0);  // "x"
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableConstStringKeyInst>(
      0, obj, (Literal*)const_idx, val1);
  auto* store2 = builder.Create<SetTableConstStringKeyInst>(
      0, obj, (Literal*)const_idx, val2);
  builder.Create<ReturnInst>(0, obj);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_FALSE(found_store1) << "First const-key store should be eliminated";
  EXPECT_TRUE(found_store2) << "Second const-key store should remain";
}

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_DifferentKeysNotEliminated) {
  // Stores to different keys should not be eliminated.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_different_keys";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key1 = builder.GetLiteralInt32(1);
  auto* key2 = builder.GetLiteralInt32(2);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableInst>(0, obj, key1, val1);
  auto* store2 = builder.Create<SetTableInst>(0, obj, key2, val2);
  builder.Create<ReturnInst>(0, obj);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_TRUE(found_store1) << "Store to key1 should remain";
  EXPECT_TRUE(found_store2) << "Store to key2 should remain";
}

// ============================================================================
// Never-written upvalue preservation tests
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, NeverWrittenUpvalue_PreservedAcrossCall) {
  // If an upvalue is never written (no SetUpvalueInst) anywhere in the module,
  // its cached value should survive across non-readonly calls.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Create a function that reads an upvalue, makes a call, then reads again.
  std::string name = "test_never_written_upvalue";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  // Add an upvalue named "readOnlyVar"
  lepus_func->AddUpvalue("readOnlyVar", false, 0);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* index = builder.GetLiteralUint8(0);
  auto* get1 = builder.Create<GetUpvalueInst>(0, func, (LiteralUint8*)index);

  // Non-readonly call (unknown callee).
  ArgList args;
  auto* callee = builder.GetLiteralInt32(999);
  builder.Create<CallInst>(0, callee, args);

  // Second read of same upvalue.
  auto* get2 = builder.Create<GetUpvalueInst>(0, func, (LiteralUint8*)index);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // Since "readOnlyVar" is never written anywhere in the module, get2 should
  // be forwarded to get1 (i.e., get2 removed).
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First upvalue read should remain";
  EXPECT_FALSE(found_get2)
      << "Second upvalue read should be eliminated (never-written upvalue "
         "preserved across call)";
}

TEST_F(LEPUSIRTestNewLSEOpts, WrittenUpvalue_NotPreservedAcrossCall) {
  // If an upvalue IS written somewhere, its cache must be invalidated at calls.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Create two functions: one that writes the upvalue, one that reads it.
  std::string writer_name = "writer";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_lepus->AddUpvalue("sharedVar", false, 0);
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);
  auto* w_index = builder.GetLiteralUint8(0);
  auto* w_val = builder.GetLiteralInt32(42);
  builder.Create<SetUpvalueInst>(0, writer_func, (LiteralUint8*)w_index, w_val);
  builder.Create<ReturnInst>(0, w_val);

  // Now the reader function.
  std::string reader_name = "reader";
  auto* reader_func = builder.Create<FuncOp>(0, reader_name);
  auto reader_lepus = lepus::Function::Create();
  reader_lepus->AddUpvalue("sharedVar", false, 0);
  reader_func->Init(reader_lepus);
  auto* reader_entry = builder.CreateBlock(reader_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(reader_entry);

  auto* r_index = builder.GetLiteralUint8(0);
  auto* get1 =
      builder.Create<GetUpvalueInst>(0, reader_func, (LiteralUint8*)r_index);

  // Non-readonly call.
  ArgList r_args;
  auto* r_callee = builder.GetLiteralInt32(999);
  builder.Create<CallInst>(0, r_callee, r_args);

  auto* get2 =
      builder.Create<GetUpvalueInst>(0, reader_func, (LiteralUint8*)r_index);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(reader_func);

  // Since "sharedVar" is written in writer_func, the cache is invalidated.
  // get2 should NOT be forwarded to get1.
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : reader_entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First upvalue read should remain";
  EXPECT_TRUE(found_get2)
      << "Second upvalue read should remain (written upvalue, cache "
         "invalidated by call)";
}

TEST_F(LEPUSIRTestNewLSEOpts,
       NeverWrittenUpvalue_SelectivePreservationAcrossLocalTableSafeCall) {
  // Verify SELECTIVE upvalue preservation across LocalTableSafeCalls:
  // - upvalue "writtenVar" (index 0): written in another function → cache
  //   should be invalidated across the call.
  // - upvalue "readOnlyVar" (index 1): never written anywhere → cache
  //   should survive across the call.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Writer function: writes to upvalue "writtenVar".
  std::string writer_name = "writer_func";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_lepus->AddUpvalue("writtenVar", false, 0);
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);
  auto* w_index = builder.GetLiteralUint8(0);
  auto* w_val = builder.GetLiteralInt32(42);
  builder.Create<SetUpvalueInst>(0, writer_func, (LiteralUint8*)w_index, w_val);
  builder.Create<ReturnInst>(0, w_val);

  // Reader function: captures both "writtenVar" (idx 0) and "readOnlyVar"
  // (idx 1), reads both before and after a LocalTableSafeCall.
  std::string reader_name = "reader_func";
  auto* reader_func = builder.Create<FuncOp>(0, reader_name);
  auto reader_lepus = lepus::Function::Create();
  reader_lepus->AddUpvalue("writtenVar", false, 0);
  reader_lepus->AddUpvalue("readOnlyVar", false, 1);
  reader_func->Init(reader_lepus);
  auto* entry = builder.CreateBlock(reader_func->GetSingleRegion(),
                                    BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* idx0 = builder.GetLiteralUint8(0);
  auto* idx1 = builder.GetLiteralUint8(1);

  // First reads.
  auto* get_written_1 =
      builder.Create<GetUpvalueInst>(0, reader_func, (LiteralUint8*)idx0);
  auto* get_readonly_1 =
      builder.Create<GetUpvalueInst>(0, reader_func, (LiteralUint8*)idx1);

  // LocalTableSafeCall (e.g., __SetAttribute).
  ArgList args;
  auto* callee = builder.GetLiteralInt32(999);
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  // Second reads of same upvalues.
  auto* get_written_2 =
      builder.Create<GetUpvalueInst>(0, reader_func, (LiteralUint8*)idx0);
  auto* get_readonly_2 =
      builder.Create<GetUpvalueInst>(0, reader_func, (LiteralUint8*)idx1);
  builder.Create<ReturnInst>(0, get_readonly_2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(reader_func);

  bool found_get_written_1 = false;
  bool found_get_written_2 = false;
  bool found_get_readonly_1 = false;
  bool found_get_readonly_2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_written_1) found_get_written_1 = true;
    if (inst == get_written_2) found_get_written_2 = true;
    if (inst == get_readonly_1) found_get_readonly_1 = true;
    if (inst == get_readonly_2) found_get_readonly_2 = true;
  }

  // "writtenVar" IS written in another function → cache invalidated by call.
  EXPECT_TRUE(found_get_written_1) << "First read of writtenVar should remain";
  EXPECT_TRUE(found_get_written_2)
      << "Second read of writtenVar should remain (cache invalidated because "
         "writtenVar is written in writer_func)";

  // "readOnlyVar" is NEVER written → cache preserved across call.
  EXPECT_TRUE(found_get_readonly_1)
      << "First read of readOnlyVar should remain";
  EXPECT_FALSE(found_get_readonly_2)
      << "Second read of readOnlyVar should be eliminated (never-written "
         "upvalue preserved across LocalTableSafeCall)";
}

// ============================================================================
// Additional DSE edge case tests
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_GenericCallInvalidates) {
  // Pattern: SetTable(obj, key, val1); GenericCall(); SetTable(obj, key, val2)
  // Generic (non-readonly) call clears pending_stores → first store NOT dead.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_generic_call";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableInst>(0, obj, key, val1);

  // Generic call with unknown callee.
  ArgList args;
  auto* callee = builder.GetLiteralInt32(999);
  builder.Create<CallInst>(0, callee, args);

  auto* store2 = builder.Create<SetTableInst>(0, obj, key, val2);
  builder.Create<ReturnInst>(0, obj);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // store1 should NOT be eliminated because generic call invalidates.
  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_TRUE(found_store1)
      << "First store should remain (generic call invalidates pending_stores)";
  EXPECT_TRUE(found_store2) << "Second store should remain";
}

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_ChainOfThreeStores) {
  // Pattern: SetTable(obj, key, val1); SetTable(obj, key, val2);
  //          SetTable(obj, key, val3)
  // First two stores are dead; only the last should remain.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_chain";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);
  auto* val3 = builder.GetLiteralInt32(300);

  auto* store1 = builder.Create<SetTableInst>(0, obj, key, val1);
  auto* store2 = builder.Create<SetTableInst>(0, obj, key, val2);
  auto* store3 = builder.Create<SetTableInst>(0, obj, key, val3);
  builder.Create<ReturnInst>(0, obj);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_store1 = false;
  bool found_store2 = false;
  bool found_store3 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
    if (inst == store3) found_store3 = true;
  }
  EXPECT_FALSE(found_store1) << "First store should be eliminated";
  EXPECT_FALSE(found_store2) << "Second store should be eliminated";
  EXPECT_TRUE(found_store3) << "Third (final) store should remain";
}

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_DifferentObjectsSameKeyNotEliminated) {
  // Stores to different objects with the same key should not interfere.
  // SetTable(obj1, key, val1); SetTable(obj2, key, val2)
  // Neither is dead.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_diff_obj";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj1 = builder.Create<NewTableInst>(0);
  auto* obj2 = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableInst>(0, obj1, key, val1);
  auto* store2 = builder.Create<SetTableInst>(0, obj2, key, val2);
  builder.Create<ReturnInst>(0, obj1);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_TRUE(found_store1) << "Store to obj1 should remain";
  EXPECT_TRUE(found_store2) << "Store to obj2 should remain";
}

// ============================================================================
// Additional PhiNot threading edge case
// ============================================================================

TEST_F(LEPUSIRTestNewCFGOpts, PhiNotJumpThreading_MultipleIncoming) {
  // Phi with 3 incoming values: two nulls and one non-null.
  // Both null predecessors should thread to create_bb.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_phi_not_multi";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* null_bb1 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* null_bb2 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* call_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* mid =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* create_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* use_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry: dispatch to three predecessors
  builder.SetInsertionPointToStart(entry);
  auto* param1 = func->CreateParam(0);
  auto* param2 = func->CreateParam(1);
  builder.Create<CondBranchInst>(0, param1, call_bb, null_bb1);

  builder.SetInsertionPointToStart(null_bb1);
  auto* null_val1 =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<CondBranchInst>(0, param2, null_bb2, mid);

  builder.SetInsertionPointToStart(null_bb2);
  auto* null_val2 =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, mid);

  builder.SetInsertionPointToStart(call_bb);
  auto* call_val = builder.Create<NewTableInst>(0);
  builder.Create<BranchInst>(0, mid);

  // mid: phi with 3 incoming
  builder.SetInsertionPointToStart(mid);
  PhiInst::ValueListType vals = {null_val1, null_val2, call_val};
  PhiInst::BlockListType blks = {null_bb1, null_bb2, call_bb};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  auto* not_inst =
      builder.Create<UnaryOperatorInst>(0, phi, ValueKind::UnaryNotInstKind);
  builder.Create<CondBranchInst>(0, not_inst, create_bb, use_bb);

  builder.SetInsertionPointToStart(create_bb);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));
  builder.SetInsertionPointToStart(use_bb);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // After threading, null_bb2 should go directly to create_bb.
  auto* null2_term = null_bb2->GetTerminator();
  if (auto* branch = llvh::dyn_cast<BranchInst>(null2_term)) {
    EXPECT_EQ(branch->GetBranchDest(), create_bb)
        << "null_bb2 should be threaded to create_bb";
  } else {
    FAIL() << "null_bb2 should have been threaded to create_bb";
  }
}

// ============================================================================
// Interprocedural readonly analysis test
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, ReadonlyCall_PreservesTableCache) {
  // Tests that LSE preserves table cache across calls marked as readonly.
  // This verifies the behavior that our interprocedural readonly analysis
  // enables: when a call is known not to write, table loads survive across it.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_readonly_call_cache";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  auto* key = builder.GetLiteralInt32(5);
  auto* get1 = builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Create a call and manually mark it as readonly (simulating what
  // ComputeReadonlyFuncNames + SetCallReadOnlyAttr would do).
  ArgList args;
  args.push_back(get1);
  auto* callee = builder.GetLiteralInt32(999);
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetReadonlyCall(true);

  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // Since the call is readonly, table cache should be preserved.
  // get2 should be eliminated (forwarded to get1).
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_FALSE(found_get2)
      << "Second table read should be eliminated (readonly call preserves "
         "cache)";
}

// ============================================================================
// CouldBeNumberString aliasing edge case tests (parametrized)
// ============================================================================
// These tests verify that the number-string aliasing refinement in
// KeysMayAlias works correctly for various edge cases. A string key that
// "could be" a number's ToString representation must conservatively alias
// with numeric keys, while non-numeric strings must not alias.

struct AliasingParam {
  const char* str_key;
  bool should_alias;  // true = second GetTable must survive (not eliminated)
};

class LEPUSIRTestLSEAliasing
    : public LEPUSIRTestNewLSEOpts,
      public ::testing::WithParamInterface<AliasingParam> {};

TEST_P(LEPUSIRTestLSEAliasing, NumericKeyAliasBehavior) {
  const auto& param = GetParam();
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = std::string("test_alias_") + param.str_key;
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t str_idx = lepus_func->AddConstString(param.str_key);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* str_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(str_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, str_key);

  auto* num_key = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(1), TypeOp::CreateInt32(&builder));
  builder.Create<SetTableInst>(0, obj, num_key, builder.GetLiteralInt32(1));

  auto* get2 = builder.Create<GetTableInst>(0, obj, str_key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  if (param.should_alias) {
    EXPECT_TRUE(found_get2)
        << "String key \"" << param.str_key
        << "\" could alias numeric keys; second load must NOT be eliminated";
  } else {
    EXPECT_FALSE(found_get2)
        << "String key \"" << param.str_key
        << "\" cannot alias numeric keys; second load should be eliminated";
  }
}

INSTANTIATE_TEST_SUITE_P(
    Aliasing, LEPUSIRTestLSEAliasing,
    ::testing::Values(AliasingParam{"42", true},         // numeric string
                      AliasingParam{"1e10", true},       // scientific notation
                      AliasingParam{"img_info", false},  // non-numeric string
                      AliasingParam{"NaN", true},        // NaN.toString()
                      AliasingParam{"Infinity", true},   // Infinity.toString()
                      AliasingParam{"-Infinity",
                                    true},      // (-Infinity).toString()
                      AliasingParam{"", false}  // empty string
                      ));

// ============================================================================
// DSE edge case: store with users should NOT be eliminated
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, DeadStore_NotEliminatedWhenStoreHasUsers) {
  // Pattern: SetTable(obj, key, val1); use(store1); SetTable(obj, key, val2)
  // If store1 has users (e.g., its result is returned), it cannot be removed.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_dead_store_with_users";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val1 = builder.GetLiteralInt32(100);
  auto* val2 = builder.GetLiteralInt32(200);

  auto* store1 = builder.Create<SetTableInst>(0, obj, key, val1);
  auto* store2 = builder.Create<SetTableInst>(0, obj, key, val2);
  // Return store1 to give it a user.
  builder.Create<ReturnInst>(0, store1);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // store1 has users (ReturnInst uses it), so DSE should NOT remove it.
  bool found_store1 = false;
  bool found_store2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
    if (inst == store2) found_store2 = true;
  }
  EXPECT_TRUE(found_store1)
      << "Store with users should NOT be eliminated by DSE";
  EXPECT_TRUE(found_store2) << "Second store should remain";
}

// ============================================================================
// Static cache ABA issue regression test
// ============================================================================
// This test exercises the scenario where multiple IRContexts are created
// sequentially (as happens across test cases), verifying that the
// never-written upvalue analysis doesn't use stale data.

TEST_F(LEPUSIRTestNewLSEOpts, NeverWrittenUpvalue_FreshAnalysisPerContext) {
  // First: create a module where upvalue "x" is NEVER written.
  // Verify it's preserved across calls.
  {
    OpBuilder builder;
    builder.SetModuleOp(mod_);
    builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

    std::string name = "test_fresh_analysis_1";
    auto* func = builder.Create<FuncOp>(0, name);
    auto lepus_func = lepus::Function::Create();
    lepus_func->AddUpvalue("x", false, 0);
    func->Init(lepus_func);
    auto* entry = builder.CreateBlock(func->GetSingleRegion(),
                                      BlockType::BT_INST, 0, "entry");
    builder.SetInsertionPointToEnd(entry);

    auto* index = builder.GetLiteralUint8(0);
    (void)builder.Create<GetUpvalueInst>(0, func, (LiteralUint8*)index);
    ArgList args;
    auto* callee = builder.GetLiteralInt32(999);
    builder.Create<CallInst>(0, callee, args);
    auto* get2 = builder.Create<GetUpvalueInst>(0, func, (LiteralUint8*)index);
    builder.Create<ReturnInst>(0, get2);

    LoadStoreElimination pass(ir_ctx_.get());
    pass.RunOnFunction(func);

    // "x" is never written → get2 should be eliminated.
    bool found_get2 = false;
    for (auto* inst : entry->InstRange()) {
      if (inst == get2) found_get2 = true;
    }
    EXPECT_FALSE(found_get2)
        << "Never-written upvalue should be preserved across call (first ctx)";
  }

  // Second: create a NEW context where upvalue "x" IS written somewhere.
  // Must NOT reuse stale analysis from the first context.
  auto vm_ctx2 = std::make_unique<lepus::VMContext>();
  vm_ctx2->Initialize();
  auto ir_ctx2 = std::make_unique<IRContext>(vm_ctx2.get());
  auto* mod2 = ir_ctx2->GetMainMod();

  {
    OpBuilder builder;
    builder.SetModuleOp(mod2);
    builder.SetInsertionPointToEnd(mod2->GetFunctionBlock());

    // Create a writer function that writes to upvalue "x".
    std::string writer_name = "writer_x";
    auto* writer_func = builder.Create<FuncOp>(0, writer_name);
    auto writer_lepus = lepus::Function::Create();
    writer_lepus->AddUpvalue("x", false, 0);
    writer_func->Init(writer_lepus);
    auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                             BlockType::BT_INST, 0, "entry");
    builder.SetInsertionPointToEnd(writer_entry);
    auto* w_index = builder.GetLiteralUint8(0);
    auto* w_val = builder.GetLiteralInt32(42);
    builder.Create<SetUpvalueInst>(0, writer_func, (LiteralUint8*)w_index,
                                   w_val);
    builder.Create<ReturnInst>(0, w_val);

    // Create a reader function that reads upvalue "x" across a call.
    std::string reader_name = "reader_x";
    auto* reader_func = builder.Create<FuncOp>(0, reader_name);
    auto reader_lepus = lepus::Function::Create();
    reader_lepus->AddUpvalue("x", false, 0);
    reader_func->Init(reader_lepus);
    auto* reader_entry = builder.CreateBlock(reader_func->GetSingleRegion(),
                                             BlockType::BT_INST, 0, "entry");
    builder.SetInsertionPointToEnd(reader_entry);

    auto* r_index = builder.GetLiteralUint8(0);
    (void)builder.Create<GetUpvalueInst>(0, reader_func,
                                         (LiteralUint8*)r_index);
    ArgList r_args;
    auto* r_callee = builder.GetLiteralInt32(999);
    builder.Create<CallInst>(0, r_callee, r_args);
    auto* get2 =
        builder.Create<GetUpvalueInst>(0, reader_func, (LiteralUint8*)r_index);
    builder.Create<ReturnInst>(0, get2);

    LoadStoreElimination pass(ir_ctx2.get());
    pass.RunOnFunction(reader_func);

    // "x" IS written in this module → get2 must NOT be eliminated.
    bool found_get2 = false;
    for (auto* inst : reader_entry->InstRange()) {
      if (inst == get2) found_get2 = true;
    }
    EXPECT_TRUE(found_get2)
        << "Written upvalue should NOT be preserved across call (second ctx "
           "must use fresh analysis, not stale cache from first ctx)";
  }
}

// ============================================================================
// OptPhiNotCondBranchJumpThreading: Additional null path variant
// ============================================================================

// ============================================================================
// OptCorrelatedBranches: catch block not folded
// ============================================================================

TEST_F(LEPUSIRTestNewCFGOpts, CorrelatedBranches_CatchBlockSkipped) {
  // Verify that catch blocks (blocks starting with CatchInst) are skipped by
  // OptCorrelatedBranches. Construct a CFG where a catch block has a correlated
  // condition that should NOT be folded.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_correlated_skip_catch";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* catch_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_succ =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* t =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* f =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, cond, catch_bb, false_succ);

  // catch_bb starts with CatchInst → IsCatchBlock returns true
  builder.SetInsertionPointToStart(catch_bb);
  builder.Create<CatchInst>(0);
  builder.Create<CondBranchInst>(0, cond, t, f);

  builder.SetInsertionPointToStart(false_succ);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(2));
  builder.SetInsertionPointToStart(t);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));
  builder.SetInsertionPointToStart(f);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // catch_bb must NOT be folded since it's a catch block
  auto* term = catch_bb->GetTerminator();
  auto* cbr = llvh::dyn_cast<CondBranchInst>(term);
  ASSERT_NE(cbr, nullptr) << "Catch block's CondBranch should not be folded";
  EXPECT_EQ(cbr->GetTrueDest(), t);
  EXPECT_EQ(cbr->GetFalseDest(), f);
}

// ============================================================================
// Idempotent call DCE: verify unused idempotent call is removed
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, IdempotentCall_RemovedByDCEWhenUnused) {
  // An idempotent call whose result is unused should be eliminable by DCE.
  // This verifies the DCE change works correctly.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_idempotent_dce";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Create a call and mark it as idempotent.
  ArgList args;
  args.push_back(builder.GetLiteralInt32(42));
  auto* callee = builder.GetLiteralInt32(999);
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetIdempotentCall(true);

  // Don't use the call result — just return a constant.
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  // Verify call exists before DCE.
  bool call_exists_before = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == call) call_exists_before = true;
  }
  EXPECT_TRUE(call_exists_before) << "Call should exist before DCE";

  // Run DCE via RunOnModule.
  DCE dce(ir_ctx_.get());
  dce.RunOnModule(mod_);

  // After DCE, the idempotent call with no users should be removed.
  bool call_exists_after = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == call) call_exists_after = true;
  }
  EXPECT_FALSE(call_exists_after)
      << "Unused idempotent call should be eliminated by DCE";
}

TEST_F(LEPUSIRTestNewLSEOpts, NonIdempotentCall_NotRemovedByDCE) {
  // A non-idempotent call (with side effects) must NOT be removed even if
  // unused.
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test_non_idempotent_dce";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  ArgList args;
  args.push_back(builder.GetLiteralInt32(42));
  auto* callee = builder.GetLiteralInt32(999);
  auto* call = builder.Create<CallInst>(0, callee, args);
  // NOT marked as idempotent — has side effects.

  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  DCE dce(ir_ctx_.get());
  dce.RunOnModule(mod_);

  // Non-idempotent call must remain even if unused.
  bool call_exists = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == call) call_exists = true;
  }
  EXPECT_TRUE(call_exists)
      << "Non-idempotent call with side effects must NOT be removed by DCE";
}

// ============================================================================
// Interprocedural table-safety analysis tests
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, TableSafe_PreservesTableCacheAcrossCall) {
  // A function with no SetTable/SetTableConstStringKey is table-safe.
  // Calls to table-safe functions (resolved via upvalue name) should preserve
  // table cache entries, eliminating redundant GetTable after the call.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Create a table-safe callee: no SetTable, just returns a value.
  std::string callee_name = "safe_helper";
  auto* callee_func = builder.Create<FuncOp>(0, callee_name);
  auto callee_lepus = lepus::Function::Create();
  callee_func->Init(callee_lepus);
  auto* callee_entry = builder.CreateBlock(callee_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(callee_entry);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(42));

  // Create the caller with an upvalue named "safe_helper".
  std::string caller_name = "caller";
  auto* caller_func = builder.Create<FuncOp>(0, caller_name);
  auto caller_lepus = lepus::Function::Create();
  caller_lepus->AddUpvalue("safe_helper", false, 0);
  caller_func->Init(caller_lepus);
  auto* caller_entry = builder.CreateBlock(caller_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(caller_entry);

  // Use a Parameter as receiver (preserved by InvalidateForLocalTableSafeCall).
  auto* obj = caller_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(5);
  auto* get1 = builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call safe_helper via GetUpvalue.
  auto* uv_index = builder.GetLiteralUint8(0);
  auto* get_callee =
      builder.Create<GetUpvalueInst>(0, caller_func, (LiteralUint8*)uv_index);
  ArgList args;
  args.push_back(get1);
  builder.Create<CallInst>(0, get_callee, args);

  // Second GetTable of same obj+key — should be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(caller_func);

  // Since safe_helper is table-safe, table cache should be preserved.
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : caller_entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_FALSE(found_get2)
      << "Second table read should be eliminated (table-safe call preserves "
         "cache)";
}

TEST_F(LEPUSIRTestNewLSEOpts, TableUnsafe_InvalidatesTableCache) {
  // A function with SetTableConstStringKeyInst is NOT table-safe.
  // Calls to it must invalidate table caches.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Create an unsafe callee: has SetTableInst on a Parameter receiver.
  // This makes param_properties_never_written_ = false AND the function is
  // not table-safe.
  std::string callee_name = "unsafe_helper";
  auto* callee_func = builder.Create<FuncOp>(0, callee_name);
  auto callee_lepus = lepus::Function::Create();
  callee_func->Init(callee_lepus);
  auto* callee_entry = builder.CreateBlock(callee_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(callee_entry);
  auto* param = callee_func->CreateParam(0);
  auto* callee_key = builder.GetLiteralInt32(1);
  auto* val = builder.GetLiteralInt32(99);
  builder.Create<SetTableInst>(0, param, callee_key, val);
  builder.Create<ReturnInst>(0, param);

  // Create the caller with an upvalue named "unsafe_helper".
  std::string caller_name = "caller_unsafe";
  auto* caller_func = builder.Create<FuncOp>(0, caller_name);
  auto caller_lepus = lepus::Function::Create();
  caller_lepus->AddUpvalue("unsafe_helper", false, 0);
  caller_func->Init(caller_lepus);
  auto* caller_entry = builder.CreateBlock(caller_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(caller_entry);

  // Use a Parameter as receiver.
  auto* obj = caller_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(5);
  auto* get1 = builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call unsafe_helper via GetUpvalue.
  auto* uv_index = builder.GetLiteralUint8(0);
  auto* get_callee =
      builder.Create<GetUpvalueInst>(0, caller_func, (LiteralUint8*)uv_index);
  ArgList args;
  args.push_back(get1);
  builder.Create<CallInst>(0, get_callee, args);

  // Second GetTable — must NOT be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(caller_func);

  // Since unsafe_helper has SetTableInst, it's not table-safe.
  // Table cache must be invalidated.
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : caller_entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_TRUE(found_get2)
      << "Second table read must remain (unsafe call invalidates table cache)";
}

TEST_F(LEPUSIRTestNewLSEOpts, TableSafe_TransitiveCallToSafeFunction) {
  // Table-safety is transitive: if A calls B and B is table-safe,
  // then A is also table-safe (if A itself has no SetTable).
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Create leaf function: table-safe (no SetTable, no calls).
  std::string leaf_name = "leaf_safe";
  auto* leaf_func = builder.Create<FuncOp>(0, leaf_name);
  auto leaf_lepus = lepus::Function::Create();
  leaf_func->Init(leaf_lepus);
  auto* leaf_entry = builder.CreateBlock(leaf_func->GetSingleRegion(),
                                         BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(leaf_entry);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  // Create middle function: calls leaf_safe via upvalue, no SetTable.
  std::string mid_name = "mid_safe";
  auto* mid_func = builder.Create<FuncOp>(0, mid_name);
  auto mid_lepus = lepus::Function::Create();
  mid_lepus->AddUpvalue("leaf_safe", false, 0);
  mid_func->Init(mid_lepus);
  auto* mid_entry = builder.CreateBlock(mid_func->GetSingleRegion(),
                                        BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(mid_entry);
  auto* mid_uv_idx = builder.GetLiteralUint8(0);
  auto* mid_callee =
      builder.Create<GetUpvalueInst>(0, mid_func, (LiteralUint8*)mid_uv_idx);
  ArgList mid_args;
  builder.Create<CallInst>(0, mid_callee, mid_args);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(2));

  // Create top-level caller that calls mid_safe.
  std::string top_name = "top_caller";
  auto* top_func = builder.Create<FuncOp>(0, top_name);
  auto top_lepus = lepus::Function::Create();
  top_lepus->AddUpvalue("mid_safe", false, 0);
  top_func->Init(top_lepus);
  auto* top_entry = builder.CreateBlock(top_func->GetSingleRegion(),
                                        BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(top_entry);

  // Use a Parameter as receiver (preserved by InvalidateForLocalTableSafeCall).
  auto* obj = top_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(7);
  auto* get1 = builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call mid_safe via GetUpvalue.
  auto* top_uv_idx = builder.GetLiteralUint8(0);
  auto* top_callee =
      builder.Create<GetUpvalueInst>(0, top_func, (LiteralUint8*)top_uv_idx);
  ArgList top_args;
  top_args.push_back(get1);
  builder.Create<CallInst>(0, top_callee, top_args);

  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(top_func);

  // mid_safe calls leaf_safe (table-safe) and has no SetTable → also safe.
  // Table cache should be preserved across call to mid_safe.
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : top_entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_FALSE(found_get2)
      << "Second table read should be eliminated (transitive table-safe call)";
}

TEST_F(LEPUSIRTestNewLSEOpts,
       TableSafe_ReassignedUpvalue_DoesNotTrustNameResolution) {
  // Regression test for soundness bug: if a function variable captured via
  // upvalue is reassigned to an unsafe function, the table-safety analysis
  // must NOT trust the name-based callee resolution.
  //
  // Scenario (in Lepus JS):
  //   function safe_func() { return 1; }   // table-safe (no SetTable)
  //   function caller(param) {
  //     let x = param.key;       // GetTable(param, key)
  //     safe_func();             // call via upvalue
  //     return param.key;        // GetTable(param, key) - must NOT be elided
  //   }
  //   safe_func = function() { param.key = 99; };  // reassignment
  //
  // Before fix: LSE trusts name "safe_func" → considers call table-safe →
  //   preserves table cache → eliminates second GetTable (WRONG).
  // After fix: LSE detects register is written → rejects name resolution →
  //   treats call as generic → invalidates table cache → keeps second GetTable.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // --- Root function setup ---
  // We need a root function that has the SetToplevelVarInst with ClosureVarReg
  // to simulate the reassignment, and a GetToplevelClosureVarInst to ensure the
  // register appears in the tracking sets.
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  root_func->SetTopLevelFunction();
  auto root_lepus = lepus::Function::Create();
  root_func->Init(root_lepus);
  auto* root_entry = builder.CreateBlock(root_func->GetSingleRegion(),
                                         BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(root_entry);

  // Simulate: the closure register 5 initially holds safe_func, then gets
  // reassigned. GetToplevelClosureVarInst reads register 5.
  auto* reg5_lit = builder.GetLiteralUint32(5);
  builder.Create<GetToplevelClosureVarInst>(0, (Literal*)reg5_lit,
                                            TypeOp::CreateAnyType(&builder));

  // SetToplevelVarInst with ClosureVarReg=5 (simulates reassignment via
  // ProcessSpecialMovPass: "safe_func = unsafeFunc").
  auto* reassign_val = builder.GetLiteralInt32(999);
  auto* set_tv =
      builder.Create<SetToplevelVarInst>(0, (Literal*)reg5_lit, reassign_val);
  set_tv->SetClosureVarReg(5);

  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  // --- Create safe_func: no SetTable → table-safe candidate ---
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string safe_name = "safe_func";
  auto* safe_func = builder.Create<FuncOp>(0, safe_name);
  auto safe_lepus = lepus::Function::Create();
  safe_func->Init(safe_lepus);
  auto* safe_entry = builder.CreateBlock(safe_func->GetSingleRegion(),
                                         BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(safe_entry);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  // --- Create param_writer: has SetTable on param → makes
  // param_properties_never_written_ = false, so generic calls invalidate
  // param-derived table entries ---
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string writer_name = "param_writer";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);
  auto* w_param = writer_func->CreateParam(0);
  auto* w_key = builder.GetLiteralInt32(1);
  auto* w_val = builder.GetLiteralInt32(42);
  builder.Create<SetTableInst>(0, w_param, w_key, w_val);
  builder.Create<ReturnInst>(0, w_param);

  // --- Create caller_func: captures "safe_func" via upvalue at register 5 ---
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string caller_name = "caller";
  auto* caller_func = builder.Create<FuncOp>(0, caller_name);
  auto caller_lepus = lepus::Function::Create();
  // Upvalue: name="safe_func", register=5, in_parent_vars=false
  caller_lepus->AddUpvalue("safe_func", 5, false);
  caller_func->Init(caller_lepus);
  auto* caller_entry = builder.CreateBlock(caller_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(caller_entry);

  // First table read from parameter.
  auto* obj = caller_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(7);
  auto* get1 = builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call via upvalue index 0 ("safe_func" at register 5).
  auto* uv_index = builder.GetLiteralUint8(0);
  auto* get_callee =
      builder.Create<GetUpvalueInst>(0, caller_func, (LiteralUint8*)uv_index);
  ArgList args;
  args.push_back(get1);
  builder.Create<CallInst>(0, get_callee, args);

  // Second table read of same obj+key — must NOT be eliminated because the
  // upvalue register was reassigned (call is NOT provably table-safe).
  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(caller_func);

  // Verify: second GetTable must remain (not eliminated).
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : caller_entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_TRUE(found_get2)
      << "Second table read must remain: upvalue register was reassigned, "
         "so the call cannot be proven table-safe via name resolution";
}

TEST_F(LEPUSIRTestNewLSEOpts,
       ParamDerived_GetTableOnParam_InvalidatedByGenericCall) {
  // Regression test: when param_properties_never_written_ = false (some
  // function writes to param.x) but param_gettable_properties_never_written_ =
  // true (no function writes to getTable(param, k).x), a generic call must
  // still invalidate table entries whose receiver is a getTable-on-param
  // derived value.
  //
  // The soundness issue: a callee writing param.x = v receives the caller's
  // getTable-derived object AS its parameter (through argument passing), so
  // preserving getTable-derived cache entries is unsound.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // --- Writer function: SetTable(param, key, val) ---
  // This makes param_properties_never_written_ = false.
  // It does NOT write to getTable(param, k).x, so
  // param_gettable_properties_never_written_ stays true.
  std::string writer_name = "param_writer";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);
  auto* w_param = writer_func->CreateParam(0);
  auto* w_key = builder.GetLiteralInt32(1);
  auto* w_val = builder.GetLiteralInt32(42);
  builder.Create<SetTableInst>(0, w_param, w_key, w_val);
  builder.Create<ReturnInst>(0, w_param);

  // --- Caller function ---
  // Has param "data", does:
  //   sub = GetTable(data, key_child)   -- sub is param-derived via GetTable
  //   val = GetTable(sub, key_x)        -- cached as (sub, key_x) -> val
  //   call writer(sub) via upvalue      -- generic non-table-safe call
  //   reload = GetTable(sub, key_x)     -- must NOT be eliminated
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string caller_name = "caller";
  auto* caller_func = builder.Create<FuncOp>(0, caller_name);
  auto caller_lepus = lepus::Function::Create();
  caller_lepus->AddUpvalue("param_writer", false, 0);
  caller_func->Init(caller_lepus);
  auto* caller_entry = builder.CreateBlock(caller_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(caller_entry);

  auto* data = caller_func->CreateParam(0);

  // sub = GetTable(data, key_child) -- sub is param-derived
  auto* key_child = builder.GetLiteralInt32(2);
  auto* sub = builder.Create<GetTableInst>(0, data, (Literal*)key_child);

  // val = GetTable(sub, key_x) -- this is cached by LSE
  auto* key_x = builder.GetLiteralInt32(3);
  auto* val = builder.Create<GetTableInst>(0, sub, (Literal*)key_x);

  // Call writer(sub) via GetUpvalue -- a generic non-table-safe call.
  auto* uv_index = builder.GetLiteralUint8(0);
  auto* get_callee =
      builder.Create<GetUpvalueInst>(0, caller_func, (LiteralUint8*)uv_index);
  ArgList args;
  args.push_back(sub);
  builder.Create<CallInst>(0, get_callee, args);

  // reload = GetTable(sub, key_x) -- must NOT be eliminated after fix.
  auto* reload = builder.Create<GetTableInst>(0, sub, (Literal*)key_x);
  builder.Create<ReturnInst>(0, reload);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(caller_func);

  // Verify: reload must remain (cache correctly invalidated by generic call).
  bool found_val = false;
  bool found_reload = false;
  for (auto* inst : caller_entry->InstRange()) {
    if (inst == val) found_val = true;
    if (inst == reload) found_reload = true;
  }
  EXPECT_TRUE(found_val) << "First GetTable(sub, key_x) should remain";
  EXPECT_TRUE(found_reload)
      << "Second GetTable(sub, key_x) must remain: generic call can pass "
         "getTable-derived object to a function that writes param.x";
}

// ============================================================================
// Regression test: mutating builtins (Array.push/pop) must NOT make a function
// table-safe.
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts, TableSafe_MutatingBuiltinNotSafe) {
  // A callee that calls Array.push (mutating builtin with BuiltinFuncName set
  // but NOT readonly/idempotent) must NOT be classified as table-safe.
  //
  // Key insight: the table-safe path (InvalidateForLocalTableSafeCall)
  // preserves cache entries whose receiver is NewTableInst. The conservative
  // path does NOT preserve NewTable receivers (they are not param-derived). So
  // using a NewTable receiver differentiates the two paths:
  //   - BUG (table-safe): entry preserved → get2 eliminated (wrong)
  //   - FIX (not table-safe): entry erased → get2 remains (correct)
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // --- Create callee function: calls Array.push (mutating builtin) ---
  std::string callee_name = "push_helper";
  auto* callee_func = builder.Create<FuncOp>(0, callee_name);
  auto callee_lepus = lepus::Function::Create();
  callee_func->Init(callee_lepus);
  auto* callee_entry = builder.CreateBlock(callee_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(callee_entry);

  // Simulate: array.push(val) — a CallInst with BuiltinFuncName="Array.push"
  // but NOT marked readonly or idempotent.
  auto* arr_param = callee_func->CreateParam(0);
  ArgList push_args;
  push_args.push_back(builder.GetLiteralInt32(42));
  auto* push_call = builder.Create<CallInst>(0, arr_param, push_args);
  push_call->SetBuiltinFuncName("Array.push");
  // Critically: NOT setting SetReadonlyCall(true) or SetIdempotentCall(true)
  builder.Create<ReturnInst>(0, arr_param);

  // --- Create caller function that calls push_helper via upvalue ---
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string caller_name = "caller";
  auto* caller_func = builder.Create<FuncOp>(0, caller_name);
  auto caller_lepus = lepus::Function::Create();
  caller_lepus->AddUpvalue("push_helper", 5, false);
  caller_func->Init(caller_lepus);
  auto* caller_entry = builder.CreateBlock(caller_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(caller_entry);

  // Use NewTable as the receiver — NOT param-derived, so the conservative
  // invalidation path erases it. But InvalidateForLocalTableSafeCall would
  // preserve it (NewTableInstKind is in the preserve list).
  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(5);
  auto* get1 = builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call push_helper via GetUpvalue.
  auto* uv_index = builder.GetLiteralUint8(0);
  auto* get_callee =
      builder.Create<GetUpvalueInst>(0, caller_func, (LiteralUint8*)uv_index);
  ArgList args;
  args.push_back(get1);
  builder.Create<CallInst>(0, get_callee, args);

  // Second GetTable of same obj+key — must NOT be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(caller_func);

  // Verify: second GetTable must remain (push_helper is NOT table-safe).
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : caller_entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_TRUE(found_get2)
      << "Second table read must remain: push_helper calls Array.push "
         "(mutating builtin), so it is NOT table-safe";
}

// ============================================================================
// Regression test: partial jump threading must preserve phi consistency.
// ============================================================================

TEST_F(LEPUSIRTestNewCFGOpts,
       PhiNotJumpThreading_PartialThread_PhiConsistency) {
  // When only some predecessors of mid can be threaded (partial threading),
  // the successor's phi must retain its entry for mid.
  //
  // CFG:
  //   entry → body, pred_direct (CondBranch)
  //   body → pred_null, pred_truthy (CondBranch)
  //   pred_null → mid              (null value)
  //   pred_truthy → pred_unknown, mid (CondBranch)
  //   pred_unknown → mid           (unknown Parameter value)
  //   pred_direct → true_succ      (provides extra phi entry)
  //   mid: phi(null/pred_null, truthy/pred_truthy, unknown/pred_unknown)
  //        Not(phi), CondBranch(not, true_succ, false_succ)
  //   true_succ: phi(phi_mid/mid, param2/pred_direct) + Return
  //   false_succ: Return
  //
  // pred_null threads to true_succ (Not(null)=true)
  // pred_truthy threads to false_succ (Not(truthy)=false)
  // pred_unknown cannot thread (unknown truthiness)
  // → mid still reachable → true_succ's phi must keep mid's entry.
  //
  // Without fix: UpdateEntry overwrites mid's entry → phi loses mid → FAIL
  // With fix: AddEntry preserves mid's entry → phi consistent → PASS
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_partial_thread_phi";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* body =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_null =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_truthy =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_unknown =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* pred_direct =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* mid =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_succ =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_succ =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry: dispatch to body or pred_direct
  builder.SetInsertionPointToStart(entry);
  auto* param0 = func->CreateParam(0);
  auto* param1 = func->CreateParam(1);
  auto* param2 = func->CreateParam(2);
  builder.Create<CondBranchInst>(0, param0, body, pred_direct);

  // body: dispatch to pred_null and pred_truthy
  builder.SetInsertionPointToStart(body);
  builder.Create<CondBranchInst>(0, param1, pred_null, pred_truthy);

  // pred_null: provides null → Branch(mid)
  builder.SetInsertionPointToStart(pred_null);
  auto* null_val =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, mid);

  // pred_truthy: provides truthy (NewTable) → CondBranch to pred_unknown or
  // mid
  builder.SetInsertionPointToStart(pred_truthy);
  auto* truthy_val = builder.Create<NewTableInst>(0);
  builder.Create<CondBranchInst>(0, param2, pred_unknown, mid);

  // pred_unknown: provides unknown (Parameter) → Branch(mid)
  builder.SetInsertionPointToStart(pred_unknown);
  builder.Create<BranchInst>(0, mid);

  // pred_direct: goes straight to true_succ (provides second phi entry)
  builder.SetInsertionPointToStart(pred_direct);
  auto* direct_val = builder.GetLiteralInt32(99);
  builder.Create<BranchInst>(0, true_succ);

  // mid: phi with 3 entries + Not + CondBranch
  builder.SetInsertionPointToStart(mid);
  PhiInst::ValueListType vals = {null_val, truthy_val, param2};
  PhiInst::BlockListType blks = {pred_null, pred_truthy, pred_unknown};
  auto* phi_mid = builder.Create<PhiInst>(0, vals, blks);
  auto* not_inst = builder.Create<UnaryOperatorInst>(
      0, phi_mid, ValueKind::UnaryNotInstKind);
  builder.Create<CondBranchInst>(0, not_inst, true_succ, false_succ);

  // true_succ: has a phi with TWO entries (so OptimizeSingleEntryPhi won't
  // eliminate it before jump threading runs)
  builder.SetInsertionPointToStart(true_succ);
  PhiInst::ValueListType ts_vals = {phi_mid, direct_val};
  PhiInst::BlockListType ts_blks = {mid, pred_direct};
  auto* true_phi = builder.Create<PhiInst>(0, ts_vals, ts_blks);
  builder.Create<ReturnInst>(0, true_phi);

  // false_succ: simple return
  builder.SetInsertionPointToStart(false_succ);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  // Run SimplifyCFG
  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // Verify: if mid is still reachable (pred_unknown still goes to it),
  // then true_succ's phi must still have an entry for mid.
  bool mid_still_exists = false;
  for (auto& b : *func) {
    if (&b == mid) {
      mid_still_exists = true;
      break;
    }
  }

  if (mid_still_exists) {
    // mid is still reachable — true_succ's phi must have an entry from mid.
    bool has_mid_entry = false;
    for (auto& inst : *true_succ) {
      auto* phi = llvh::dyn_cast<PhiInst>(&inst);
      if (!phi) break;
      for (unsigned i = 0; i < phi->GetNumEntries(); ++i) {
        if (phi->GetEntry(i).second == mid) {
          has_mid_entry = true;
          break;
        }
      }
    }
    EXPECT_TRUE(has_mid_entry)
        << "true_succ's phi must retain an entry for mid when mid is still "
           "reachable (partial threading)";
  }
}

// ============================================================================
// Bug fix regression tests
// ============================================================================

TEST_F(LEPUSIRTestNewCFGOpts, CorrelatedBranches_LoopHeaderNotFolded) {
  // Regression test: when dom_true is a loop header that structurally dominates
  // dom_bb (because dom_bb is inside the loop), the correlated branch
  // optimization must NOT fold, since bb is reached via the FALSE edge.
  //
  // CFG:
  //   entry: Branch(loop_header)
  //   loop_header: Branch(body)
  //   body: CondBranch(%cond, loop_header, inner)   ← dom_cbr
  //   inner: CondBranch(%cond, X, Y)                ← bb (should NOT fold)
  //   X: Branch(loop_header)                        ← back-edge
  //   Y: Return
  //
  // dom_true = loop_header dominates inner (structurally), but inner is reached
  // via body's FALSE edge → %cond is FALSE at inner, NOT true.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_correlated_loop_header";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* loop_header =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* body =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* inner =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* x_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* y_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry: Branch(loop_header)
  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  builder.Create<BranchInst>(0, loop_header);

  // loop_header: Branch(body)
  builder.SetInsertionPointToStart(loop_header);
  builder.Create<BranchInst>(0, body);

  // body: CondBranch(%cond, loop_header, inner)
  builder.SetInsertionPointToStart(body);
  builder.Create<CondBranchInst>(0, cond, loop_header, inner);

  // inner: CondBranch(%cond, X, Y) — the block we test
  builder.SetInsertionPointToStart(inner);
  builder.Create<CondBranchInst>(0, cond, x_bb, y_bb);

  // X: Branch(loop_header) — back-edge
  builder.SetInsertionPointToStart(x_bb);
  builder.Create<BranchInst>(0, loop_header);

  // Y: Return
  builder.SetInsertionPointToStart(y_bb);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // OptTrivialRebranchBlock correctly threads body's false-edge (cond==false)
  // directly to y_bb, bypassing inner. Other passes may merge the trivial
  // loop_header trampoline. Verify that body's false-edge reaches y_bb.
  auto* body_term = llvh::dyn_cast<CondBranchInst>(body->GetTerminator());
  ASSERT_NE(body_term, nullptr);
  EXPECT_EQ(body_term->GetCondition(), cond);
  EXPECT_EQ(body_term->GetFalseDest(), y_bb);
}

TEST_F(LEPUSIRTestNewLSEOpts, RegularCall_UpvalueWriteInvalidatesParamCache) {
  // Regression test: if a function writes to a GetUpvalue-loaded receiver,
  // param_properties_never_written_ must be false, preventing preservation
  // of param-derived table entries across regular calls.
  //
  // Scenario:
  //   function B() { getUpvalue().x = val; }  ← writes via upvalue alias
  //   function A(param) {
  //     var a = param.x;   ← cached
  //     call();            ← generic call, should invalidate param cache
  //     var b = param.x;   ← must NOT be eliminated
  //   }
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Function B: writes to an upvalue-loaded receiver.
  std::string writer_name = "upvalue_writer";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_lepus->AddUpvalue("sharedObj", false, 0);
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);
  auto* uv_idx = builder.GetLiteralUint8(0);
  auto* uv_obj =
      builder.Create<GetUpvalueInst>(0, writer_func, (LiteralUint8*)uv_idx);
  auto* w_key = builder.GetLiteralInt32(1);
  auto* w_val = builder.GetLiteralInt32(99);
  builder.Create<SetTableInst>(0, uv_obj, w_key, w_val);
  builder.Create<ReturnInst>(0, w_val);

  // Function A: reads param.x before and after a generic call.
  std::string reader_name = "param_reader";
  auto* reader_func = builder.Create<FuncOp>(0, reader_name);
  auto reader_lepus = lepus::Function::Create();
  reader_func->Init(reader_lepus);
  auto* entry = builder.CreateBlock(reader_func->GetSingleRegion(),
                                    BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* param = reader_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(1);

  // First read: param[key]
  auto* get1 = builder.Create<GetTableInst>(0, param, (Literal*)key);

  // Generic (non-readonly, non-table-safe) call.
  ArgList args;
  auto* callee = builder.GetLiteralInt32(999);
  builder.Create<CallInst>(0, callee, args);

  // Second read: param[key] — must NOT be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, param, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(reader_func);

  // Since writer_func writes to a GetUpvalue receiver, the analysis should
  // set param_properties_never_written_ = false. Therefore param-derived table
  // entries are NOT preserved across generic calls.
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_TRUE(found_get2)
      << "Second table read must remain (upvalue write makes "
         "param_properties_never_written_ false, cache invalidated)";
}

// Verify that a SetTable whose receiver is a non-builtin CallInst causes
// param_properties_never_written_ = false, preventing param-derived table
// entries from being preserved across generic calls.
TEST_F(LEPUSIRTestNewLSEOpts,
       CallInstReceiver_NonBuiltin_DisablesParamPreservation) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Writer function: calls an unknown function, then writes to its result.
  std::string writer_name = "writer_func";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);

  // call_result = CallInst(unknown_callee) — no builtin name set.
  ArgList w_args;
  auto* w_callee = builder.GetLiteralInt32(999);
  auto* call_result = builder.Create<CallInst>(0, w_callee, w_args);

  // SetTable(call_result, key, val) — writes to a call result.
  auto* w_key = builder.GetLiteralInt32(1);
  auto* w_val = builder.GetLiteralInt32(42);
  builder.Create<SetTableInst>(0, call_result, w_key, w_val);
  builder.Create<ReturnInst>(0, w_val);

  // Reader function: reads param[key] before and after a generic call.
  std::string reader_name = "reader_func";
  auto* reader_func = builder.Create<FuncOp>(0, reader_name);
  auto reader_lepus = lepus::Function::Create();
  reader_func->Init(reader_lepus);
  auto* entry = builder.CreateBlock(reader_func->GetSingleRegion(),
                                    BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* param = reader_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(1);

  auto* get1 = builder.Create<GetTableInst>(0, param, (Literal*)key);

  // Generic call (non-readonly, non-table-safe).
  ArgList r_args;
  auto* r_callee = builder.GetLiteralInt32(888);
  builder.Create<CallInst>(0, r_callee, r_args);

  auto* get2 = builder.Create<GetTableInst>(0, param, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(reader_func);

  // Because writer_func writes to a non-builtin CallInst receiver,
  // param_properties_never_written_ should be false. Therefore get2
  // must NOT be eliminated (cache invalidated by the generic call).
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_TRUE(found_get2)
      << "Second table read must remain (non-builtin CallInst receiver makes "
         "param_properties_never_written_ false)";
}

// Verify that a SetTable whose receiver is a builtin CallInst (fresh
// allocation) does NOT disable param preservation —
// param_properties_never_written_ remains true.
TEST_F(LEPUSIRTestNewLSEOpts,
       CallInstReceiver_Builtin_DoesNotDisableParamPreservation) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Writer function: calls a builtin that returns a fresh object, then writes
  // to it. This should NOT affect param_properties_never_written_.
  std::string writer_name = "writer_func";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);

  // call_result = CallInst(callee) with builtin name "__CreateElement".
  ArgList w_args;
  auto* w_callee = builder.GetLiteralInt32(999);
  auto* call_result = builder.Create<CallInst>(0, w_callee, w_args);
  call_result->SetBuiltinFuncName("__CreateElement");

  // SetTable(call_result, key, val) — writes to a fresh allocation.
  auto* w_key = builder.GetLiteralInt32(1);
  auto* w_val = builder.GetLiteralInt32(42);
  builder.Create<SetTableInst>(0, call_result, w_key, w_val);
  builder.Create<ReturnInst>(0, w_val);

  // Reader function: reads param[key] before and after a generic call.
  std::string reader_name = "reader_func";
  auto* reader_func = builder.Create<FuncOp>(0, reader_name);
  auto reader_lepus = lepus::Function::Create();
  reader_func->Init(reader_lepus);
  auto* entry = builder.CreateBlock(reader_func->GetSingleRegion(),
                                    BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* param = reader_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(1);

  auto* get1 = builder.Create<GetTableInst>(0, param, (Literal*)key);

  // Generic call (non-readonly, non-table-safe).
  ArgList r_args;
  auto* r_callee = builder.GetLiteralInt32(888);
  builder.Create<CallInst>(0, r_callee, r_args);

  auto* get2 = builder.Create<GetTableInst>(0, param, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(reader_func);

  // Because writer_func only writes to a builtin CallInst receiver (fresh
  // allocation), param_properties_never_written_ stays true. Therefore get2
  // should be eliminated (forwarded to get1, cache preserved across call).
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_FALSE(found_get2)
      << "Second table read should be eliminated (builtin CallInst receiver "
         "does not disable param preservation)";
}

// ============================================================================
// Regression test: SetTable whose receiver is GetTable(GetToplevelClosureVar,
// key) must set param_gettable_properties_never_written_ = false, so that
// param-derived cache entries are invalidated by generic calls.
// ============================================================================

TEST_F(LEPUSIRTestNewLSEOpts,
       ClosureVarDerived_SetTableOnGetTableOfClosure_InvalidatesParamCache) {
  // Bug scenario: a function does SetTable(GetTable(closureVar, k1), k2, val).
  // The receiver GetTable(closureVar, k1) is NOT caught by:
  //   - IsParamOrPhiOfParam (it's a GetTableInst)
  //   - IsParamDerived (traces to GetToplevelClosureVarInst, not a Param)
  //   - isa<GetUpvalueInst> (it's a GetTableInst)
  // So param_gettable_properties_never_written_ incorrectly stays true,
  // causing stale cache entries to survive generic calls.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // --- Writer function: SetTable(GetTable(GetToplevelClosureVar(reg), k1),
  //                               k2, val) ---
  // This should make param_gettable_properties_never_written_ = false.
  std::string writer_name = "closure_writer";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);

  // GetToplevelClosureVarInst loads a closure variable (register 3).
  auto* reg_lit = builder.GetLiteralUint32(3);
  auto* closure_var = builder.Create<GetToplevelClosureVarInst>(
      0, (Literal*)reg_lit, TypeOp::CreateAnyType(&builder));

  // GetTable(closure_var, key1) — the receiver of the subsequent SetTable.
  auto* key1 = builder.GetLiteralInt32(1);
  auto* derived_obj =
      builder.Create<GetTableInst>(0, closure_var, (Literal*)key1);

  // SetTable(derived_obj, key2, val) — writes to a sub-property of closure var.
  auto* key2 = builder.GetLiteralInt32(2);
  auto* val_lit = builder.GetLiteralInt32(42);
  builder.Create<SetTableInst>(0, derived_obj, key2, val_lit);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  // --- Caller function ---
  // Has param "data", does:
  //   sub = GetTable(data, key_child) — sub is param-derived via GetTable
  //   val = GetTable(sub, key_x)      — cached as (sub, key_x) -> val
  //   call func via upvalue           — generic non-table-safe call
  //   reload = GetTable(sub, key_x)   — must NOT be eliminated
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string caller_name = "caller";
  auto* caller_func = builder.Create<FuncOp>(0, caller_name);
  auto caller_lepus = lepus::Function::Create();
  caller_lepus->AddUpvalue("closure_writer", false, 0);
  caller_func->Init(caller_lepus);
  auto* caller_entry = builder.CreateBlock(caller_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(caller_entry);

  auto* data = caller_func->CreateParam(0);

  // sub = GetTable(data, key_child) — sub is param-derived
  auto* key_child = builder.GetLiteralInt32(10);
  auto* sub = builder.Create<GetTableInst>(0, data, (Literal*)key_child);

  // val = GetTable(sub, key_x) — this is cached by LSE
  auto* key_x = builder.GetLiteralInt32(11);
  auto* first_get = builder.Create<GetTableInst>(0, sub, (Literal*)key_x);

  // Call via GetUpvalue — a generic non-table-safe call.
  auto* uv_index = builder.GetLiteralUint8(0);
  auto* get_callee =
      builder.Create<GetUpvalueInst>(0, caller_func, (LiteralUint8*)uv_index);
  ArgList args;
  args.push_back(sub);
  builder.Create<CallInst>(0, get_callee, args);

  // reload = GetTable(sub, key_x) — must NOT be eliminated after fix.
  auto* second_get = builder.Create<GetTableInst>(0, sub, (Literal*)key_x);
  builder.Create<ReturnInst>(0, second_get);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(caller_func);

  // Verify: second_get must remain (cache correctly invalidated by generic
  // call because param_gettable_properties_never_written_ is now false).
  bool found_first = false;
  bool found_second = false;
  for (auto* inst : caller_entry->InstRange()) {
    if (inst == first_get) found_first = true;
    if (inst == second_get) found_second = true;
  }
  EXPECT_TRUE(found_first) << "First GetTable(sub, key_x) should remain";
  EXPECT_TRUE(found_second)
      << "Second GetTable(sub, key_x) must remain: a function writes to "
         "GetTable(closureVar, k).prop which can alias getTable-derived param "
         "entries at runtime";
}

TEST_F(LEPUSIRTestNewLSEOpts,
       GetContextSlotMovInst_DirectReceiver_InvalidatesParamCache) {
  // Regression: SetTable whose receiver is a GetContextSlotMovInst must set
  // param_properties_never_written_ = false, just like GetContextSlotInst.
  // GetContextSlotMovInst is semantically equivalent to GetContextSlotInst
  // (both load from a block context slot) and should be treated identically
  // for param-alias analysis.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Writer function: SetTable(GetContextSlotMovInst(ctx, idx), key, val).
  std::string writer_name = "ctx_slot_mov_writer";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);

  auto* ctx = builder.GetLiteralInt32(0);
  auto* index = builder.GetLiteralUint8(1);
  auto* ctx_slot_mov =
      builder.Create<GetContextSlotMovInst>(0, ctx, (LiteralUint8*)index);

  auto* w_key = builder.GetLiteralInt32(1);
  auto* w_val = builder.GetLiteralInt32(99);
  builder.Create<SetTableInst>(0, ctx_slot_mov, w_key, w_val);
  builder.Create<ReturnInst>(0, w_val);

  // Reader function: reads param[key] before and after a generic call.
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string reader_name = "param_reader_ctx_mov";
  auto* reader_func = builder.Create<FuncOp>(0, reader_name);
  auto reader_lepus = lepus::Function::Create();
  reader_func->Init(reader_lepus);
  auto* entry = builder.CreateBlock(reader_func->GetSingleRegion(),
                                    BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* param = reader_func->CreateParam(0);
  auto* key = builder.GetLiteralInt32(1);

  auto* get1 = builder.Create<GetTableInst>(0, param, (Literal*)key);

  ArgList args;
  auto* callee = builder.GetLiteralInt32(999);
  builder.Create<CallInst>(0, callee, args);

  auto* get2 = builder.Create<GetTableInst>(0, param, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(reader_func);

  // Verify: get2 must survive because GetContextSlotMovInst receiver triggers
  // param_properties_never_written_ = false.
  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1) << "First table read should remain";
  EXPECT_TRUE(found_get2)
      << "Second table read must remain (GetContextSlotMovInst write makes "
         "param_properties_never_written_ false, cache invalidated)";
}

TEST_F(LEPUSIRTestNewLSEOpts,
       GetContextSlotMovInst_ViaGetTable_InvalidatesParamGettableCache) {
  // Regression: SetTable whose receiver is GetTable(GetContextSlotMovInst, key)
  // must trigger IsClosureVarDerived -> true, setting
  // param_gettable_properties_never_written_ = false.
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  // Writer function: SetTable(GetTable(GetContextSlotMovInst(ctx, idx), k1),
  //                           k2, val)
  std::string writer_name = "ctx_slot_mov_gettable_writer";
  auto* writer_func = builder.Create<FuncOp>(0, writer_name);
  auto writer_lepus = lepus::Function::Create();
  writer_func->Init(writer_lepus);
  auto* writer_entry = builder.CreateBlock(writer_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(writer_entry);

  auto* ctx = builder.GetLiteralInt32(0);
  auto* index = builder.GetLiteralUint8(1);
  auto* ctx_slot_mov =
      builder.Create<GetContextSlotMovInst>(0, ctx, (LiteralUint8*)index);

  // GetTable(ctx_slot_mov, key1) — derived from closure var.
  auto* key1 = builder.GetLiteralInt32(1);
  auto* derived_obj =
      builder.Create<GetTableInst>(0, ctx_slot_mov, (Literal*)key1);

  // SetTable(derived_obj, key2, val)
  auto* key2 = builder.GetLiteralInt32(2);
  auto* val_lit = builder.GetLiteralInt32(42);
  builder.Create<SetTableInst>(0, derived_obj, key2, val_lit);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  // Caller function: param -> GetTable -> Call -> GetTable (gettable-derived).
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string caller_name = "caller_ctx_mov";
  auto* caller_func = builder.Create<FuncOp>(0, caller_name);
  auto caller_lepus = lepus::Function::Create();
  caller_lepus->AddUpvalue("ctx_slot_mov_gettable_writer", false, 0);
  caller_func->Init(caller_lepus);
  auto* caller_entry = builder.CreateBlock(caller_func->GetSingleRegion(),
                                           BlockType::BT_INST, 0, "entry");
  builder.SetInsertionPointToEnd(caller_entry);

  auto* data = caller_func->CreateParam(0);

  // sub = GetTable(data, key_child) — sub is param-derived
  auto* key_child = builder.GetLiteralInt32(10);
  auto* sub = builder.Create<GetTableInst>(0, data, (Literal*)key_child);

  // val = GetTable(sub, key_x) — cached by LSE
  auto* key_x = builder.GetLiteralInt32(11);
  auto* first_get = builder.Create<GetTableInst>(0, sub, (Literal*)key_x);

  // Call via GetUpvalue — a generic non-table-safe call.
  auto* uv_index = builder.GetLiteralUint8(0);
  auto* get_callee =
      builder.Create<GetUpvalueInst>(0, caller_func, (LiteralUint8*)uv_index);
  ArgList args;
  args.push_back(sub);
  builder.Create<CallInst>(0, get_callee, args);

  // reload = GetTable(sub, key_x) — must NOT be eliminated.
  auto* second_get = builder.Create<GetTableInst>(0, sub, (Literal*)key_x);
  builder.Create<ReturnInst>(0, second_get);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(caller_func);

  // Verify: second_get must remain.
  bool found_first = false;
  bool found_second = false;
  for (auto* inst : caller_entry->InstRange()) {
    if (inst == first_get) found_first = true;
    if (inst == second_get) found_second = true;
  }
  EXPECT_TRUE(found_first) << "First GetTable(sub, key_x) should remain";
  EXPECT_TRUE(found_second)
      << "Second GetTable(sub, key_x) must remain: writer uses "
         "GetContextSlotMovInst via GetTable chain, which should trigger "
         "IsClosureVarDerived and invalidate param-gettable cache";
}

// ============================================================================
// OptMergeTrivialCondArm tests
// ============================================================================
// Verifies the optimization that merges trivial conditional arms (containing
// only a LoadNull/LoadConst + Branch) into the predecessor block.

TEST_F(LEPUSIRTestNewCFGOpts, MergeTrivialCondArm_BasicFalseArm) {
  // Pattern:
  //   entry: CondBranch(cond, other_bb, trivial_bb)
  //   trivial_bb: v = LoadNull; Branch(join)
  //   other_bb: <some work>; Branch(join)
  //   join: phi(other_val/other_bb, v/trivial_bb); use phi; return
  //
  // Expected: trivial_bb merged into entry; phi updated to reference entry.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_merge_trivial_false";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* other_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* trivial_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  // entry: CondBranch(param, other_bb, trivial_bb)
  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, cond, other_bb, trivial_bb);

  // other_bb: produce a value and Branch to join
  builder.SetInsertionPointToStart(other_bb);
  auto* other_val = builder.Create<LoadConstInst>(
      0, builder.GetLiteralInt32(42), TypeOp::CreateInt32(&builder));
  builder.Create<BranchInst>(0, join);

  // trivial_bb: LoadNull + Branch(join)
  builder.SetInsertionPointToStart(trivial_bb);
  auto* null_val =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, join);

  // join: phi + GetTable(phi, key) + Return — non-trivial join prevents
  // OptPhiReturnThreading from transforming the CFG before our pass runs.
  builder.SetInsertionPointToStart(join);
  PhiInst::ValueListType phi_vals = {other_val, null_val};
  PhiInst::BlockListType phi_blks = {other_bb, trivial_bb};
  auto* phi = builder.Create<PhiInst>(0, phi_vals, phi_blks);
  phi->SetType(TypeOp::CreateAnyType(&builder));
  auto* key_lit = builder.GetLiteralInt32(0);
  auto* get = builder.Create<GetTableInst>(0, phi, (Literal*)key_lit);
  builder.Create<ReturnInst>(0, get);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // After optimization: trivial_bb should be gone. The phi in join should
  // reference entry instead of trivial_bb, and the LoadNull should be in entry.
  bool trivial_bb_exists = false;
  for (auto& b : *func) {
    if (&b == trivial_bb) trivial_bb_exists = true;
  }
  EXPECT_FALSE(trivial_bb_exists)
      << "Trivial block should have been merged into predecessor";

  // Verify LoadNull was hoisted into entry (before CondBranch).
  bool found_null_in_entry = false;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<LoadNullOrUndefinedInst>(inst)) found_null_in_entry = true;
  }
  EXPECT_TRUE(found_null_in_entry)
      << "LoadNull should be hoisted into entry block";
}

TEST_F(LEPUSIRTestNewCFGOpts, MergeTrivialCondArm_TrueArm) {
  // Symmetric case: true arm is trivial.
  //   entry: CondBranch(cond, trivial_bb, other_bb)
  //   trivial_bb: v = LoadConst(0); Branch(join)
  //   other_bb: <work>; Branch(join)
  //   join: phi(v/trivial_bb, other_val/other_bb)
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_merge_trivial_true";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* trivial_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* other_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, cond, trivial_bb, other_bb);

  builder.SetInsertionPointToStart(trivial_bb);
  auto* const_val = builder.Create<LoadConstInst>(
      0, builder.GetLiteralInt32(0), TypeOp::CreateInt32(&builder));
  builder.Create<BranchInst>(0, join);

  // other_bb must have >2 instructions so it does NOT match the trivial
  // pattern (otherwise arm=0 merges it first, preventing the true-arm test).
  builder.SetInsertionPointToStart(other_bb);
  auto* other_val = builder.Create<LoadConstInst>(
      0, builder.GetLiteralInt32(99), TypeOp::CreateInt32(&builder));
  auto* key_lit2 = builder.GetLiteralInt32(1);
  auto* get2 = builder.Create<GetTableInst>(0, other_val, (Literal*)key_lit2);
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(join);
  PhiInst::ValueListType phi_vals = {const_val, get2};
  PhiInst::BlockListType phi_blks = {trivial_bb, other_bb};
  auto* phi = builder.Create<PhiInst>(0, phi_vals, phi_blks);
  phi->SetType(TypeOp::CreateAnyType(&builder));
  auto* key_lit = builder.GetLiteralInt32(0);
  auto* get = builder.Create<GetTableInst>(0, phi, (Literal*)key_lit);
  builder.Create<ReturnInst>(0, get);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  bool trivial_bb_exists = false;
  for (auto& b : *func) {
    if (&b == trivial_bb) trivial_bb_exists = true;
  }
  EXPECT_FALSE(trivial_bb_exists)
      << "Trivial true-arm should have been merged into predecessor";
}

TEST_F(LEPUSIRTestNewCFGOpts, MergeTrivialCondArm_SkipLargeFunction) {
  // When function exceeds kMergeTrivialCondArmMaxInsts (100),
  // OptMergeTrivialCondArm should not fire.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_merge_trivial_skip_large";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* trivial_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* other_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, cond, other_bb, trivial_bb);

  // Fill other_bb with >100 instructions to exceed the limit.
  builder.SetInsertionPointToStart(other_bb);
  Value* last = nullptr;
  for (int i = 0; i < 105; i++) {
    last = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(i),
                                         TypeOp::CreateInt32(&builder));
  }
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(trivial_bb);
  auto* null_val =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(join);
  PhiInst::ValueListType phi_vals = {last, null_val};
  PhiInst::BlockListType phi_blks = {other_bb, trivial_bb};
  auto* phi = builder.Create<PhiInst>(0, phi_vals, phi_blks);
  phi->SetType(TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, phi);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // trivial_bb should still exist because function is too large.
  bool trivial_bb_exists = false;
  for (auto& b : *func) {
    if (&b == trivial_bb) trivial_bb_exists = true;
  }
  EXPECT_TRUE(trivial_bb_exists)
      << "Trivial arm should NOT be merged when function exceeds size limit";
}

TEST_F(LEPUSIRTestNewCFGOpts, MergeTrivialCondArm_SkipMultiplePredecessors) {
  // trivial_bb with multiple predecessors should NOT be merged.
  //   entry: CondBranch(cond, other_bb, trivial_bb)
  //   other_bb: SetTable(...); Branch(trivial_bb) ← non-trivial, adds 2nd pred
  //   trivial_bb: LoadNull; Branch(join)
  //   join: phi + use + Return
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_merge_trivial_multi_pred";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* other_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* trivial_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* join =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* param = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, param, other_bb, trivial_bb);

  // other_bb: side-effectful work + Branch(trivial_bb) → gives trivial_bb
  // a 2nd predecessor. SetTable is side-effectful so this block won't be
  // folded by OptimizeIndirectJump.
  builder.SetInsertionPointToStart(other_bb);
  auto* key = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(1),
                                            TypeOp::CreateInt32(&builder));
  auto* val = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(2),
                                            TypeOp::CreateInt32(&builder));
  builder.Create<SetTableInst>(0, param, key, val);
  builder.Create<BranchInst>(0, trivial_bb);

  builder.SetInsertionPointToStart(trivial_bb);
  auto* null_val =
      builder.Create<LoadNullOrUndefinedInst>(0, builder.GetLiteralInt8(1));
  builder.Create<BranchInst>(0, join);

  builder.SetInsertionPointToStart(join);
  PhiInst::ValueListType phi_vals = {null_val, null_val};
  PhiInst::BlockListType phi_blks = {trivial_bb, trivial_bb};
  auto* phi = builder.Create<PhiInst>(0, phi_vals, phi_blks);
  phi->SetType(TypeOp::CreateAnyType(&builder));
  auto* get_key = builder.GetLiteralInt32(0);
  auto* get = builder.Create<GetTableInst>(0, phi, (Literal*)get_key);
  builder.Create<ReturnInst>(0, get);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // trivial_bb should still exist due to multiple predecessors.
  bool trivial_bb_exists = false;
  for (auto& b : *func) {
    if (&b == trivial_bb) trivial_bb_exists = true;
  }
  EXPECT_TRUE(trivial_bb_exists)
      << "Trivial arm with multiple predecessors should NOT be merged";
}

// ============================================================================
// InvalidateForNativeRendererCall tests
// ============================================================================
// Verifies that native renderer calls (SetLocalTableSafeCall=true) correctly
// preserve table caches for safe receivers while invalidating unsafe ones.

TEST_F(LEPUSIRTestNewLSEOpts,
       NativeRendererCall_PreservesTableCacheForSafeReceiver) {
  // Pattern:
  //   obj = NewTable()
  //   v1 = GetTable(obj, key)
  //   Call(__SetAttribute, ...)  [LocalTableSafeCall=true]
  //   v2 = GetTable(obj, key)   ← should be eliminated (obj is safe receiver)
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test_native_renderer_safe";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t attr_idx = lepus_func->AddConstString("__SetAttribute");
  uint32_t key_idx = lepus_func->AddConstString("width");
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(key_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, key);

  // Create a call annotated as LocalTableSafeCall
  auto* callee = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(attr_idx), TypeOp::CreateString(&builder));
  ArgList args;
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  auto* get2 = builder.Create<GetTableInst>(0, obj, key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // get2 should be eliminated because NewTable is a safe receiver.
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "GetTable from NewTable receiver should be eliminated after native "
         "renderer call (safe receiver preserved)";
}

TEST_F(LEPUSIRTestNewLSEOpts, NativeRendererCall_InvalidatesUnsafeReceiver) {
  // Pattern:
  //   obj = GetGlobal(...)  ← unsafe receiver
  //   v1 = GetTable(obj, key)
  //   Call(__SetAttribute, ...) [LocalTableSafeCall=true]
  //   v2 = GetTable(obj, key)  ← should NOT be eliminated
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test_native_renderer_unsafe";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t attr_idx = lepus_func->AddConstString("__SetAttribute");
  uint32_t key_idx = lepus_func->AddConstString("height");
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Unsafe receiver (GetGlobal)
  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(key_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, key);

  auto* callee = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(attr_idx), TypeOp::CreateString(&builder));
  ArgList args2;
  auto* call = builder.Create<CallInst>(0, callee, args2);
  call->SetLocalTableSafeCall(true);

  auto* get2 = builder.Create<GetTableInst>(0, obj, key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // get2 should NOT be eliminated because GetGlobal is unsafe receiver.
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "GetTable from GetGlobal receiver should NOT be eliminated after "
         "native renderer call (unsafe receiver invalidated)";
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
