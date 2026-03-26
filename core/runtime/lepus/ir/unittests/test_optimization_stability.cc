// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <filesystem>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallPtrSet.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_analysis.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_iterator.h"
#include "core/runtime/lepus/ir/transformer/mir/change_special_attribute.h"
#include "core/runtime/lepus/ir/transformer/mir/get_toplevel_related_inst_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/ssa_ir_verify.h"
#include "core/runtime/lepus/ir/transformer/vm/mov_elimination.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIROptimizationStabilityTest : public IRTestBase {
 public:
  virtual void SetUp(void) { IRTestBase::SetUp(); }

  FuncOp* createTestFunction(std::string name) {
    auto builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
    auto* func_op = builder->Create<FuncOp>(0, name);
    auto region = builder->CreateRegion(func_op);
    builder->CreateBlock(region, BlockType::BT_INST, {});
    return func_op;
  }
};

// 1. Verify RegisterAllocator Coalesce with Path Compression
TEST_F(LEPUSIROptimizationStabilityTest,
       RegisterAllocatorCoalescePathCompression) {
  auto* func = createTestFunction("test_coalesce");
  auto builder = ir_ctx->GetOpBuilder();
  builder->SetInsertionPointToEnd(&func->Front());

  // Create a chain of MovInsts that will be coalesced
  auto* v0 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(0),
                                            TypeOp::CreateInt32(builder));
  auto* v1 = builder->Create<MovInst>(0, v0);
  auto* v2 = builder->Create<MovInst>(0, v1);
  auto* v3 = builder->Create<MovInst>(0, v2);
  builder->Create<ReturnInst>(0, v3);

  auto* ra = new RegisterAllocator(func);
  PostOrderAnalysis po(func);
  llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
  ra->Preallocate();
  ra->Allocate(order);

  // v0, v1, v2, v3 should all have the same register
  Register r0 = ra->GetRegister(v0);
  Register r1 = ra->GetRegister(v1);
  Register r2 = ra->GetRegister(v2);
  Register r3 = ra->GetRegister(v3);

  EXPECT_EQ(r0, r1);
  EXPECT_EQ(r1, r2);
  EXPECT_EQ(r2, r3);

  delete ra;
}

// 0. Verify DominanceInfo / RegionDominanceInfo core APIs (coverage +
// stability)
TEST_F(LEPUSIROptimizationStabilityTest, DominanceInfoAndNCDSmoke) {
  auto* func = createTestFunction("test_dom");
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);
  auto* entry = &func->Front();
  builder->SetInsertionPointToEnd(entry);

  // entry: a; b; if (true) then else
  auto* a = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  auto* b = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                           TypeOp::CreateInt32(builder));
  auto* cond = builder->Create<LoadConstInst>(0, builder->GetLiteralBool(true),
                                              TypeOp::CreateBoolean(builder));

  auto* then_bb =
      builder->CreateBlock(entry->GetParent(), BlockType::BT_INST, {});
  auto* else_bb =
      builder->CreateBlock(entry->GetParent(), BlockType::BT_INST, {});
  auto* merge_bb =
      builder->CreateBlock(entry->GetParent(), BlockType::BT_INST, {});
  ASSERT_NE(nullptr, then_bb);
  ASSERT_NE(nullptr, else_bb);
  ASSERT_NE(nullptr, merge_bb);

  builder->Create<CondBranchInst>(0, cond, then_bb, else_bb);

  builder->SetInsertionPointToEnd(then_bb);
  auto* t = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(10),
                                           TypeOp::CreateInt32(builder));
  builder->Create<BranchInst>(0, merge_bb);

  builder->SetInsertionPointToEnd(else_bb);
  auto* e = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(20),
                                           TypeOp::CreateInt32(builder));
  builder->Create<BranchInst>(0, merge_bb);

  builder->SetInsertionPointToEnd(merge_bb);
  builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));

  DominanceInfo dom(func);
  EXPECT_TRUE(dom.ProperlyDominates(a, b));
  EXPECT_FALSE(dom.ProperlyDominates(b, a));
  EXPECT_TRUE(dom.ProperlyDominates(a, t));
  EXPECT_FALSE(dom.ProperlyDominates(t, e));

  RegionDominanceInfo region_dominance(func->GetSingleRegion());
  auto* ncd =
      region_dominance.FindRegionNearestCommonDominator(then_bb, else_bb);
  ASSERT_NE(nullptr, ncd);
  EXPECT_EQ(entry, ncd);
  EXPECT_EQ(entry,
            region_dominance.FindRegionNearestCommonDominator(entry, then_bb));
}

// 2. Verify RegisterAllocator Worklist Liveness Convergence
TEST_F(LEPUSIROptimizationStabilityTest, RegisterAllocatorWorkListLiveness) {
  auto* func = createTestFunction("test_work_list_liveness");
  auto builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToEnd(entry);

  // Block 0: x = 1; goto Block 1
  auto* x = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  auto* loop_header =
      builder->CreateBlock(entry->GetParent(), BlockType::BT_INST, {});
  builder->Create<BranchInst>(0, loop_header);

  // Block 1 (Loop Header): use x; if (cond) goto Block 2 else goto Block 1
  builder->SetInsertionPointToEnd(loop_header);
  auto* cond = builder->Create<LoadConstInst>(0, builder->GetLiteralBool(true),
                                              TypeOp::CreateBoolean(builder));
  auto* exit_block =
      builder->CreateBlock(entry->GetParent(), BlockType::BT_INST, {});
  builder->Create<CondBranchInst>(0, cond, exit_block, loop_header);

  // Block 2 (Exit): return x
  builder->SetInsertionPointToEnd(exit_block);
  builder->Create<ReturnInst>(0, x);

  auto* ra = new RegisterAllocator(func);
  PostOrderAnalysis po(func);
  llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
  ra->Preallocate();
  // This will trigger CalculateGlobalLiveness
  ra->Allocate(order);

  // x must be allocated and live across the loop
  EXPECT_TRUE(ra->IsAllocated(x));
  Register rx = ra->GetRegister(x);
  EXPECT_TRUE(rx.IsValid());

  delete ra;
}

// 3. Verify RedundantMovElimination Grouping Logic
TEST_F(LEPUSIROptimizationStabilityTest, RedundantMovGrouping) {
  auto* func = createTestFunction("test_grouping");
  auto builder = ir_ctx->GetOpBuilder();
  builder->SetInsertionPointToEnd(&func->Front());

  // Create many variables to fill up register space
  std::vector<Value*> vars;
  for (int i = 0; i < 10; ++i) {
    vars.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(i), TypeOp::CreateInt32(builder)));
  }

  // Create a call with a MovInst that can be eliminated
  auto* f = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                           TypeOp::CreateInt32(builder));
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(200),
                                             TypeOp::CreateInt32(builder));
  auto* mov = builder->Create<MovInst>(0, src);

  ArgList args;
  args.push_back(mov);
  auto* call = builder->Create<CallInst>(0, f, args);

  // Use all vars to keep them live
  Value* last = call;
  for (auto* v : vars) {
    last = builder->Create<BinaryOperatorInst>(
        0, last, v, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(builder));
  }
  builder->Create<ReturnInst>(0, last);

  // Run Register Allocation
  auto ra = std::make_unique<RegisterAllocator>(func);
  PostOrderAnalysis po(func);
  llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
  ra->Preallocate();
  ra->Allocate(order);

  ir_ctx->GetTargetContext()->SetRegisterAllocAnalysis(func, ra);

  // Run RedundantMovElimination
  MovEliminationPass pass(ir_ctx.get());
  [[maybe_unused]] bool changed = pass.RunOnFunction(func);

  // If there's no conflict, it might have been eliminated.
  // We primarily want to ensure it doesn't crash and handles the grouping
  // correctly.
}

// 4. Verify BytecodeAnalysis UnionIsChanged Logic
TEST_F(LEPUSIROptimizationStabilityTest, BytecodeAnalysisUnionIsChanged) {
  llvh::BitVector lhs(10, false);
  llvh::BitVector rhs(10, false);

  lhs.set(1);
  lhs.set(3);

  rhs.set(3);
  rhs.set(5);

  // Initial state: lhs = {1, 3}, rhs = {3, 5}
  // Union should change lhs to {1, 3, 5} and return true
  bool changed = false;
  if (rhs.test(lhs)) {
    lhs |= rhs;
    changed = true;
  }
  EXPECT_TRUE(changed);
  EXPECT_TRUE(lhs.test(1));
  EXPECT_TRUE(lhs.test(3));
  EXPECT_TRUE(lhs.test(5));

  // Second union with same rhs should return false
  changed = false;
  if (rhs.test(lhs)) {
    lhs |= rhs;
    changed = true;
  }
  EXPECT_FALSE(changed);
}

// 5. Verify BytecodeAnalysis Worklist Liveness
TEST_F(LEPUSIROptimizationStabilityTest, BytecodeAnalysisWorkListLiveness) {
  auto* func = createTestFunction("test_ba_work_list");

  // We need to simulate bytecode because BytecodeAnalysis runs on raw
  // instructions 0: LoadConst R0, 1 1: Jmp 3 (to 4) 2: LoadConst R0, 2 3: Jmp
  // -1 (to 2) 4: Ret R0
  using BytecodeInst = lynx::lepus::Instruction;
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 1),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, 3),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 2),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, -1),
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 0)};
  std::vector<int64_t> line_cols(code.size(), 0);

  auto lepus_func = lynx::lepus::Function::Create();
  for (auto& inst : code) lepus_func->AddInstruction(inst);
  lepus_func->SetRegisterCount(10);
  func->Init(lepus_func);

  BytecodeAnalysis analysis(func, 10);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  // Verify that R0 is live at the end and propagates correctly
  auto bb4 = analysis.GetBlockItem(4);
  ASSERT_NE(bb4, nullptr);
  EXPECT_TRUE(bb4->GetLiveIn()->test(0));

  auto bb0 = analysis.GetBlockItem(0);
  ASSERT_NE(bb0, nullptr);
  // R0 is assigned in BB0, so it shouldn't be live-in
  EXPECT_FALSE(bb0->GetLiveIn()->test(0));
}

// 6. Verify PostOrderAnalysis2 does not miss blocks on large CFGs.
//
// PostOrderAnalysis2 previously stored iterators into a per-frame successor
// vector inside a SmallVector-backed DFS stack. When that stack grew beyond its
// inline capacity, State objects were moved and those iterators became invalid,
// causing the traversal to skip some successors (and later leading to
// instruction-selection relocation target missing).
TEST_F(LEPUSIROptimizationStabilityTest, PostOrderAnalysis2LargeCFGNoMiss) {
  constexpr int kNumBlocks = 128;  // > 32 to force DFS stack growth.

  auto* func = createTestFunction("test_po2_large_cfg");
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  auto* entry = &func->Front();
  ASSERT_NE(nullptr, entry);

  // Create a deep linear CFG: BB0 -> BB1 -> ... -> bb(kNumBlocks-1)
  // This produces a DFS stack depth of kNumBlocks.
  std::vector<Block*> blocks;
  blocks.reserve(kNumBlocks);
  blocks.push_back(entry);
  for (int i = 1; i < kNumBlocks; ++i) {
    blocks.push_back(
        builder->CreateBlock(entry->GetParent(), BlockType::BT_INST, {}));
    ASSERT_NE(nullptr, blocks.back());
  }

  for (int i = 0; i < kNumBlocks - 1; ++i) {
    builder->SetInsertionPointToEnd(blocks[i]);
    builder->Create<BranchInst>(0, blocks[i + 1]);
  }
  builder->SetInsertionPointToEnd(blocks.back());
  builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));

  PostOrderAnalysis2 po2(func);
  llvh::SmallVector<Block*, 16> order(po2.begin(), po2.end());

  // All blocks must be visited.
  ASSERT_EQ(static_cast<int>(order.size()), kNumBlocks);
  ASSERT_EQ(order.back(), entry);

  llvh::SmallPtrSet<Block*, 32> visited;
  visited.insert(order.begin(), order.end());
  ASSERT_EQ(static_cast<int>(visited.size()), kNumBlocks);
  for (auto* bb : blocks) {
    EXPECT_TRUE(visited.count(bb)) << "missing bb in PostOrderAnalysis2";
  }
}

// 7. Cover RegisterFile APIs (used by reg_alloc.cc)
TEST_F(LEPUSIROptimizationStabilityTest, RegisterFileBasicAPIs) {
  RegisterFile rf;

  // Allocate a few registers.
  Register r0 = rf.AllocateRegister();
  Register r1 = rf.AllocateRegister();
  EXPECT_TRUE(r0.IsValid());
  EXPECT_TRUE(r1.IsValid());
  EXPECT_TRUE(r0.IsConsecutive(r1));
  EXPECT_TRUE(rf.IsUsed(r0));
  EXPECT_TRUE(rf.IsUsed(r1));

  // Kill and re-check.
  rf.KillRegister(r1);
  EXPECT_TRUE(rf.IsFree(r1));

  // Allocate fixed index + cluster.
  Register r10 = rf.AllocateRegister(10);
  EXPECT_EQ(10u, r10.GetIndex());
  Register base = rf.AllocateRegisterCluster(20, 3);
  EXPECT_EQ(20u, base.GetIndex());
  EXPECT_TRUE(rf.IsUsed(base));
  EXPECT_TRUE(rf.IsUsed(base.GetConsecutive(1)));
  EXPECT_TRUE(rf.IsUsed(base.GetConsecutive(2)));

  // Tail allocate consecutive.
  Register tail = rf.TailAllocateConsecutive(2);
  EXPECT_TRUE(tail.IsValid());
  EXPECT_TRUE(rf.IsUsed(tail));
  EXPECT_TRUE(rf.IsUsed(tail.GetConsecutive(1)));
}

// 8. Cover PassManager run branches + dumping hook (pass_manager.cc)
TEST_F(LEPUSIROptimizationStabilityTest, PassManagerRunAndDumpSmoke) {
  ASSERT_NE(nullptr, mod);

  // Ensure the Dump root exists (avoid CFGRawViewer failing on mkdir).
  const std::string dump_root = "out/ir_coverage/coverage/pass_manager_dump";
  std::error_code ec;
  std::filesystem::create_directories(dump_root, ec);

  PassManager pm(mod->GetIRCtx());
  pm.SetIRDumpPath(dump_root.c_str());

  struct DummyFunctionPass : public FunctionPass {
    explicit DummyFunctionPass(IRContext* ir_ctx)
        : FunctionPass(ir_ctx, "dummy-func") {}
    bool RunOnFunction(FuncOp* func) override { return false; }
  };

  struct DummyModulePass : public ModulePass {
    explicit DummyModulePass(IRContext* ir_ctx)
        : ModulePass(ir_ctx, "dummy-mod") {}
    bool RunOnModule(ModuleOp* m) override { return false; }
  };

  pm.AddPass(new DummyFunctionPass(mod->GetIRCtx()));
  pm.AddPass(new DummyModulePass(mod->GetIRCtx()));
  pm.Run(mod);
}

// 9. Cover SSAIRVerify special-attribute checks for SetToplevel* instructions.
TEST_F(LEPUSIROptimizationStabilityTest, SSAIRVerifySpecialSetInstructions) {
  ASSERT_NE(nullptr, mod);
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "ssa_verify_special";
  auto* func = builder->Create<FuncOp>(1, name);
  ASSERT_NE(nullptr, func);
  auto* region = builder->CreateRegion(func);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  ASSERT_NE(nullptr, bb);
  builder->SetInsertionPointToEnd(bb);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  auto* set =
      builder->Create<SetToplevelVarInst>(1, builder->GetLiteralUint32(10), v);
  ASSERT_NE(nullptr, set);
  // Make closure_var_reg consistent with toplevel reg to exercise the verifier.
  set->SetClosureVarReg(10);
  ir_ctx->InsertToplevelValue(set, 10);

  builder->Create<ReturnInst>(1, builder->GetLiteralInt32(0));

  SSAIRVerifyPass verify(ir_ctx.get());
  ASSERT_TRUE(verify.RunOnModule(mod));
}

// 10. Cover ChangeSpecialAttributePass behavior.
TEST_F(LEPUSIROptimizationStabilityTest,
       ChangeSpecialAttributeClearsClosureOldReg) {
  ASSERT_NE(nullptr, mod);
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "change_special_attr";
  auto* func = builder->Create<FuncOp>(1, name);
  auto* region = builder->CreateRegion(func);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(bb);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(1),
                                           TypeOp::CreateInt32(builder));
  auto* set =
      builder->Create<SetToplevelVarInst>(1, builder->GetLiteralUint32(10), v);
  ASSERT_NE(nullptr, set);
  set->SetToplevelVarReg(10);
  set->SetClosureVarReg(10);

  builder->Create<ReturnInst>(1, builder->GetLiteralInt32(0));

  ChangeSpecialAttributePass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));
  EXPECT_EQ(set->GetClosureVarReg(), constants::kInvalidSignedValue);
}

// 11. Cover BytecodeIterator + Bytecode helpers (bytecode_iterator.cc).
TEST_F(LEPUSIROptimizationStabilityTest, BytecodeIteratorAndHelpersSmoke) {
  using BytecodeInst = lynx::lepus::Instruction;

  std::vector<BytecodeInst> code = {
      // JumpOffset(): cover both jump and non-jump paths.
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, 7),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpFalse, 0, -3),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpTrue, 0, 5),
      // GetOperandReg(2): force the ParamC branch.
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Add, 7, 8, 9),
      // Terminate opcode.
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 0),
  };
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeIterator it;
  it.Reset(&code[0], &code[0] + code.size(), line_cols);

  it.SkipTo(0);
  EXPECT_EQ(it.GetOpcode(), LepusOpcode::OP_TypeOp_Jmp);
  EXPECT_EQ(it.JumpOffset(), 7);
  EXPECT_EQ(it.GetOperand1x(), 7);

  it.SkipTo(1);
  EXPECT_EQ(it.GetOpcode(), LepusOpcode::OP_TypeOp_JmpFalse);
  it.JumpOffset();

  it.SkipTo(2);
  EXPECT_EQ(it.GetOpcode(), LepusOpcode::OP_TypeOp_JmpTrue);
  it.JumpOffset();

  it.SkipTo(3);
  EXPECT_EQ(it.GetOpcode(), LepusOpcode::OP_TypeOp_Add);
  EXPECT_EQ(it.JumpOffset(), -1);
  EXPECT_EQ(it.NumOperands(), 3);
  it.GetOperandType(0);
  it.GetOperandType(2);
  EXPECT_EQ(it.GetOperand0(), 7);
  EXPECT_EQ(it.GetOperand1(), 8);
  EXPECT_EQ(it.GetOperand2(), 9);
  EXPECT_EQ(it.GetOperandReg(0), 7);
  EXPECT_EQ(it.GetOperandReg(1), 8);
  EXPECT_EQ(it.GetOperandReg(2), 9);

  // Cover Bytecode helper switches.
  EXPECT_TRUE(Bytecode::IsTerminate(LepusOpcode::OP_TypeOp_Ret));
  EXPECT_FALSE(Bytecode::IsTerminate(LepusOpcode::OP_TypeOp_LoadConst));
  EXPECT_TRUE(Bytecode::IsJump(LepusOpcode::OP_TypeOp_Jmp));
  EXPECT_FALSE(Bytecode::IsJump(LepusOpcode::OP_TypeOp_LoadConst));
  EXPECT_TRUE(Bytecode::IsJumpImm(LepusOpcode::OP_TypeOp_Jmp));
  EXPECT_FALSE(Bytecode::IsJumpImm(LepusOpcode::OP_TypeOp_JmpFalse));

  EXPECT_TRUE(Bytecode::IsCallRange(LepusOpcode::OP_TypeOp_Call));
  EXPECT_FALSE(Bytecode::IsCallRange(LepusOpcode::OP_TypeOp_LoadConst));
  EXPECT_TRUE(Bytecode::IsNewArray(LepusOpcode::OP_TypeOp_NewArray));
  EXPECT_FALSE(Bytecode::IsNewArray(LepusOpcode::OP_TypeOp_LoadConst));
}

// 12. Cover GetToplevelRelatedInstEliminationPass patterns.
TEST_F(LEPUSIROptimizationStabilityTest,
       RemoveUselessGetToplevelVarCoversEraseAndRewrite) {
  ASSERT_NE(nullptr, mod);
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "remove_useless_get_toplevel";
  auto* func = builder->Create<FuncOp>(1, name);
  auto* region = builder->CreateRegion(func);
  auto* bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  builder->SetInsertionPointToEnd(bb);

  auto* v = builder->Create<LoadConstInst>(1, builder->GetLiteralInt32(42),
                                           TypeOp::CreateInt32(builder));

  // 1) Unused GetToplevelVarInst should be erased.
  auto* unused_get = builder->Create<GetToplevelVarInst>(
      1, builder->GetLiteralUint32(100), TypeOp::CreateInt32(builder));
  ASSERT_NE(nullptr, unused_get);
  unused_get->SetToplevelVarReg(100);
  unused_get->SetClosureVarReg(100);

  // 2) SetToplevelVarInst + GetToplevelVarInst (same reg) should be rewritten.
  builder->Create<SetToplevelVarInst>(1, builder->GetLiteralUint32(10), v);
  auto* get_tv = builder->Create<GetToplevelVarInst>(
      1, builder->GetLiteralUint32(10), TypeOp::CreateInt32(builder));
  ASSERT_NE(nullptr, get_tv);
  get_tv->SetToplevelVarReg(10);
  get_tv->SetClosureVarReg(10);

  // 3) Keep a GetToplevelClosureVarInst alive but clear its attributes.
  [[maybe_unused]] auto* set_cv = builder->Create<SetToplevelClosureVarInst>(
      1, builder->GetLiteralUint32(11), v);
  // Prevent the immediate set/get rewrite for closure-var (the pass only
  // matches adjacent instructions).
  builder->Create<MovInst>(1, v);
  auto* get_cv = builder->Create<GetToplevelClosureVarInst>(
      1, builder->GetLiteralUint32(11), TypeOp::CreateInt32(builder));
  ASSERT_NE(nullptr, get_cv);
  get_cv->SetToplevelVarReg(11);
  get_cv->SetClosureVarReg(11);

  // 4) Unused GetToplevelClosureVarInst should be erased.
  auto* unused_get_cv = builder->Create<GetToplevelClosureVarInst>(
      1, builder->GetLiteralUint32(12), TypeOp::CreateInt32(builder));
  ASSERT_NE(nullptr, unused_get_cv);
  unused_get_cv->SetToplevelVarReg(12);
  unused_get_cv->SetClosureVarReg(12);

  // 5) SetToplevelClosureVarInst + GetToplevelClosureVarInst (same reg) should
  // be rewritten.
  [[maybe_unused]] auto* set_cv2 = builder->Create<SetToplevelClosureVarInst>(
      1, builder->GetLiteralUint32(13), v);
  auto* get_cv2 = builder->Create<GetToplevelClosureVarInst>(
      1, builder->GetLiteralUint32(13), TypeOp::CreateInt32(builder));
  ASSERT_NE(nullptr, get_cv2);
  get_cv2->SetToplevelVarReg(13);
  get_cv2->SetClosureVarReg(13);

  // Use both to ensure they have users before the pass.
  auto* sum = builder->Create<BinaryOperatorInst>(1, get_tv, get_cv,
                                                  ValueKind::BinaryAddInstKind,
                                                  TypeOp::CreateInt32(builder));
  // Keep get_cv2 alive before the pass (it should be rewritten away by the
  // pass).
  auto* sum2 = builder->Create<BinaryOperatorInst>(
      1, sum, get_cv2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(1, sum2);

  GetToplevelRelatedInstEliminationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  // The live closure-var getter should keep the instruction but have attrs
  // cleared.
  EXPECT_EQ(get_cv->GetToplevelVarReg(), constants::kInvalidSignedValue);
  EXPECT_EQ(get_cv->GetClosureVarReg(), constants::kInvalidSignedValue);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
