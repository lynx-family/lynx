// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/update_toplevel_closure_var.h"
#include "core/runtime/lepus/ir/transformer/vm/mov_elimination.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"
#include "core/runtime/lepus/ir/transformer/vm/register_compaction_pass.h"
#include "core/runtime/lepus/ir/transformer/vm/toplevel_store_optimization.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRRedundantMovEliminationTest : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }

  virtual void TearDown(void) {}

  FuncOp* createTestFunction(std::string name) {
    auto builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

    auto* func_op = builder->Create<FuncOp>(0, name);
    EXPECT_NE(nullptr, func_op);

    auto region = builder->CreateRegion(func_op);
    EXPECT_NE(nullptr, region);

    auto block = builder->CreateBlock(region, BlockType::BT_INST, {});
    EXPECT_NE(nullptr, block);

    return func_op;
  }

  FuncOp* createToplevelTestFunction(std::string name) {
    auto* func_op = createTestFunction(std::move(name));
    func_op->SetTopLevelFunction();
    mod->SetRootFunction(func_op);
    return func_op;
  }
};

TEST_F(LEPUSIRRedundantMovEliminationTest, EliminateRedundantMov) {
  auto* func = createTestFunction("test_redundant");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Create instructions
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));

  auto* mov1 = builder->Create<MovInst>(0, src);

  ArgList args;
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, mov1, args);
  auto* mov2 = builder->Create<MovInst>(0, src);
  builder->Create<ReturnInst>(0, mov2);

  // 2. Run RA pass
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  // 3. Run MovEliminationPass
  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // 4. Verify
  size_t mov_count = 0;
  for (auto& op : *block) {
    if (auto* mov = llvh::dyn_cast<MovInst>(&op)) {
      if (mov->GetSingleOperand() == src) mov_count++;
    }
  }

  // mov1 should be removed (same src/dst or tracked redundant),
  // but call_mov logic might keep one or ra might have assigned them
  // differently. Generally we expect redundancy to be eliminated.
  EXPECT_LE(mov_count, 1u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, UnallocatedMovShouldNotCrash) {
  // Regression:
  // MovEliminationPass::RemoveMovWithSameSrcAndDst must not call
  // RegisterAllocator::GetRegister() on an unallocated MovInst.
  auto* func = createTestFunction("test_unallocated_mov");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  auto* mov = builder->Create<MovInst>(0, src);
  // Keep mov alive so it won't be removed as dead.
  builder->Create<ReturnInst>(0, mov);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(src));
  ASSERT_TRUE(ra->IsAllocated(mov));

  // Simulate a pipeline state where MOV exists but is not allocated.
  ra->RemoveFromAllocated(mov);
  ASSERT_FALSE(ra->IsAllocated(mov));

  MovEliminationPass pass(ir_ctx.get());
  EXPECT_NO_THROW(pass.RemoveMovWithSameSrcAndDst(func));
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       ReuseMovAcrossCallWhenNoClobberOverlap) {
  auto* func = createTestFunction("test_reuse_across_call_no_overlap");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Two MOVs with the same (src_reg, dst_reg) pair separated by a call.
  // The call is forced to be a non-consecutive call, but its clobbered range
  // does NOT overlap with src/dst regs of the tracked MOV. The second MOV
  // should still be eliminated as redundant.
  auto* src =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));
  auto* mov1 = builder->Create<MovInst>(0, src);

  // Give mov1 a use before the call so it won't be dropped as dead.
  builder->Create<SetUpvalueInst>(0, func, builder->GetLiteralUint8(0), mov1);

  auto* fn =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(1));
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(123),
                                             TypeOp::CreateInt32(builder));
  ArgList args;
  args.push_back(arg);
  auto* call = builder->Create<CallInst>(0, fn, args);

  auto* mov2 = builder->Create<MovInst>(0, src);
  builder->Create<ReturnInst>(0, mov2);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);

  // Force mov1/mov2 to have the same reg pair.
  ra->UpdateRegister(src, Register(1));
  ra->UpdateRegister(mov1, Register(5));
  ra->UpdateRegister(mov2, Register(5));

  // Force the call to be a non-consecutive call with clobber range far away:
  // func_reg = 100, argc = 1 => clobbered range is [101, 101].
  // This does not overlap with {1, 5}.
  if (Value* call_fn = call->GetFunction();
      call_fn && ra->IsAllocated(call_fn)) {
    ra->UpdateRegister(call_fn, Register(100));
  }
  if (ra->IsAllocated(arg)) {
    ra->UpdateRegister(arg, Register(0));
  }
  if (ra->IsAllocated(call)) {
    ra->UpdateRegister(call, Register(2));
  }

  // Stabilize this test: ensure no other instruction outputs to $1/$5, which
  // would invalidate active_movs and make the test flaky.
  unsigned next_safe_reg2 = 300;
  Value* call_fn2 = call->GetFunction();
  for (auto& op : *block) {
    auto* I = llvh::dyn_cast<Instruction>(&op);
    if (!I) continue;
    if (!ra->IsAllocated(I)) continue;
    if (I == src || I == mov1 || I == mov2 || I == call_fn2 || I == arg ||
        I == call) {
      continue;
    }
    if (!I->HasOutput()) continue;
    unsigned r = ra->GetRegister(I).GetIndex();
    if (r == 1u || r == 5u) {
      ra->UpdateRegister(I, Register(next_safe_reg2++));
    }
  }

  if (auto* call_fn_mov2 = llvh::dyn_cast<MovInst>(call_fn2)) {
    Value* src_v = call_fn_mov2->GetSingleOperand();
    if (src_v && ra->IsAllocated(src_v) &&
        ra->GetRegister(src_v).GetIndex() ==
            ra->GetRegister(call_fn_mov2).GetIndex()) {
      ra->UpdateRegister(src_v, Register(next_safe_reg2++));
    }
  }

  MovEliminationPass pass(ir_ctx.get());
  pass.RemoveMovWithSameSrcAndDst(func);

  bool mov1_exists = false;
  bool mov2_exists = false;
  for (auto& op : *block) {
    if (&op == mov1) mov1_exists = true;
    if (&op == mov2) mov2_exists = true;
  }
  EXPECT_TRUE(mov1_exists);
  EXPECT_FALSE(mov2_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       ToplevelCallShouldNotClobberPreallocatedRegs) {
  // Regression test:
  // In toplevel functions, toplevel variables are preallocated into the prefix
  // register range [0..prefix-1]. VM Call1/CallRandom* fast-paths materialize
  // arguments into `a+1..a+argc`, which can clobber those low registers.
  //
  // Even if IR liveness does not see all toplevel vars as live at the call
  // site, we must conservatively keep the callee out of the prefix range when
  // argc > 0.

  auto* func = createToplevelTestFunction("test_toplevel_call_clobber_fix");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Create 7 toplevel var anchors (original regs 0..6) and register them into
  // IRContext so RA will Preallocate them.
  llvh::SmallVector<Instruction*, 8> tops;
  for (unsigned r = 0; r < 7; ++r) {
    auto* v = builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(static_cast<int32_t>(r)),
        TypeOp::CreateInt32(builder));
    v->SetToplevelVarReg(r);
    ir_ctx->InsertToplevelValue(v, r);
    tops.push_back(v);
  }

  // Build a non-consecutive single-arg call: callee is in a higher toplevel
  // slot, and arg comes from a lower slot. Without moving the callee to a fresh
  // top register, Call1 would clobber `callee+1` which is still in the
  // preallocated toplevel range.
  ArgList args;
  args.push_back(tops[0]);
  auto* call = builder->Create<CallInst>(0, tops[5], args);
  builder->Create<ReturnInst>(0, call);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  // Verify RA inserted a call-func MOV to move the callee out of the prefix.
  auto* call_fn = call->GetFunction();
  auto* call_fn_mov = llvh::dyn_cast<MovInst>(call_fn);
  ASSERT_NE(nullptr, call_fn_mov);
  EXPECT_TRUE(call_fn_mov->IsCallFuncMov());
}

TEST_F(LEPUSIRRedundantMovEliminationTest, EliminateMovWithSingleUseSrc) {
  auto* func = createTestFunction("test_single_use_src");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Create instructions
  auto* src =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));

  auto* mov = builder->Create<MovInst>(0, src);
  mov->SetCallFuncMov(true);

  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);

  builder->Create<ReturnInst>(0, call);

  // 2. Run RA pass
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  unsigned mov_reg = ra->GetRegister(mov).GetIndex();

  // Make sure `src` and `mov` are in different physical registers so the pass
  // really performs a register reassignment (UpdateRegister) on `src`.
  unsigned forced_src_reg = mov_reg + 1;
  ra->UpdateRegister(src, Register(forced_src_reg));

  // Avoid conflict by keeping call's output register away from `mov_reg`.
  if (ra->IsAllocated(call) && ra->GetRegister(call).GetIndex() == mov_reg) {
    ra->UpdateRegister(call, Register(mov_reg + 2));
  }

  // 3. Run MovEliminationPass
  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // 4. Verify
  // src's register should have been updated to mov's register
  EXPECT_EQ(ra->GetRegister(src).GetIndex(), mov_reg);

  // src is the instruction whose register got reassigned by this pass.
  EXPECT_TRUE(src->IsFixReg());

  size_t mov_count = 0;
  for (auto& op : *block) {
    if (llvh::isa<MovInst>(&op)) mov_count++;
  }
  // A new call-func MOV may be re-inserted to satisfy the VM call clobber
  // convention; allow at most one MOV in this tiny function.
  EXPECT_LE(mov_count, 1u);

  // call should use either src directly or a freshly inserted call-func MOV
  // whose operand is src.
  if (call->GetFunction() != src) {
    auto* call_mov = llvh::dyn_cast<MovInst>(call->GetFunction());
    ASSERT_NE(nullptr, call_mov);
    EXPECT_TRUE(call_mov->IsCallFuncMov());
    EXPECT_TRUE(call_mov->IsFixReg());
    EXPECT_EQ(call_mov->GetSingleOperand(), src);
  }
}

TEST_F(LEPUSIRRedundantMovEliminationTest, EliminateBackToBackReverseMov) {
  auto* func = createTestFunction("test_back_to_back_reverse_mov");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Build a minimal pattern in IR:
  //   mov1 = Mov(src)
  //   mov2 = Mov(mov1)
  // After register allocation we force registers to form:
  //   mov  r3 <- r1
  //   mov  r1 <- r3
  // The second MOV is a redundant back-copy and should be eliminated.
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                             TypeOp::CreateInt32(builder));
  auto* mov1 = builder->Create<MovInst>(0, src);
  auto* mov2 = builder->Create<MovInst>(0, mov1);
  auto* ret = builder->Create<ReturnInst>(0, mov2);
  (void)ret;

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);

  // Force the back-to-back reverse pattern:
  // src in r1, mov1 in r3, mov2 in r1 (so mov2 writes back into src reg).
  ra->UpdateRegister(src, Register(1));
  ra->UpdateRegister(mov1, Register(3));
  ra->UpdateRegister(mov2, Register(1));

  // Stabilize: avoid other outputs clobbering r1/r3.
  unsigned next_safe = 100;
  for (auto& op : *block) {
    auto* I = llvh::dyn_cast<Instruction>(&op);
    if (!I) continue;
    if (I == src || I == mov1 || I == mov2) continue;
    if (!I->HasOutput()) continue;
    if (!ra->IsAllocated(I)) continue;
    unsigned r = ra->GetRegister(I).GetIndex();
    if (r == 1u || r == 3u) {
      ra->UpdateRegister(I, Register(next_safe++));
    }
  }

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // mov2 should be gone, and Return should read the original src directly.
  bool mov2_exists = false;
  ReturnInst* return_inst = nullptr;
  for (auto& op : *block) {
    if (&op == mov2) mov2_exists = true;
    if (!return_inst) {
      return_inst = llvh::dyn_cast<ReturnInst>(&op);
    }
  }
  EXPECT_FALSE(mov2_exists);
  ASSERT_NE(nullptr, return_inst);
  EXPECT_EQ(return_inst->GetValue(), src);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       BackToBackReverseMovShouldNotTouchSpecialMov) {
  auto* func = createTestFunction("test_reverse_mov_special");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(7),
                                             TypeOp::CreateInt32(builder));
  auto* mov1 = builder->Create<MovInst>(0, src);
  auto* mov2 = builder->Create<MovInst>(0, mov1);
  // Mark mov2 as special (toplevel var) so mov elimination must not rewrite it.
  mov2->SetToplevelVarReg(0);
  builder->Create<ReturnInst>(0, mov2);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);

  // Force the same register pattern, but the pass must ignore it because mov2
  // is special.
  ra->UpdateRegister(src, Register(1));
  ra->UpdateRegister(mov1, Register(3));
  ra->UpdateRegister(mov2, Register(1));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool mov2_exists = false;
  for (auto& op : *block) {
    if (&op == mov2) mov2_exists = true;
  }
  EXPECT_TRUE(mov2_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       EliminateMovEvenIfSrcHasMultipleUsersWhenSafe) {
  auto* func = createTestFunction("test_multi_use_src_call_mov");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // src has multiple users: (1) a Mov used by Call, (2) Return uses src.
  auto* src =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));
  auto* mov = builder->Create<MovInst>(0, src);
  mov->SetCallFuncMov(true);
  ArgList args;
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, mov, args);
  builder->Create<ReturnInst>(0, src);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(src));
  ASSERT_TRUE(ra->IsAllocated(mov));

  unsigned mov_reg = ra->GetRegister(mov).GetIndex();
  unsigned forced_src_reg = mov_reg + 1;
  ra->UpdateRegister(src, Register(forced_src_reg));

  // Avoid conflict by keeping call's output register away from mov_reg.
  if (ra->IsAllocated(call) && ra->GetRegister(call).GetIndex() == mov_reg) {
    ra->UpdateRegister(call, Register(mov_reg + 2));
  }

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // src's physical register should be updated to mov's register.
  EXPECT_EQ(ra->GetRegister(src).GetIndex(), mov_reg);
  EXPECT_TRUE(src->IsFixReg());

  // mov should have been eliminated.
  size_t mov_count = 0;
  for (auto& op : *block) {
    if (llvh::isa<MovInst>(&op)) mov_count++;
  }
  EXPECT_EQ(mov_count, 0u);

  // call should now use src directly.
  EXPECT_EQ(call->GetFunction(), src);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, EliminateSelfMov) {
  auto* func = createTestFunction("test_self_mov");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));

  auto* mov = builder->Create<MovInst>(0, src);
  builder->Create<ReturnInst>(0, mov);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  // Force same register for testing self-move elimination
  ra->UpdateRegister(mov, ra->GetRegister(src));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  size_t mov_count = 0;
  for (auto& op : *block) {
    if (llvh::isa<MovInst>(&op)) mov_count++;
  }
  EXPECT_EQ(mov_count, 0u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, DoNotEliminateClobberedMov) {
  auto* func = createTestFunction("test_clobber");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  [[maybe_unused]] auto* mov1 = builder->Create<MovInst>(0, src);

  // An instruction that clobbers src's register.
  auto* clobber = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(200), TypeOp::CreateInt32(builder));

  auto* mov2 = builder->Create<MovInst>(0, src);
  builder->Create<ReturnInst>(0, mov2);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  // Force different registers
  // src -> 0, mov1 -> 1, clobber -> 0, mov2 -> 2
  ra->UpdateRegister(src, Register(0));
  ra->UpdateRegister(mov1, Register(1));
  ra->UpdateRegister(clobber, Register(0));
  ra->UpdateRegister(mov2, Register(2));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // mov2 should NOT be removed because src was clobbered.
  bool mov2_exists = false;
  for (auto& op : *block) {
    if (&op == mov2) mov2_exists = true;
  }
  EXPECT_TRUE(mov2_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, DoNotEliminateMovOnConflict) {
  auto* func = createTestFunction("test_call_mov_conflict");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));
  auto* other = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                               TypeOp::CreateInt32(builder));
  auto* mov = builder->Create<MovInst>(0, src);

  ArgList args;
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, mov, args);
  builder->Create<ReturnInst>(0, other);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  // Force conflict: set other's register to mov's register
  ra->UpdateRegister(other, ra->GetRegister(mov));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // mov should NOT be eliminated because 'other' uses the same register and
  // overlaps.
  size_t mov_count = 0;
  for (auto& op : *block) {
    if (llvh::isa<MovInst>(&op)) mov_count++;
  }
  EXPECT_EQ(mov_count, 1u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       EliminateSetToplevelVarByCoalescingProducerReg) {
  auto* func = createToplevelTestFunction("test_set_toplevel_coalesce");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Create one toplevel var so Preallocate assigns it fixed reg 0.
  auto* toplevel_init = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1), TypeOp::CreateInt32(builder));
  ir_ctx->InsertToplevelValue(toplevel_init, 0);

  // Producer + Set are adjacent.
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                             TypeOp::CreateInt32(builder));
  auto* set_top =
      builder->Create<SetToplevelVarInst>(0, builder->GetLiteralUint32(0), src);
  builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(src));
  EXPECT_EQ(src->GetNumUsers(), 1u);
  EXPECT_FALSE(src->IsFixReg());

  // Force src away from the target reg so coalescing is observable.
  ra->UpdateRegister(src, Register(5));
  ASSERT_NE(ra->GetRegister(src).GetIndex(), 0u);

  ToplevelStoreOptimizationPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  EXPECT_EQ(ra->GetRegister(src).GetIndex(), 0u);
  EXPECT_TRUE(src->IsFixReg());

  bool set_exists = false;
  for (auto& op : *block) {
    if (&op == set_top) set_exists = true;
  }
  EXPECT_FALSE(set_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       DoNotEliminateSetToplevelVarByCoalescingOnConflict) {
  // Ensure the set-toplevel-elimination pass respects live-range conflicts in
  // the fixed target register.
  auto* func =
      createToplevelTestFunction("test_set_toplevel_coalesce_conflict");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Producer: single-use by Set.
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(42),
                                             TypeOp::CreateInt32(builder));
  auto* other = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(99),
                                               TypeOp::CreateInt32(builder));

  auto* set_top =
      builder->Create<SetToplevelVarInst>(0, builder->GetLiteralUint32(0), src);
  // Keep `other` live across the Set, so its interval overlaps `src`.
  builder->Create<ReturnInst>(0, other);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(src));
  ASSERT_TRUE(ra->IsAllocated(other));
  EXPECT_EQ(src->GetNumUsers(), 1u);

  // Force `other` into the target reg 0 to create an overlap conflict.
  ra->UpdateRegister(other, Register(0));
  // Force `src` away from target reg so we can observe whether coalescing
  // happens.
  ra->UpdateRegister(src, Register(5));
  ASSERT_NE(ra->GetRegister(src).GetIndex(), 0u);

  ToplevelStoreOptimizationPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // Must not Coalesce due to conflict.
  EXPECT_NE(ra->GetRegister(src).GetIndex(), 0u);

  bool set_exists = false;
  for (auto& op : *block) {
    if (&op == set_top) set_exists = true;
  }
  EXPECT_TRUE(set_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       EliminateSetToplevelClosureVarInToplevelByCoalescingProducerReg) {
  auto* func = createToplevelTestFunction("test_set_toplevel_closure_coalesce");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  constexpr uint32_t kOldClosureReg = 10;
  auto* closure_value = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1), TypeOp::CreateInt32(builder));
  closure_value->SetClosureVarReg(kOldClosureReg);
  func->RecordClosureVarRegAndValue(kOldClosureReg, closure_value);

  // Producer + Set are adjacent.
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(99),
                                             TypeOp::CreateInt32(builder));
  auto* set_closure = builder->Create<SetToplevelClosureVarInst>(
      0, builder->GetLiteralUint32(kOldClosureReg), src);
  builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(closure_value));
  unsigned target_reg = ra->GetRegister(closure_value).GetIndex();

  // Ensure the target register is available for coalescing (no interval
  // conflicts with other values currently assigned to the same register).
  for (auto& kv : ra->GetAllocatedMap()) {
    if (kv.second.GetIndex() != target_reg) continue;
    if (kv.first == closure_value || kv.first == set_closure) continue;
    ra->UpdateRegister(kv.first, Register(target_reg + 10));
  }

  ra->UpdateRegister(src, Register(target_reg + 3));
  EXPECT_FALSE(src->IsFixReg());

  ToplevelStoreOptimizationPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  EXPECT_EQ(ra->GetRegister(src).GetIndex(), target_reg);
  EXPECT_TRUE(src->IsFixReg());

  bool set_exists = false;
  for (auto& op : *block) {
    if (&op == set_closure) set_exists = true;
  }
  EXPECT_FALSE(set_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       RefreshUpvalueMappingIfClosureAnchorRegIsReassigned) {
  auto builder = ir_ctx->GetOpBuilder();

  // 1) Create toplevel root function (must call init() before building region).
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string root_name = "test_refresh_upvalue_mapping";
  auto* root = builder->Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root->Init(root_lepus_func);
  root->SetTopLevelFunction();
  mod->SetRootFunction(root);

  auto* entry =
      builder->CreateBlock(root->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToEnd(entry);

  // 2) Create a child lepus::Function with one upvalue referring to root's
  // closure old reg.
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string child_name = "child";
  auto* child_op = builder->Create<FuncOp>(0, child_name);
  auto child_lepus_func = lepus::Function::Create();
  child_op->Init(child_lepus_func);
  root_lepus_func->AddChildFunction(child_lepus_func);

  constexpr uint32_t kOldClosureReg = 10;
  child_lepus_func->AddUpvalue("x", kOldClosureReg, true);

  // 3) Create a fixed toplevel var reg (0), and a closure anchor value.
  builder->SetInsertionPointToEnd(entry);
  auto* toplevel_init = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1), TypeOp::CreateInt32(builder));
  ir_ctx->InsertToplevelValue(toplevel_init, 0);

  auto* closure_anchor = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(7), TypeOp::CreateInt32(builder));
  closure_anchor->SetClosureVarReg(kOldClosureReg);
  root->RecordClosureVarRegAndValue(kOldClosureReg, closure_anchor);
  root->InsertToplevelClosureVarReg(kOldClosureReg);

  // Force the anchor to be the sole user of this Set, so the elimination pass
  // can Coalesce it.
  auto* set_top = builder->Create<SetToplevelVarInst>(
      0, builder->GetLiteralUint32(0), closure_anchor);
  builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));

  // 4) Run RA.
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(root);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(root);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(closure_anchor));

  // Put the closure anchor in a non-0 register so the mapping is observable.
  ra->UpdateRegister(closure_anchor, Register(20));
  ASSERT_EQ(ra->GetRegister(closure_anchor).GetIndex(), 20u);

  // 5) Run UpdateToplevelClosureVarPass first (as in pipeline).
  auto* opt_pass = CreateUpdateToplevelClosureVarPass(ir_ctx.get());
  static_cast<ModulePass*>(opt_pass)->RunOnModule(mod);
  delete opt_pass;
  EXPECT_EQ(child_op->GetClosureVarToplevelReg(0), 20);

  // 6) Now eliminate redundant SetToplevelVar by coalescing the producer into
  // the fixed toplevel reg 0. This changes closure_anchor's physical reg.
  ToplevelStoreOptimizationPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  EXPECT_EQ(ra->GetRegister(closure_anchor).GetIndex(), 0u);
  EXPECT_EQ(child_op->GetClosureVarToplevelReg(0), 0);

  bool set_exists = false;
  for (auto& op : *entry) {
    if (&op == set_top) set_exists = true;
  }
  EXPECT_FALSE(set_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       DoNotReassignRegisterForClosureAnchorEvenIfAttrsCleared) {
  auto builder = ir_ctx->GetOpBuilder();

  // Create toplevel function.
  auto* func = createToplevelTestFunction("test_skip_anchor_reg_reassign");
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  constexpr uint32_t kOldClosureReg = 10;
  auto* closure_anchor = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(7), TypeOp::CreateInt32(builder));
  closure_anchor->SetClosureVarReg(kOldClosureReg);
  func->RecordClosureVarRegAndValue(kOldClosureReg, closure_anchor);

  // Simulate a later/other pass clearing special attrs, while the side-table
  // still treats this value as the closure anchor.
  closure_anchor->SetClosureVarReg(constants::kInvalidSignedValue);
  closure_anchor->SetToplevelVarReg(constants::kInvalidSignedValue);
  closure_anchor->SetFixReg(false);

  // Create Mov + Call so RedundantMovElimination may try to Coalesce.
  auto* mov = builder->Create<MovInst>(0, closure_anchor);
  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);
  builder->Create<ReturnInst>(0, call);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(closure_anchor));
  ASSERT_TRUE(ra->IsAllocated(mov));

  // Force src/mov into different regs and avoid conflicts.
  unsigned mov_reg = ra->GetRegister(mov).GetIndex();
  ra->UpdateRegister(closure_anchor, Register(mov_reg + 5));
  ASSERT_NE(ra->GetRegister(closure_anchor).GetIndex(), mov_reg);

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // Must not rewrite anchor's physical register.
  EXPECT_NE(ra->GetRegister(closure_anchor).GetIndex(), mov_reg);
  EXPECT_FALSE(closure_anchor->IsFixReg());
}

TEST_F(LEPUSIRRedundantMovEliminationTest, DoNotEliminateMovWithMultiUsers) {
  auto* func = createTestFunction("test_multi_users");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));
  auto* mov = builder->Create<MovInst>(0, src);

  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);

  // Another user for src
  [[maybe_unused]] auto* mov2 = builder->Create<MovInst>(0, src);

  builder->Create<ReturnInst>(0, call);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // mov should NOT be eliminated in EliminateMov because src has two users:
  // mov and mov2.
  bool mov_exists = false;
  for (auto& op : *block) {
    if (&op == mov) mov_exists = true;
  }
  EXPECT_TRUE(mov_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, DoNotEliminateMovForToplevel) {
  auto* func = createTestFunction("test_toplevel_mov");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Create instructions
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  src->SetToplevelVarReg(10);  // Mark as top-level

  auto* mov = builder->Create<MovInst>(0, src);

  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);
  builder->Create<ReturnInst>(0, call);

  // 2. Run RA pass
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  // This test only cares about the generic redundant-mov elimination. However,
  // register allocation may inject a call-func MOV for the call-site, and our
  // manual register overrides must not violate its invariant (dst_reg !=
  // src_reg).
  for (auto& op : *block) {
    auto* m = llvh::dyn_cast<MovInst>(&op);
    if (!m || !m->IsCallFuncMov()) continue;
    if (!ra->IsAllocated(m) || !ra->IsAllocated(m->GetSingleOperand()))
      continue;
    // Put call-func MOV into a high reg, and ensure it differs from its
    // operand.
    ra->UpdateRegister(m, Register(10));
    if (ra->GetRegister(m->GetSingleOperand()).GetIndex() == 10u) {
      ra->UpdateRegister(m->GetSingleOperand(), Register(11));
    }
  }

  // Force DIFFERENT registers for (src, mov) to avoid self-move removal.
  ra->UpdateRegister(src, Register(0));
  ra->UpdateRegister(mov, Register(2));

  // 3. Run MovEliminationPass
  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // 4. Verify
  // mov should NOT be eliminated for top-level src because EliminateMov
  // should skip it and they have different registers.
  bool mov_exists = false;
  for (auto& op : *block) {
    if (&op == mov) mov_exists = true;
  }
  EXPECT_TRUE(mov_exists);
  EXPECT_EQ(ra->GetRegister(src).GetIndex(), 0u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, DoNotEliminateMovForClosure) {
  auto* func = createTestFunction("test_closure_mov");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Create instructions
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  src->SetClosureVarReg(10);  // Mark as closure

  auto* mov = builder->Create<MovInst>(0, src);

  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);
  builder->Create<ReturnInst>(0, call);

  // 2. Run RA pass
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  // Same as toplevel test: keep any injected call-func MOV well-formed before
  // we override registers.
  for (auto& op : *block) {
    auto* m = llvh::dyn_cast<MovInst>(&op);
    if (!m || !m->IsCallFuncMov()) continue;
    if (!ra->IsAllocated(m) || !ra->IsAllocated(m->GetSingleOperand()))
      continue;
    ra->UpdateRegister(m, Register(10));
    if (ra->GetRegister(m->GetSingleOperand()).GetIndex() == 10u) {
      ra->UpdateRegister(m->GetSingleOperand(), Register(11));
    }
  }

  // Force DIFFERENT registers for (src, mov) to avoid self-move removal.
  ra->UpdateRegister(src, Register(0));
  ra->UpdateRegister(mov, Register(2));

  // 3. Run MovEliminationPass
  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // 4. Verify
  bool mov_exists = false;
  for (auto& op : *block) {
    if (&op == mov) mov_exists = true;
  }
  EXPECT_TRUE(mov_exists);
  EXPECT_EQ(ra->GetRegister(src).GetIndex(), 0u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       CoalesceMovEvenIfSrcHasCallUserBeforeMov) {
  // This test validates a reg-alloc coalescing corner case:
  // the source value has a CallInst user (target-specific lowering), which
  // previously made the source "manually allocated" and blocked coalescing.
  // We still want to coalesce a following MOV if intervals do not overlap.
  auto* func = createTestFunction("test_coalesce_with_call_user");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  auto* fn =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(1));

  // Call uses `src` as an argument before the MOV.
  ArgList args;
  args.push_back(src);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, fn, args);

  // A redundant copy after the call.
  auto* mov = builder->Create<MovInst>(0, src);
  builder->Create<ReturnInst>(0, mov);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  EXPECT_TRUE(ra->IsManuallyAllocatedInterval(src));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // The MOV should become a self-move (coalesced) and be removed.
  size_t mov_count = 0;
  for (auto& op : *block) {
    auto* m = llvh::dyn_cast<MovInst>(&op);
    if (!m) continue;
    if (m->GetSingleOperand() == src && !m->IsCallFuncMov()) {
      mov_count++;
    }
  }
  EXPECT_EQ(mov_count, 0u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, EliminateMultipleMovs) {
  auto* func = createTestFunction("test_multiple_movs");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Call 1
  auto* src1 =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(1));
  auto* mov1 = builder->Create<MovInst>(0, src1);
  mov1->SetCallFuncMov(true);
  ArgList args1;
  [[maybe_unused]] auto* call1 = builder->Create<CallInst>(0, mov1, args1);

  // Call 2
  auto* src2 =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(2));
  auto* mov2 = builder->Create<MovInst>(0, src2);
  mov2->SetCallFuncMov(true);
  ArgList args2;
  auto* call2 = builder->Create<CallInst>(0, mov2, args2);

  builder->Create<ReturnInst>(0, call2);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  unsigned mov1_reg = ra->GetRegister(mov1).GetIndex();
  unsigned mov2_reg = ra->GetRegister(mov2).GetIndex();

  // Force `src` and `mov` to be different regs so the pass definitely performs
  // UpdateRegister(src, mov_reg) for both pairs.
  ra->UpdateRegister(src1, Register(mov1_reg + 1));
  ra->UpdateRegister(src2, Register(mov2_reg + 1));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // Both should be eliminated
  EXPECT_EQ(ra->GetRegister(src1).GetIndex(), mov1_reg);
  EXPECT_EQ(ra->GetRegister(src2).GetIndex(), mov2_reg);

  // All values whose registers were reassigned by EliminateMov must be
  // marked as fixed.
  EXPECT_TRUE(src1->IsFixReg());
  EXPECT_TRUE(src2->IsFixReg());

  size_t mov_count = 0;
  for (auto& op : *block) {
    if (llvh::isa<MovInst>(&op)) mov_count++;
  }

  // The pass may re-insert call-func MOVs to satisfy the VM call clobber
  // convention. Allow at most one per call in this function.
  EXPECT_LE(mov_count, 2u);

  auto verify_call_target = [&](CallInst* call, Instruction* expected_src) {
    ASSERT_NE(nullptr, call);
    Value* fn = call->GetFunction();
    if (fn == expected_src) return;
    auto* call_mov = llvh::dyn_cast<MovInst>(fn);
    ASSERT_NE(nullptr, call_mov);
    EXPECT_TRUE(call_mov->IsCallFuncMov());
    EXPECT_TRUE(call_mov->IsFixReg());
    EXPECT_EQ(call_mov->GetSingleOperand(), expected_src);
  };
  verify_call_target(call1, src1);
  verify_call_target(call2, src2);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, EliminateNewArrayMovSetsFixReg) {
  auto* func = createTestFunction("test_new_array_mov_fix_reg");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));
  auto* mov = builder->Create<MovInst>(0, src);
  mov->SetCallFuncMov(true);

  ArgList items;
  items.push_back(mov);
  auto* arr = builder->Create<NewArrayInst>(0, items);
  builder->Create<ReturnInst>(0, arr);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  unsigned mov_reg = ra->GetRegister(mov).GetIndex();

  // Ensure src/mov are different regs so EliminateMov triggers.
  ra->UpdateRegister(src, Register(mov_reg + 1));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  EXPECT_EQ(ra->GetRegister(src).GetIndex(), mov_reg);
  EXPECT_TRUE(src->IsFixReg());

  // mov should be eliminated and NewArrayInst should use src directly.
  EXPECT_EQ(arr->GetOperand(0), src);
  size_t mov_count = 0;
  for (auto& op : *block) {
    if (llvh::isa<MovInst>(&op)) mov_count++;
  }
  EXPECT_EQ(mov_count, 0u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       CallFuncMovInvariant_NotAnchorAndNoSpecialAttrs) {
  auto builder = ir_ctx->GetOpBuilder();

  // Create a function with a parameter used as the call target. Parameters are
  // preallocated to low fixed registers, so if there are other values live at
  // the call-site, RegisterAllocator::ProcessCallInst should inject a
  // call-func MOV (IsCallFuncMov=true) into a fresh top register.
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string func_name = "test_call_func_mov_invariant";
  auto* func = builder->Create<FuncOp>(0, func_name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  auto* block =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToEnd(block);

  auto* p0 = func->CreateParam(0);

  // A value live across the call-site.
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                            TypeOp::CreateInt32(builder));

  ArgList args;
  auto* call = builder->Create<CallInst>(0, p0, args);

  // Use both v1 and call after the call-site to ensure v1 is live at the call.
  auto* add = builder->Create<BinaryOperatorInst>(
      0, v1, call, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(builder));
  builder->Create<ReturnInst>(0, add);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  size_t call_func_mov_cnt = 0;
  for (auto& op : *block) {
    auto* mov = llvh::dyn_cast<MovInst>(&op);
    if (!mov) continue;
    if (!mov->IsCallFuncMov()) continue;
    ++call_func_mov_cnt;

    // Must carry no special attributes.
    ASSERT_EQ(mov->GetClosureVarReg(), constants::kInvalidSignedValue);
    ASSERT_EQ(mov->GetToplevelVarReg(), constants::kInvalidSignedValue);

    // Must not be a side-table anchor.
    for (const auto& kv : ir_ctx->GetToplevelVariables()) {
      ASSERT_NE(kv.second, mov);
    }
    for (const auto& kv : func->GetClosureVarReg2ValueMap()) {
      ASSERT_NE(kv.second, mov);
    }
  }
  ASSERT_GT(call_func_mov_cnt, 0u);

  // Running the pass should not hit any invariant asserts.
  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);
}

TEST_F(LEPUSIRRedundantMovEliminationTest, DoNotEliminateAcrossBlocks) {
  auto* func = createTestFunction("test_across_blocks");
  auto builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();

  auto* region = func->GetRegion(0);
  auto* next_block = builder->CreateBlock(region, BlockType::BT_INST, {});

  builder->SetInsertionPointToEnd(entry);
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  builder->Create<BranchInst>(0, next_block);

  builder->SetInsertionPointToEnd(next_block);
  auto* mov = builder->Create<MovInst>(0, src);
  ArgList args;
  auto* call = builder->Create<CallInst>(0, mov, args);
  builder->Create<ReturnInst>(0, call);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  // Register allocation may have injected call-func MOV(s) for the call-site.
  // Our manual register overrides must not violate the invariant
  // (call-func MOV dst_reg != src_reg), otherwise MovEliminationPass will hit
  // debug assertions.
  for (auto& op : *next_block) {
    auto* m = llvh::dyn_cast<MovInst>(&op);
    if (!m || !m->IsCallFuncMov()) continue;
    if (!ra->IsAllocated(m) || !ra->IsAllocated(m->GetSingleOperand()))
      continue;
    ra->UpdateRegister(m, Register(10));
    if (ra->GetRegister(m->GetSingleOperand()).GetIndex() == 10u) {
      ra->UpdateRegister(m->GetSingleOperand(), Register(11));
    }
  }

  // Force different registers for (src, mov) to avoid self-move removal.
  ra->UpdateRegister(src, Register(0));
  ra->UpdateRegister(mov, Register(2));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // Should NOT be eliminated because src and mov are in different blocks
  bool mov_exists = false;
  for (auto& op : *next_block) {
    if (&op == mov) mov_exists = true;
  }
  EXPECT_TRUE(mov_exists);
  EXPECT_EQ(ra->GetRegister(src).GetIndex(), 0u);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       EliminateTrackedRedundantMovWithIntervalUpdate) {
  auto* func = createTestFunction("test_tracked_redundant");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));

  // mov1: reg src -> reg 1
  auto* mov1 = builder->Create<MovInst>(0, src);

  // Give mov1 a user so it's not removed as "userless"
  [[maybe_unused]] auto* dummy_user = builder->Create<MovInst>(0, mov1);

  // Some intermediate instruction that uses neither reg src nor reg 1
  [[maybe_unused]] auto* other = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(200), TypeOp::CreateInt32(builder));

  // mov2: reg src -> reg 1
  auto* mov2 = builder->Create<MovInst>(0, src);

  builder->Create<ReturnInst>(0, mov2);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  // Force registers: src -> 0, mov1 -> 1, dummy_user -> 3, other -> 2, mov2 ->
  // 1
  ra->UpdateRegister(src, Register(0));
  ra->UpdateRegister(mov1, Register(1));
  ra->UpdateRegister(dummy_user, Register(3));
  ra->UpdateRegister(other, Register(2));
  ra->UpdateRegister(mov2, Register(1));

  // Initialize intervals
  ra->GetInstructionInterval(mov1);        // ensure created
  ra->GetInstructionInterval(dummy_user);  // ensure created
  ra->GetInstructionInterval(mov2);        // ensure created
  unsigned mov1_end_before = ra->GetInstructionInterval(mov1).End();
  unsigned mov2_end = ra->GetInstructionInterval(mov2).End();

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // mov2 should be eliminated and replaced by mov1
  bool mov2_exists = false;
  for (auto& op : *block) {
    if (&op == mov2) mov2_exists = true;
  }
  EXPECT_FALSE(mov2_exists);

  // mov1's interval should have been extended to cover mov2's uses
  EXPECT_GT(ra->GetInstructionInterval(mov1).End(), mov1_end_before);
  EXPECT_EQ(ra->GetInstructionInterval(mov1).End(), mov2_end);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       RegisterCompactionMakesRegsDenseAndKeepsCallSafety) {
  auto* func = createTestFunction("test_register_compaction");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Keep two values live across a call.
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                            TypeOp::CreateInt32(builder));
  auto* v2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                            TypeOp::CreateInt32(builder));

  auto* target_func =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));
  ArgList args;
  auto* call = builder->Create<CallInst>(0, target_func, args);

  // Use v1/v2 after call so they are live at the call-site.
  auto* sum = builder->Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, sum);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(v1));
  ASSERT_TRUE(ra->IsAllocated(v2));
  ASSERT_TRUE(ra->IsAllocated(call->GetFunction()));

  // Run post-RA pass first (as in pipeline).
  MovEliminationPass mov_elimination_pass(ir_ctx.get());
  mov_elimination_pass.RunOnFunction(func);

  // Introduce sparse register indices (holes) to simulate post-RA rewrites.
  Value* call_func = call->GetFunction();
  ra->UpdateRegister(v1, Register(10));
  ra->UpdateRegister(v2, Register(30));
  ra->UpdateRegister(call_func, Register(50));
  if (ra->IsAllocated(call)) {
    ra->UpdateRegister(call, Register(60));
  }

  // Run compaction.
  RegisterCompactionPass compact(ir_ctx.get());
  compact.RunOnFunction(func);

  // Verify dense register range for non-prefix registers.
  const unsigned prefix = static_cast<unsigned>(func->GetParamSize());
  llvh::SmallDenseSet<unsigned, 64> used;
  unsigned max_used = 0;
  for (const auto& kv : ra->GetAllocatedMap()) {
    if (!kv.second.IsValid()) continue;
    unsigned idx = kv.second.GetIndex();
    used.insert(idx);
    max_used = std::max(max_used, idx);
  }
  for (unsigned r = prefix; r <= max_used; ++r) {
    EXPECT_TRUE(used.count(r));
  }

  // Verify call safety: function reg must be >= max live reg at call-site.
  auto max_live_excluding_call_func = [&]() {
    unsigned call_idx = ra->GetInstructionNumber(call);
    Segment call_point(call_idx + 1, call_idx + 2);
    Value* func_op = call->GetFunction();
    unsigned max_live = 0;
    for (auto& pair : ra->GetAllocatedMap()) {
      Value* v = pair.first;
      Register reg = pair.second;
      if (!reg.IsValid()) continue;
      if (v == func_op) continue;
      auto* inst = llvh::dyn_cast<Instruction>(v);
      if (!inst) continue;
      if (!ra->HasInstructionNumber(inst)) continue;
      if (inst == call) continue;
      if (ra->GetInstructionInterval(inst).Intersects(call_point)) {
        max_live = std::max(max_live, reg.GetIndex());
      }
    }
    // Conservatively include call operands (arguments).
    for (int i = 0, e = call->GetNumOperands(); i < e; ++i) {
      Value* op = call->GetOperand(i);
      if (op == func_op) continue;
      if (!ra->IsAllocated(op)) continue;
      auto* inst = llvh::dyn_cast<Instruction>(op);
      if (inst && ra->HasInstructionNumber(inst) &&
          !ra->GetInstructionInterval(inst).Intersects(call_point)) {
        continue;
      }
      max_live = std::max(max_live, ra->GetRegister(op).GetIndex());
    }
    return max_live;
  };

  EXPECT_GE(ra->GetRegister(call->GetFunction()).GetIndex(),
            max_live_excluding_call_func());
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       RegisterCompactionDoesNotMoveToplevelPreallocatedRegs) {
  auto* func = createToplevelTestFunction("test_reg_compact_toplevel_prefix");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // One toplevel var: must be preallocated to reg 0.
  auto* toplevel_init = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1), TypeOp::CreateInt32(builder));
  ir_ctx->InsertToplevelValue(toplevel_init, 0);

  // Some non-prefix values.
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                            TypeOp::CreateInt32(builder));
  auto* v2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(3),
                                            TypeOp::CreateInt32(builder));
  auto* add = builder->Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, add);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(toplevel_init));
  ASSERT_EQ(ra->GetRegister(toplevel_init).GetIndex(), 0u);

  // Make non-prefix regs sparse.
  ra->UpdateRegister(v1, Register(10));
  ra->UpdateRegister(v2, Register(20));
  ra->UpdateRegister(add, Register(30));

  RegisterCompactionPass compact(ir_ctx.get());
  compact.RunOnFunction(func);

  // Prefix reg (toplevel var) stays at 0.
  EXPECT_EQ(ra->GetRegister(toplevel_init).GetIndex(), 0u);

  // Non-prefix regs become dense starting from prefix=1.
  const unsigned prefix =
      static_cast<unsigned>(ir_ctx->GetToplevelVariables().size());
  EXPECT_EQ(prefix, 1u);
  llvh::SmallDenseSet<unsigned, 32> used;
  unsigned max_used = 0;
  for (const auto& kv : ra->GetAllocatedMap()) {
    if (!kv.second.IsValid()) continue;
    used.insert(kv.second.GetIndex());
    max_used = std::max(max_used, kv.second.GetIndex());
  }
  for (unsigned r = prefix; r <= max_used; ++r) {
    EXPECT_TRUE(used.count(r));
  }
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       DoNotEliminateCallFuncMovIfItViolatesOtherCallConstraint) {
  auto* func = createTestFunction("test_conservative_call_constraint");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Define src (will be R0)
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));

  // 2. Define func_B (will be R5)
  auto* func_b =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(1));

  // 3. Call B (uses func_B)
  // src is live here because it is used later by mov.
  ArgList args_b;
  [[maybe_unused]] auto* call_b = builder->Create<CallInst>(0, func_b, args_b);

  // 4. Mov (src -> R10)
  auto* mov = builder->Create<MovInst>(0, src);
  mov->SetCallFuncMov(true);

  // 5. Call A (uses mov as function)
  ArgList args_a;
  auto* call_a = builder->Create<CallInst>(0, mov, args_a);

  builder->Create<ReturnInst>(0, call_a);

  // Run RA to init structures
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);

  // Manually override registers to create the conflict scenario
  ra->UpdateRegister(src, Register(0));
  ra->UpdateRegister(func_b, Register(5));
  ra->UpdateRegister(mov, Register(10));

  // Ensure intervals are correct/extended if needed.
  // RA pass should have calculated basic intervals.
  // src interval: [src_def, mov_use] -> covers Call B.

  // Run MovElimination
  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // Expectation:
  // mov should NOT be eliminated.
  // src should still be R0.

  EXPECT_EQ(ra->GetRegister(src).GetIndex(), 0u);
  bool mov_exists = false;
  for (auto& op : *block) {
    if (&op == mov) mov_exists = true;
  }
  EXPECT_TRUE(mov_exists);
}

TEST_F(LEPUSIRRedundantMovEliminationTest,
       EliminateMovChainsShortcutsThroughSelfMoveAfterRegUpdate) {
  auto* func = createTestFunction("test_eliminate_mov_chains_shortcut");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Build a simple mov chain:
  //   mov1 = mov(src)
  //   mov2 = mov(mov1)
  // After post-RA register updates, mov1 may become a self-move (dst==src_reg).
  // EliminateMovChains should then rewrite mov2 to read src directly so mov1
  // can be removed safely.
  auto* src =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(7));
  auto* mov1 = builder->Create<MovInst>(0, src);
  auto* mov2 = builder->Create<MovInst>(0, mov1);
  builder->Create<ReturnInst>(0, mov2);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(src));
  ASSERT_TRUE(ra->IsAllocated(mov1));
  ASSERT_TRUE(ra->IsAllocated(mov2));

  // Simulate a prior post-RA transform that reassigned `src` into mov1's dst
  // register. This makes mov1 a self-move.
  ra->UpdateRegister(src, Register(10));
  ra->UpdateRegister(mov1, Register(10));
  ra->UpdateRegister(mov2, Register(11));

  MovEliminationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // mov2 should now read src directly.
  EXPECT_EQ(mov2->GetSingleOperand(), src);

  // mov1 should be eliminated.
  bool mov1_exists = false;
  for (auto& op : *block) {
    if (&op == mov1) {
      mov1_exists = true;
      break;
    }
  }
  EXPECT_FALSE(mov1_exists);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
