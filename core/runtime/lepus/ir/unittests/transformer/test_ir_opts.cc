// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <limits>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/transformer/mir/construct_ssa_ir.h"
#include "core/runtime/lepus/ir/transformer/mir/cse.h"
#include "core/runtime/lepus/ir/transformer/mir/dce.h"
#include "core/runtime/lepus/ir/transformer/mir/inst_combine/inst_combine.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/normalize_phi.h"
#include "core/runtime/lepus/ir/transformer/mir/process_special_mov.h"
#include "core/runtime/lepus/ir/transformer/mir/simplify_cfg.h"
#include "core/runtime/lepus/ir/transformer/mir/type_specification.h"
#include "core/runtime/lepus/ir/transformer/vm/instruction_selection.h"
#include "core/runtime/lepus/ir/transformer/vm/mov_elimination.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestIROpts : public IRTestBase {
 public:
  virtual void SetUp(void) { IRTestBase::SetUp(); }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestIROpts, DCEBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  auto* v1 = builder.GetLiteralInt32(10);
  auto* v2 = builder.GetLiteralInt32(20);
  auto* add = builder.Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(&builder));  // Dead

  DCE pass(ir_ctx.get());
  pass.RunOnModule(mod);

  bool found_add = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == add) found_add = true;
  }
  EXPECT_FALSE(found_add);
}

TEST_F(LEPUSIRTestIROpts, DCETableArray) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  auto* table = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(100);
  auto* val = builder.GetLiteralInt32(10);
  auto* set = builder.Create<SetTableInst>(0, table, key, val);

  DCE pass(ir_ctx.get());
  pass.RunOnModule(mod);

  bool found_table = false;
  bool found_set = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == table) found_table = true;
    if (inst == set) found_set = true;
  }
  EXPECT_FALSE(found_table);
  EXPECT_FALSE(found_set);
}

TEST_F(LEPUSIRTestIROpts, CSEBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  auto* v1 = builder.GetLiteralInt32(10);
  auto* v2 = builder.GetLiteralInt32(20);
  auto* add1 = builder.Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(&builder));
  auto* add2 = builder.Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(&builder));
  builder.Create<BinaryOperatorInst>(0, add1, add2,
                                     ValueKind::BinaryAddInstKind,
                                     TypeOp::CreateInt32(&builder));

  CSE pass(ir_ctx.get());
  pass.RunOnFunction(func);

  int add_count = 0;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<BinaryOperatorInst>(inst)) {
      add_count++;
    }
  }
  EXPECT_EQ(add_count, 2);
}

TEST_F(LEPUSIRTestIROpts, SimplifyCFGUnreachable) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  auto* unreachable = builder.CreateBlock(func->GetSingleRegion(),
                                          BlockType::BT_INST, 0, "unreachable");
  builder.SetInsertionPointToEnd(unreachable);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  EXPECT_EQ(func->GetBlockSize(), 2);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  EXPECT_EQ(func->GetBlockSize(), 1);
}

TEST_F(LEPUSIRTestIROpts, SimplifyCFGConstantCond) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_dest =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_dest =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(true_dest);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));
  builder.SetInsertionPointToStart(false_dest);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  builder.SetInsertionPointToStart(entry);
  auto* cond = builder.GetLiteralBool(true);
  builder.Create<CondBranchInst>(0, cond, true_dest, false_dest);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // entry should now have its content merged with true_dest, and false_dest
  // should be gone. The final block should have the ReturnInst from true_dest.
  EXPECT_EQ(func->GetBlockSize(), 1);
  auto* term = entry->GetTerminator();
  EXPECT_TRUE(llvh::isa<ReturnInst>(term));
}

TEST_F(LEPUSIRTestIROpts, SimplifyCFGMergeBlocks) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* next =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* v1 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateInt32(&builder));
  builder.Create<BranchInst>(0, next);

  builder.SetInsertionPointToStart(next);
  auto* v2 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(1),
                                           TypeOp::CreateInt32(&builder));
  builder.Create<ReturnInst>(0, v2);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // Blocks should be merged into one
  EXPECT_EQ(func->GetBlockSize(), 1);
  bool found_v1 = false;
  bool found_v2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == v1) found_v1 = true;
    if (inst == v2) found_v2 = true;
  }
  EXPECT_TRUE(found_v1);
  EXPECT_TRUE(found_v2);
}

TEST_F(LEPUSIRTestIROpts, SimplifyCFGSameTargets) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* target =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(target);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  builder.SetInsertionPointToStart(entry);
  auto* cond = builder.GetLiteralInt32(10);  // non-constant but same targets
  builder.Create<CondBranchInst>(0, cond, target, target);

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // entry should now have its content merged with target.
  EXPECT_EQ(func->GetBlockSize(), 1);
  auto* term = entry->GetTerminator();
  EXPECT_TRUE(llvh::isa<ReturnInst>(term));
}

TEST_F(LEPUSIRTestIROpts, SimplifyCFGIndirectJumpPattern1) {
  // Pattern from OptIndirectJmp (identifyConvergingBranch)
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb1 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb2 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* target =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* final_true =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* final_false =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond1 = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, cond1, bb1, bb2);

  builder.SetInsertionPointToStart(bb1);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(bb2);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(target);
  PhiInst::ValueListType vals = {builder.GetLiteralBool(true),
                                 builder.GetLiteralBool(false)};
  PhiInst::BlockListType blks = {bb1, bb2};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  builder.Create<CondBranchInst>(0, phi, final_true, final_false);

  builder.SetInsertionPointToStart(final_true);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));
  builder.SetInsertionPointToStart(final_false);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // The optimization should happen.
  // Let's check if the total block size reduced.
  EXPECT_LT(func->GetBlockSize(), 6);

  // After indirect jump optimization, the entry block's condition should NOT be
  // the Phi anymore, or the targets should be the final ones.
  bool found_optimized_branch = false;
  for (auto& block : *func) {
    if (auto* cbr = llvh::dyn_cast<CondBranchInst>(block.GetTerminator())) {
      // If indirect jump optimization worked, we should have a CondBranch
      // that doesn't point to 'target' (which had the Phi).
      if (cbr->GetTrueDest() == final_true ||
          cbr->GetFalseDest() == final_false) {
        found_optimized_branch = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found_optimized_branch);
}

TEST_F(LEPUSIRTestIROpts, SimplifyCFGIndirectJumpPattern2) {
  // Pattern from OptIndirectJmp (identifySequentialBranch)
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb3 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb4 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb5 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* cond = func->CreateParam(0);
  builder.Create<CondBranchInst>(0, cond, bb3, bb4);

  builder.SetInsertionPointToStart(bb3);
  builder.Create<BranchInst>(0, bb4);

  builder.SetInsertionPointToStart(bb4);
  PhiInst::ValueListType vals = {builder.GetLiteralBool(false),
                                 builder.GetLiteralBool(true)};
  PhiInst::BlockListType blks = {bb3, entry};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  builder.Create<CondBranchInst>(0, phi, bb5, entry);

  builder.SetInsertionPointToStart(bb5);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(1));

  SimplifyCFGPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // The optimization should happen.
  // entry should now point directly to bb5 and its old self (for the false
  // case).
  bool found_optimized_branch = false;
  for (auto& block : *func) {
    if (auto* cbr = llvh::dyn_cast<CondBranchInst>(block.GetTerminator())) {
      if (cbr->GetTrueDest() == bb5) {
        found_optimized_branch = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found_optimized_branch);
}

TEST_F(LEPUSIRTestIROpts, ConstructSSAPhi) {
  auto lepus_func = lepus::Function::Create();
  lepus_func->SetRegisterCount(2);
  lepus_func->SetParamsSize(1);
  lepus_func->AddConstNumber(1);  // idx 0
  lepus_func->AddConstNumber(2);  // idx 1

  // Bytecode:
  // 0: JMP_FALSE 0, 3  // If param 0 is false, jump to 0+3=3
  // 1: LOAD_CONST 1, 0 // reg 1 = 1
  // 2: JMP 2           // jump to 2+2=4
  // 3: LOAD_CONST 1, 1 // reg 1 = 2
  // 4: RET 1

  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_JmpFalse, 0, (short)3));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_LoadConst, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_Jmp, 0, (short)2));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_LoadConst, 1, (short)1));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_Ret, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::OP_PLACEHOLDER, 0, (short)0));

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_ssa_phi";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  ConstructSSAIRPass ssa_pass(ir_ctx.get());
  ssa_pass.RunOnFunction(func);

  // The final block (with RET) should have a Phi node for reg 1
  Block* ret_block = nullptr;
  for (auto& block : *func) {
    if (llvh::isa<ReturnInst>(block.GetTerminator())) {
      ret_block = &block;
      break;
    }
  }

  ASSERT_NE(ret_block, nullptr);
  bool found_phi = false;
  for (auto& inst : *ret_block) {
    if (llvh::isa<PhiInst>(&inst)) {
      found_phi = true;
      break;
    }
  }
  EXPECT_TRUE(found_phi);
}

TEST_F(LEPUSIRTestIROpts, NormalizePhiComplex) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  Block* entry = &*func->begin();
  Block* bb1 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb2 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* target =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  auto* v1 = builder.GetLiteralInt32(10);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), bb1, bb2);

  builder.SetInsertionPointToStart(bb1);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(bb2);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(target);
  // All same value phi
  PhiInst::ValueListType vals1 = {v1, v1};
  PhiInst::BlockListType blks1 = {bb1, bb2};
  auto* phi1 = builder.Create<PhiInst>(0, vals1, blks1);
  phi1->SetType(TypeOp::CreateInt32(&builder));

  // Self-referencing phi
  PhiInst::ValueListType empty_vals;
  PhiInst::BlockListType empty_blks;
  auto* phi2 = builder.Create<PhiInst>(0, empty_vals, empty_blks);
  phi2->AddEntry(v1, bb1);
  phi2->AddEntry(phi2, bb2);  // Self-referencing
  phi2->SetType(TypeOp::CreateInt32(&builder));

  builder.Create<ReturnInst>(0, phi1);
  builder.Create<ReturnInst>(0, phi2);

  NormalizePhiPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // phi1 should be replaced by v1
  // phi2's second entry should be removed, then it becomes single entry and
  // replaced by v1
  bool found_phi1 = false;
  bool found_phi2 = false;
  for (auto* inst : target->InstRange()) {
    if (inst == phi1) found_phi1 = true;
    if (inst == phi2) found_phi2 = true;
  }
  EXPECT_FALSE(found_phi1);
  EXPECT_FALSE(found_phi2);
}

TEST_F(LEPUSIRTestIROpts, NormalizePhiType) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  Block* entry = &*func->begin();
  Block* bb1 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* bb2 =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* target =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  builder.SetInsertionPointToStart(entry);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), bb1, bb2);

  builder.SetInsertionPointToStart(bb1);
  auto* v1 = builder.GetLiteralInt32(10);
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(bb2);
  auto* v2 = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(0),
                                           TypeOp::CreateString(&builder));
  builder.Create<BranchInst>(0, target);

  builder.SetInsertionPointToStart(target);
  PhiInst::ValueListType vals = {v1, v2};
  PhiInst::BlockListType blks = {bb1, bb2};
  auto* phi = builder.Create<PhiInst>(0, vals, blks);
  builder.Create<ReturnInst>(0, phi);

  NormalizePhiPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // phi should have AnyType because Int32 and String are merged
  EXPECT_TRUE(phi->GetType()->IsAnyType());
}

TEST_F(LEPUSIRTestIROpts, NormalizePhiBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  std::string name = "test";
  FuncOp* func = nullptr;
  CreateTestFuncOp(name, func);
  Block* entry = &*func->begin();
  builder.SetInsertionPointToStart(entry);

  auto* v1 = builder.GetLiteralInt32(10);

  // Single entry phi
  PhiInst::ValueListType values = {v1};
  PhiInst::BlockListType blocks = {entry};
  auto* phi = builder.Create<PhiInst>(0, values, blocks);
  builder.Create<ReturnInst>(0, phi);

  NormalizePhiPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // Phi should be replaced by v1
  bool found_phi = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == phi) found_phi = true;
  }
  EXPECT_FALSE(found_phi);
}

TEST_F(LEPUSIRTestIROpts, ReplaceToplevelClosureMov) {
  // Goal: verify that ProcessSpecialMovPass rewrites a MovInst with
  // ClosureVarReg into Set/GetToplevelClosureVarInst in a toplevel function,
  // and updates the root function's closure-var-reg mapping accordingly.

  auto* builder = ir_ctx->GetOpBuilder();
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder->Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);

  Block* entry =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  // ProcessSpecialMovPass unconditionally reads mov's src->GetToplevelVarReg(),
  // so `src` must be an Instruction here (consistent with the real bytecode
  // builder).
  auto* src = builder->Create<LoadConstInst>(0, builder->GetLiteralUint32(0),
                                             TypeOp::CreateInt32(builder));

  auto* mov = builder->Create<MovInst>(0, src);

  // Explicitly fill in default attribute values to avoid getXXX() crashing when
  // the key is missing in Attributes. Here, -1 will be treated as an unsigned
  // sentinel value (UINT_MAX).
  mov->SetClosureVarReg(5);

  ASSERT_TRUE(mov->HasAttr(SpecificAttr::SA_ToplevelVarReg));
  ASSERT_TRUE(mov->HasAttr(SpecificAttr::SA_ClosureVarReg));
  EXPECT_GE(mov->GetAttrSize(), 4u);
  // Ensure UpdateClosureVar has the corresponding key, and verify the mapping
  // is updated from `mov` to the new value.
  func->RecordClosureVarRegAndValue(5, mov);
  auto* ret = builder->Create<ReturnInst>(0, mov);

  ASSERT_TRUE(ret->HasAttr(SpecificAttr::SA_ToplevelVarReg));
  ASSERT_TRUE(ret->HasAttr(SpecificAttr::SA_ClosureVarReg));

  ProcessSpecialMovPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  int mov_cnt = 0;
  int set_cnt = 0;
  int get_cnt = 0;
  SetToplevelClosureVarInst* set_inst = nullptr;
  GetToplevelClosureVarInst* get_inst = nullptr;

  for (auto* inst : entry->InstRange()) {
    mov_cnt += llvh::isa<MovInst>(inst);
    if (auto* s = llvh::dyn_cast<SetToplevelClosureVarInst>(inst)) {
      set_cnt++;
      set_inst = s;
    }
    if (auto* g = llvh::dyn_cast<GetToplevelClosureVarInst>(inst)) {
      get_cnt++;
      get_inst = g;
    }
  }

  EXPECT_EQ(mov_cnt, 0);
  ASSERT_EQ(set_cnt, 1);
  ASSERT_EQ(get_cnt, 1);
  ASSERT_NE(set_inst, nullptr);
  ASSERT_NE(get_inst, nullptr);

  auto* set_reg = llvh::cast<LiteralUint32>(set_inst->GetClosureReg());
  auto* get_reg = llvh::cast<LiteralUint32>(get_inst->GetClosureReg());
  EXPECT_EQ(set_reg->GetValue(), 5u);
  EXPECT_EQ(get_reg->GetValue(), 5u);

  // GetToplevelClosureVarInst must preserve ClosureVarReg to ensure later
  // stages always read from the original register.
  EXPECT_EQ(get_inst->GetClosureVarReg(), 5);
  EXPECT_EQ(ret->GetValue(), get_inst);
  EXPECT_EQ(func->GetClosureVarGivenReg(5), get_inst);

  // Non-toplevel functions should not be processed.
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name2 = "test_not_toplevel";
  auto* func2 = builder->Create<FuncOp>(0, name2);
  auto lepus_func2 = lepus::Function::Create();
  func2->Init(lepus_func2);
  Block* entry2 =
      builder->CreateBlock(func2->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry2);
  auto* src2 = builder->Create<LoadConstInst>(0, builder->GetLiteralUint32(0),
                                              TypeOp::CreateInt32(builder));

  auto* mov2 = builder->Create<MovInst>(0, src2);
  mov2->SetClosureVarReg(6);
  [[maybe_unused]] auto* ret2 = builder->Create<ReturnInst>(0, mov2);

  pass.RunOnFunction(func2);
  bool found_mov2 = false;
  for (auto* inst : entry2->InstRange()) {
    if (inst == mov2) {
      found_mov2 = true;
      break;
    }
  }
  EXPECT_TRUE(found_mov2);
}

TEST_F(LEPUSIRTestIROpts, TypeSpecificationStringLength) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Create mock lepus function and add "length" constant
  auto lepus_func = lepus::Function::Create();
  uint32_t length_id = lepus_func->AddConstString(constants::kStringLength);
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* str = builder.GetLiteralInt32(0);  // placeholder for string
  str->SetType(TypeOp::CreateString(&builder));

  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(length_id), TypeOp::CreateString(&builder));
  key->SetType(TypeOp::CreateString(&builder));

  auto* get_table = builder.Create<GetTableInst>(0, str, key);
  builder.Create<ReturnInst>(0, get_table);

  TypeSpecification pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // get_table should be replaced by GetStringLengthInst
  bool found_get_table = false;
  bool found_get_length = false;
  for (auto* inst : entry->InstRange()) {
    if (llvh::isa<GetTableInst>(inst)) found_get_table = true;
    if (llvh::isa<GetStringLengthInst>(inst)) found_get_length = true;
  }
  EXPECT_FALSE(found_get_table);
  EXPECT_TRUE(found_get_length);
}

TEST_F(LEPUSIRTestIROpts, TypeSpecificationStringProto) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Create mock lepus function and add "split" constant
  auto lepus_func = lepus::Function::Create();
  uint32_t split_id = lepus_func->AddConstString(constants::kStringSplit);
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* str = builder.GetLiteralInt32(0);  // placeholder for string
  str->SetType(TypeOp::CreateString(&builder));

  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(split_id), TypeOp::CreateString(&builder));
  key->SetType(TypeOp::CreateString(&builder));

  auto* get_table = builder.Create<GetTableInst>(0, str, key);

  // GetTableInst on string should be marked as StringProtoAPI
  TypeSpecification pass(ir_ctx.get());
  pass.RunOnFunction(func);

  EXPECT_TRUE(get_table->GetType()->IsStringProtoAPIType());

  // Now test CallInst on this GetTableInst
  builder.SetInsertionPointAfter(get_table);
  ArgList args = {get_table};
  auto* call = builder.Create<CallInst>(0, get_table, args);
  builder.Create<ReturnInst>(0, call);

  pass.RunOnFunction(func);

  // split should return Array type
  EXPECT_TRUE(call->GetType()->IsArrayType());
}

TEST_F(LEPUSIRTestIROpts, InstCombineCompareJmp) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_dest =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_dest =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* v1 = builder.GetLiteralInt32(10);
  auto* v2 = builder.GetLiteralInt32(20);
  auto* cmp = builder.Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryStrictlyEqualInstKind,
      TypeOp::CreateBoolean(&builder));

  auto* cond_br = builder.Create<CondBranchInst>(0, cmp, true_dest, false_dest);
  cond_br->SetSmallJmp(true);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  // cmp and cond_br should be replaced by EqCondBranchInst
  bool found_cmp = false;
  bool found_cond_br = false;
  bool found_eq_cond_br = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == cmp) found_cmp = true;
    if (inst == cond_br) found_cond_br = true;
    if (llvh::isa<EqCondBranchInst>(inst)) found_eq_cond_br = true;
  }
  EXPECT_FALSE(found_cmp);
  EXPECT_FALSE(found_cond_br);
  EXPECT_TRUE(found_eq_cond_br);
}

TEST_F(LEPUSIRTestIROpts, InstCombineConstantFoldInt64Add) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_const_fold_int64_add";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  lepus::Value v10;
  v10.SetNumber(static_cast<int64_t>(10));
  lepus::Value v20;
  v20.SetNumber(static_cast<int64_t>(20));
  auto idx10 = lepus_func->AddConstValue(v10);
  auto idx20 = lepus_func->AddConstValue(v20);

  auto* c10 = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(static_cast<uint32_t>(idx10)),
      TypeOp::CreateInt64(&builder));
  auto* c20 = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(static_cast<uint32_t>(idx20)),
      TypeOp::CreateInt64(&builder));
  auto* add = builder.Create<BinaryOperatorInst>(
      0, c10, c20, ValueKind::BinaryAddInstKind, TypeOp::CreateInt64(&builder));
  auto* ret = builder.Create<ReturnInst>(0, add);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // BinaryAdd should be folded to a new constant.
  bool found_add = false;
  LoadConstInst* folded = nullptr;
  for (auto* inst : entry->InstRange()) {
    if (auto* bin = llvh::dyn_cast<BinaryOperatorInst>(inst)) {
      if (bin->GetKind() == ValueKind::BinaryAddInstKind) found_add = true;
    }
  }
  EXPECT_FALSE(found_add);

  folded = llvh::dyn_cast<LoadConstInst>(ret->GetValue());
  ASSERT_NE(folded, nullptr);
  auto folded_idx = llvh::cast<LiteralUint32>(folded->GetConst())->GetValue();
  auto* folded_val = lepus_func->GetConstValue(folded_idx);
  ASSERT_TRUE(folded_val->IsInt64());
  EXPECT_EQ(folded_val->Int64(), 30);
}

TEST_F(LEPUSIRTestIROpts, InstCombineNoFoldInt64AddOnOverflow) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_const_no_fold_int64_add_overflow";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  lepus::Value vmax;
  vmax.SetNumber(std::numeric_limits<int64_t>::max());
  lepus::Value value_one;
  value_one.SetNumber(static_cast<int64_t>(1));
  auto idx_max = lepus_func->AddConstValue(vmax);
  auto idx_one = lepus_func->AddConstValue(value_one);

  auto* const_max = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(static_cast<uint32_t>(idx_max)),
      TypeOp::CreateInt64(&builder));
  auto* c1 = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(static_cast<uint32_t>(idx_one)),
      TypeOp::CreateInt64(&builder));
  builder.Create<BinaryOperatorInst>(0, const_max, c1,
                                     ValueKind::BinaryAddInstKind,
                                     TypeOp::CreateInt64(&builder));
  builder.Create<ReturnInst>(0, const_max);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // Overflow is conservatively not folded.
  bool found_add = false;
  for (auto* inst : entry->InstRange()) {
    if (auto* bin = llvh::dyn_cast<BinaryOperatorInst>(inst)) {
      if (bin->GetKind() == ValueKind::BinaryAddInstKind) found_add = true;
    }
  }
  EXPECT_TRUE(found_add);
}

TEST_F(LEPUSIRTestIROpts, InstCombineConstantFoldBitOrMask32ForDouble) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_const_fold_bitor_mask32";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Keep LHS as double so VM uses 32-bit mask semantics for bitwise ops, while
  // keeping RHS as int64 to avoid const-pool de-dup with a double 1.0.
  lepus::Value v2p32;
  v2p32.SetNumber(4294967296.0);  // 2^32
  auto idx2p32 = lepus_func->AddConstValue(v2p32);
  auto idx1 = lepus_func->AddConstNumber(1.0);

  auto* c2p32 = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(static_cast<uint32_t>(idx2p32)),
      TypeOp::CreateFloat64(&builder));
  auto* c1 = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(static_cast<uint32_t>(idx1)),
      TypeOp::CreateFloat64(&builder));
  auto* bitor_inst = builder.Create<BinaryOperatorInst>(
      0, c2p32, c1, ValueKind::BinaryBitOrInstKind,
      TypeOp::CreateInt64(&builder));
  auto* ret = builder.Create<ReturnInst>(0, bitor_inst);

  InstCombinePass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  auto* folded = llvh::dyn_cast<LoadConstInst>(ret->GetValue());
  ASSERT_NE(folded, nullptr);
  auto folded_idx = llvh::cast<LiteralUint32>(folded->GetConst())->GetValue();
  auto* folded_val = lepus_func->GetConstValue(folded_idx);
  ASSERT_TRUE(folded_val->IsInt64());
  // (2^32 | 1) with VM semantics: (0 | 1) == 1.
  EXPECT_EQ(folded_val->Int64(), 1);
}

TEST_F(LEPUSIRTestIROpts, DCEToplevelVarSync) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);
  func->SetTopLevelFunction();

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // 1. Create SetToplevelVarInst. It has side effects (WriteHeap), so DCE
  // should not delete it.
  auto* val = builder.GetLiteralInt32(42);
  auto* set_inst =
      builder.Create<SetToplevelVarInst>(0, builder.GetLiteralUint32(10), val);

  // 2. Create GetToplevelVarInst. It has ReadHeap (not enough for
  // HasSideEffect), but it should have ToplevelVarReg set via
  // IRContext::UpdateToplevelVar in real pipeline. Here we set it manually to
  // test DCE protection.
  auto* get_inst = builder.Create<GetToplevelVarInst>(
      0, builder.GetLiteralUint32(10), TypeOp::CreateAnyType(&builder));
  get_inst->SetToplevelVarReg(10);

  // Neither has users.
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  bool found_set = false;
  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == set_inst) found_set = true;
    if (inst == get_inst) found_get = true;
  }
  EXPECT_TRUE(found_set);
  EXPECT_TRUE(found_get);
}

TEST_F(LEPUSIRTestIROpts, DCERespectsInvalidSignedValueSentinelForAttrs) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_dce_attr_sentinel";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Dead instruction with attrs cleared by sentinel: should be removable.
  auto* dead_clear = builder.Create<NopInst>(0);
  dead_clear->SetToplevelVarReg(constants::kInvalidSignedValue);
  dead_clear->SetClosureVarReg(constants::kInvalidSignedValue);

  // Dead instruction with a real special attr: should NOT be removed by DCE.
  auto* dead_special = builder.Create<NopInst>(0);
  dead_special->SetClosureVarReg(0);

  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  bool found_clear = false;
  bool found_special = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == dead_clear) found_clear = true;
    if (inst == dead_special) found_special = true;
  }
  EXPECT_FALSE(found_clear);
  EXPECT_TRUE(found_special);
}

TEST_F(LEPUSIRTestIROpts, LoadStoreEliminationBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* global_name =
      builder.GetLiteralUint32(0);  // placeholder for global name index

  auto* load1 = builder.Create<GetGlobalInst>(0, global_name,
                                              TypeOp::CreateAnyType(&builder));
  auto* load2 = builder.Create<GetGlobalInst>(0, global_name,
                                              TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, load2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // load2 should be replaced by load1, and load2 should be removed
  bool found_load1 = false;
  bool found_load2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == load1) found_load1 = true;
    if (inst == load2) found_load2 = true;
  }
  EXPECT_TRUE(found_load1);
  EXPECT_FALSE(found_load2);
}

TEST_F(LEPUSIRTestIROpts, LoadStoreEliminationTableAliasing) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key1 = builder.GetLiteralUint32(0);
  auto* key2 = builder.GetLiteralUint32(1);
  auto* val = builder.GetLiteralInt32(42);

  auto* load1 = builder.Create<GetTableInst>(0, obj, key1);
  auto* load2 = builder.Create<GetTableInst>(0, obj, key2);

  // SetTable with key1 should ONLY invalidate load1, not load2
  builder.Create<SetTableInst>(0, obj, key1, val);

  auto* load1_again = builder.Create<GetTableInst>(0, obj, key1);
  auto* load2_again = builder.Create<GetTableInst>(0, obj, key2);

  builder.Create<ReturnInst>(0, load1_again);
  builder.Create<ReturnInst>(0, load2_again);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // load2_again should be replaced by load2
  // load1_again should NOT be replaced by load1 (it should be replaced by 'val'
  // because of Store-to-Load forwarding)

  bool found_load1 = false;
  bool found_load2 = false;
  bool found_load2_again = false;
  bool found_load1_again = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == load1) found_load1 = true;
    if (inst == load2) found_load2 = true;
    if (inst == load2_again) found_load2_again = true;
    if (inst == load1_again) found_load1_again = true;
  }
  EXPECT_TRUE(found_load1);
  EXPECT_TRUE(found_load2);
  EXPECT_FALSE(found_load2_again);
  EXPECT_FALSE(found_load1_again);
}

TEST_F(LEPUSIRTestIROpts, LoadStoreEliminationNonConstantKey) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* key_const = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateString(&builder));
  auto* key_dynamic = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(1),
      TypeOp::CreateAnyType(&builder));  // truly dynamic key
  auto* val = builder.GetLiteralInt32(42);

  auto* load_const = builder.Create<GetTableInst>(0, obj, key_const);

  // SetTable with dynamic key should invalidate ALL table entries for safety
  builder.Create<SetTableInst>(0, obj, key_dynamic, val);

  auto* load_const_again = builder.Create<GetTableInst>(0, obj, key_const);
  builder.Create<ReturnInst>(0, load_const_again);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // load_const_again should NOT be replaced because of dynamic key write
  bool found_load_const = false;
  bool found_load_const_again = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == load_const) found_load_const = true;
    if (inst == load_const_again) found_load_const_again = true;
  }
  EXPECT_TRUE(found_load_const);
  EXPECT_TRUE(found_load_const_again);
}

TEST_F(LEPUSIRTestIROpts, RegisterAllocationBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* v1_const = builder.GetLiteralUint32(0);
  auto* v1 =
      builder.Create<LoadConstInst>(0, v1_const, TypeOp::CreateInt32(&builder));

  auto* v2_const = builder.GetLiteralUint32(1);
  auto* v2 =
      builder.Create<LoadConstInst>(0, v2_const, TypeOp::CreateInt32(&builder));

  auto* add = builder.Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(&builder));
  builder.Create<ReturnInst>(0, add);

  RegisterAllocationPass pass(ir_ctx.get());
  pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);

  // Check if instructions have registers
  EXPECT_TRUE(ra->GetRegister(v1).IsValid());
  EXPECT_TRUE(ra->GetRegister(v2).IsValid());
  EXPECT_TRUE(ra->GetRegister(add).IsValid());
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* v1_const = builder.GetLiteralUint32(0);
  auto* v1 =
      builder.Create<LoadConstInst>(0, v1_const, TypeOp::CreateInt32(&builder));

  auto* v2_const = builder.GetLiteralUint32(1);
  auto* v2 =
      builder.Create<LoadConstInst>(0, v2_const, TypeOp::CreateInt32(&builder));

  auto* add = builder.Create<BinaryOperatorInst>(
      0, v1, v2, ValueKind::BinaryAddInstKind, TypeOp::CreateInt32(&builder));
  builder.Create<ReturnInst>(0, add);

  // Register Allocation is required for Instruction Selection
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  // Check if lepus_function has opcodes
  EXPECT_GT(lepus_func->OpCodeSize(), 0);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionFoldLoadConstUnaryNeg) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_fold_load_const_neg";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  uint32_t one_idx = lepus_func->AddConstNumber(1);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* one = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(one_idx), TypeOp::CreateInt64(&builder));
  auto* neg =
      builder.Create<UnaryOperatorInst>(0, one, ValueKind::UnaryNegInstKind);
  builder.Create<ReturnInst>(0, neg);

  // Fold happens in inst-combine (not in instruction selection).
  InstCombinePass ic_pass(ir_ctx.get());
  ic_pass.RunOnFunction(func);
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  // Register Allocation is required for Instruction Selection
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  // Expect: LoadConst(-1) + Ret, and no explicit Neg opcode.
  ASSERT_GE(lepus_func->OpCodeSize(), 2u);
  EXPECT_EQ(lepus_func->OpCodeSize(), 2u);

  auto* inst0 = lepus_func->GetInstruction(0);
  auto* inst1 = lepus_func->GetInstruction(1);
  ASSERT_NE(inst0, nullptr);
  ASSERT_NE(inst1, nullptr);

  long op0 = lepus::Instruction::GetOpCode(*inst0);
  long op1 = lepus::Instruction::GetOpCode(*inst1);
  EXPECT_EQ(op0, lepus::TypeOp_LoadConst);
  EXPECT_EQ(op1, lepus::TypeOp_Ret);

  // The folded LoadConst should load -1.
  auto const_idx =
      static_cast<uint32_t>(lepus::Instruction::GetParamBx(*inst0));
  auto* cval = lepus_func->GetConstValue(const_idx);
  ASSERT_NE(cval, nullptr);
  EXPECT_TRUE(cval->IsNumber());
  EXPECT_EQ(cval->Int64(), -1);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionAddAnyString) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_add_any_string";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  // Build: ret (any + string)
  auto* lhs_any = func->CreateParam(0);
  lhs_any->SetType(TypeOp::CreateAnyType(&builder));
  auto* rhs_str = func->CreateParam(1);
  rhs_str->SetType(TypeOp::CreateString(&builder));

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* add = builder.Create<BinaryOperatorInst>(
      0, lhs_any, rhs_str, ValueKind::BinaryAddInstKind,
      TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, add);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  ASSERT_GT(lepus_func->OpCodeSize(), 0u);

  bool found_add_any_string = false;
  bool found_add_string_any = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(static_cast<uint32_t>(i));
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_AddAnyString) {
      found_add_any_string = true;
    }
    if (op == lepus::TypeOp_AddStringAny) {
      found_add_string_any = true;
    }
  }

  EXPECT_TRUE(found_add_any_string);
  EXPECT_FALSE(found_add_string_any);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionCondBranchUnaryNotNoNotBytecode) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_cond_branch_unary_not_no_not_bytecode";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* true_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  Block* false_bb =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});

  uint32_t one_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(1));
  uint32_t zero_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(0));

  builder.SetInsertionPointToStart(true_bb);
  auto* one = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(one_idx), TypeOp::CreateInt64(&builder));
  builder.Create<ReturnInst>(0, one);
  builder.SetInsertionPointToStart(false_bb);
  auto* zero = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(zero_idx), TypeOp::CreateInt64(&builder));
  builder.Create<ReturnInst>(0, zero);

  builder.SetInsertionPointToStart(entry);
  auto* p0 = func->CreateParam(0);
  p0->SetType(TypeOp::CreateAnyType(&builder));
  auto* not1 =
      builder.Create<UnaryOperatorInst>(0, p0, ValueKind::UnaryNotInstKind);
  auto* cb = builder.Create<CondBranchInst>(0, not1, true_bb, false_bb);
  cb->SetSmallJmp(true);

  // Run MIR opts (InstCombine contains the inversion).
  InstCombinePass ic_pass(ir_ctx.get());
  try {
    ic_pass.RunOnFunction(func);
  } catch (const ::lynx::lepus::CompileException& e) {
    FAIL() << e.message();
  } catch (...) {
    FAIL() << "unknown exception";
  }
  DCE dce_pass(ir_ctx.get());
  try {
    dce_pass.RunOnModule(mod);
  } catch (const ::lynx::lepus::CompileException& e) {
    FAIL() << e.message();
  } catch (...) {
    FAIL() << "unknown exception";
  }

  // Sanity: condition must remain encodable by instruction selection.
  if (auto* entry_term = entry->GetTerminator()) {
    if (auto* cbr = llvh::dyn_cast<CondBranchInst>(entry_term)) {
      auto* c = cbr->GetCondition();
      ASSERT_NE(c, nullptr);
      ASSERT_TRUE(llvh::isa<Instruction>(c) || llvh::isa<Parameter>(c))
          << "CondBranch condition kind=" << c->GetKindStr();
    }
  }

  RegisterAllocationPass ra_pass(ir_ctx.get());
  try {
    ra_pass.RunOnFunction(func);
  } catch (const ::lynx::lepus::CompileException& e) {
    FAIL() << e.message();
  } catch (...) {
    FAIL() << "unknown exception";
  }

  InstructionSelectionPass is_pass(ir_ctx.get());
  try {
    is_pass.RunOnFunction(func);
  } catch (const ::lynx::lepus::CompileException& e) {
    FAIL() << e.message();
  } catch (...) {
    FAIL() << "unknown exception";
  }

  bool found_not_opcode = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Not) {
      found_not_opcode = true;
      break;
    }
  }
  EXPECT_FALSE(found_not_opcode);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionUnaryNotEmitsNot2WhenSrcAlive) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_unary_not_emits_not2";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* p0 = func->CreateParam(0);
  p0->SetType(TypeOp::CreateAnyType(&builder));

  // Keep p0 alive after `!p0` by packing both into an array.
  auto* not1 =
      builder.Create<UnaryOperatorInst>(0, p0, ValueKind::UnaryNotInstKind);
  ArgList items = {p0, not1};
  auto* arr = builder.Create<NewArrayInst>(0, items);
  builder.Create<ReturnInst>(0, arr);

  // Keep it simple: instruction selection should emit Not2 when src is alive.
  InstCombinePass ic_pass(ir_ctx.get());
  ic_pass.RunOnFunction(func);
  DCE dce_pass(ir_ctx.get());
  dce_pass.RunOnModule(mod);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_not2 = false;
  int move_count = 0;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Not2) found_not2 = true;
    if (op == lepus::TypeOp_Move) move_count++;
  }

  EXPECT_TRUE(found_not2);
  // Not2 should avoid introducing a Move solely for unary-not.
  EXPECT_EQ(move_count, 0);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionUnaryNegEmitsNeg2WhenSrcAlive) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_unary_neg_emits_neg2";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t ten_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(10));
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* ten = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(ten_idx), TypeOp::CreateInt64(&builder));
  auto* neg =
      builder.Create<UnaryOperatorInst>(0, ten, ValueKind::UnaryNegInstKind);
  // Keep `ten` alive after unary.
  ArgList items = {ten, neg};
  auto* arr = builder.Create<NewArrayInst>(0, items);
  builder.Create<ReturnInst>(0, arr);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_neg2 = false;
  bool found_move_neg = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Neg2) found_neg2 = true;
    if (op == lepus::TypeOp_Neg) found_move_neg = true;
  }
  EXPECT_TRUE(found_neg2);
  EXPECT_FALSE(found_move_neg);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionUnaryNotUsesInplaceWhenDstEqSrc) {
  // This test validates the policy:
  // - If RA assigns dst==src, codegen should keep using the legacy in-place
  //   opcode TypeOp_Not.
  // - Otherwise, it should fall back to TypeOp_Not2.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_unary_not_inplace_when_dst_eq_src";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t one_idx = static_cast<uint32_t>(lepus_func->AddConstBoolean(true));
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);
  auto* one = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(one_idx), TypeOp::CreateBoolean(&builder));
  auto* not1 =
      builder.Create<UnaryOperatorInst>(0, one, ValueKind::UnaryNotInstKind);
  builder.Create<ReturnInst>(0, not1);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  Register src_reg = ra->GetRegister(one);
  Register dst_reg = ra->GetRegister(not1);
  ASSERT_TRUE(src_reg.IsValid());
  ASSERT_TRUE(dst_reg.IsValid());

  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_not = false;
  bool found_not2 = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Not) found_not = true;
    if (op == lepus::TypeOp_Not2) found_not2 = true;
  }

  if (src_reg == dst_reg) {
    EXPECT_TRUE(found_not);
    EXPECT_FALSE(found_not2);
  } else {
    EXPECT_TRUE(found_not2);
  }
}

TEST_F(LEPUSIRTestIROpts,
       InstructionSelectionUnaryBitNotEmitsBitNot2WhenSrcAlive) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_unary_bit_not_emits_bit_not2";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t ten_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(10));
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* ten = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(ten_idx), TypeOp::CreateInt64(&builder));
  auto* bn =
      builder.Create<UnaryOperatorInst>(0, ten, ValueKind::UnaryBitNotInstKind);
  ArgList items = {ten, bn};
  auto* arr = builder.Create<NewArrayInst>(0, items);
  builder.Create<ReturnInst>(0, arr);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_bn2 = false;
  bool found_inplace = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_BitNot2) found_bn2 = true;
    if (op == lepus::TypeOp_BitNot) found_inplace = true;
  }
  EXPECT_TRUE(found_bn2);
  EXPECT_FALSE(found_inplace);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionUnaryPosEmitsPos2WhenSrcAlive) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_unary_pos_emits_pos2";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t ten_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(10));
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* ten = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(ten_idx), TypeOp::CreateInt64(&builder));
  auto* pos =
      builder.Create<UnaryOperatorInst>(0, ten, ValueKind::UnaryPosInstKind);
  ArgList items = {ten, pos};
  auto* arr = builder.Create<NewArrayInst>(0, items);
  builder.Create<ReturnInst>(0, arr);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_pos2 = false;
  bool found_inplace = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Pos2) found_pos2 = true;
    if (op == lepus::TypeOp_Pos) found_inplace = true;
  }
  EXPECT_TRUE(found_pos2);
  EXPECT_FALSE(found_inplace);
}

TEST_F(LEPUSIRTestIROpts,
       InstructionSelectionTypeofEmitsTypeof2WhenSrcDstDifferent) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_typeof_src_dst_different";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);

  uint32_t one_idx = lepus_func->AddConstNumber(1);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* one = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(one_idx), TypeOp::CreateAnyType(&builder));
  auto* typeof_inst =
      builder.Create<UnaryOperatorInst>(0, one, ValueKind::UnaryTypeofInstKind);
  builder.Create<ReturnInst>(0, typeof_inst);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  ASSERT_NE(nullptr, ra);
  ASSERT_TRUE(ra->IsAllocated(one));
  ASSERT_TRUE(ra->IsAllocated(typeof_inst));

  // Force a src/dst mismatch to exercise the typeof-specific non-destructive
  // opcode.
  ra->UpdateRegister(one, Register(1));
  ra->UpdateRegister(typeof_inst, Register(2));

  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_typeof2 = false;
  int move_count = 0;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(static_cast<uint32_t>(i));
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Typeof2) {
      EXPECT_EQ(lepus::Instruction::GetParamA(*inst), 2);
      EXPECT_EQ(lepus::Instruction::GetParamB(*inst), 1);
      found_typeof2 = true;
    }
    if (op == lepus::TypeOp_Move) move_count++;
  }
  EXPECT_TRUE(found_typeof2);
  EXPECT_EQ(move_count, 0);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionUnaryIncEmitsInc2WhenSrcAlive) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_unary_inc_emits_inc2";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t ten_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(10));
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* ten = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(ten_idx), TypeOp::CreateInt64(&builder));
  auto* inc =
      builder.Create<UnaryOperatorInst>(0, ten, ValueKind::UnaryIncInstKind);
  // Keep `ten` alive after unary.
  ArgList items = {ten, inc};
  auto* arr = builder.Create<NewArrayInst>(0, items);
  builder.Create<ReturnInst>(0, arr);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_inc2 = false;
  bool found_inplace = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Inc2) found_inc2 = true;
    if (op == lepus::TypeOp_Inc) found_inplace = true;
  }
  EXPECT_TRUE(found_inc2);
  EXPECT_FALSE(found_inplace);
}

TEST_F(LEPUSIRTestIROpts, InstructionSelectionUnaryDecEmitsDec2WhenSrcAlive) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_isel_unary_dec_emits_dec2";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t ten_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(10));
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* ten = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(ten_idx), TypeOp::CreateInt64(&builder));
  auto* dec =
      builder.Create<UnaryOperatorInst>(0, ten, ValueKind::UnaryDecInstKind);
  // Keep `ten` alive after unary.
  ArgList items = {ten, dec};
  auto* arr = builder.Create<NewArrayInst>(0, items);
  builder.Create<ReturnInst>(0, arr);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_dec2 = false;
  bool found_inplace = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    long op = lepus::Instruction::GetOpCode(*inst);
    if (op == lepus::TypeOp_Dec2) found_dec2 = true;
    if (op == lepus::TypeOp_Dec) found_inplace = true;
  }
  EXPECT_TRUE(found_dec2);
  EXPECT_FALSE(found_inplace);
}

TEST_F(LEPUSIRTestIROpts, ConstructSSALoop) {
  auto lepus_func = lepus::Function::Create();
  lepus_func->SetRegisterCount(4);
  lepus_func->SetParamsSize(0);
  lepus_func->AddConstNumber(0);   // idx 0
  lepus_func->AddConstNumber(10);  // idx 1

  // Bytecode:
  // 0: LOAD_CONST 1, 0  // i = 0
  // 1: LOAD_CONST 2, 1  // limit = 10
  // 2: LESS 3, 1, 2     // loop header: tmp = i < limit
  // 3: JMP_FALSE 3, 3   // if !tmp jump to 3+3=6 (exit)
  // 4: INC 1            // i++
  // 5: JMP -4           // jump to 5-4=1
  // 6: RET 1
  // 7: PLACEHOLDER

  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_LoadConst, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_LoadConst, 2, (short)1));
  lepus_func->AddInstruction(lepus::Instruction(lepus::TypeOp_Less, 3, 1, 2));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_JmpFalse, 3, (short)3));
  lepus_func->AddInstruction(lepus::Instruction(lepus::TypeOp_Inc, 1, 1, 0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_Jmp, 0, (short)-4));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_Ret, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::OP_PLACEHOLDER, 0, (short)0));

  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_ssa_loop";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);

  ConstructSSAIRPass ssa_pass(ir_ctx.get());
  ssa_pass.RunOnFunction(func);

  // The loop header block (at offset 1) should have a Phi node for reg 1 (i).
  // The jump -4 from offset 5 goes to offset 1 (LOAD_CONST 2, 1).
  // So the block starting at 1 is a loop header.
  Block* loop_header = nullptr;
  for (auto& block : *func) {
    // Find the loop header by checking for the block containing offset 1.
    for (auto& inst : block) {
      if (auto* load = llvh::dyn_cast<LoadConstInst>(&inst)) {
        if (load->GetConst() &&
            llvh::isa<LiteralUint32>(load->GetSingleOperand()) &&
            llvh::cast<LiteralUint32>(load->GetSingleOperand())->GetValue() ==
                1) {
          loop_header = &block;
          break;
        }
      }
    }
    if (loop_header) break;
  }

  ASSERT_NE(loop_header, nullptr);
  bool found_phi = false;
  for (auto& inst : *loop_header) {
    if (llvh::isa<PhiInst>(&inst)) {
      found_phi = true;
      break;
    }
  }
  EXPECT_TRUE(found_phi);
}

TEST_F(LEPUSIRTestIROpts, TypeSpecificationStringSlice) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_slice";
  auto* func = builder.Create<FuncOp>(0, name);
  auto lepus_func = lepus::Function::Create();
  uint32_t slice_id = lepus_func->AddConstString(constants::kStringSlice);
  func->Init(lepus_func);

  Block* entry =
      builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* str = builder.GetLiteralInt32(0);
  str->SetType(TypeOp::CreateString(&builder));

  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(slice_id), TypeOp::CreateString(&builder));

  auto* get_table = builder.Create<GetTableInst>(0, str, key);

  TypeSpecification pass(ir_ctx.get());
  pass.RunOnFunction(func);

  EXPECT_TRUE(get_table->GetType()->IsStringProtoAPIType());

  builder.SetInsertionPointAfter(get_table);
  ArgList args = {get_table};
  auto* call = builder.Create<CallInst>(0, get_table, args);
  builder.Create<ReturnInst>(0, call);

  pass.RunOnFunction(func);

  // slice should return String type (verified in StringSwitch)
  EXPECT_TRUE(call->GetType()->IsStringType());
}

TEST_F(LEPUSIRTestIROpts, ConstructSSAToplevelVar) {
  auto lepus_func = lepus::Function::Create();
  lepus_func->SetRegisterCount(2);
  lepus_func->SetParamsSize(0);

  // Bytecode:
  // 0: LOAD_CONST 1, 0
  // 1: RET 1
  lepus_func->AddConstNumber(100);
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_LoadConst, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_Ret, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::OP_PLACEHOLDER, 0, (short)0));

  // Set up toplevel variable info in VMContext
  lepus::VMContext vm_ctx_local;
  vm_ctx_local.UpdateToplevelVarReg(base::String("var0"),
                                    1);  // reg 1 is toplevel var "var0"
  vm_ctx_local.UpdateToplevelVarRegToOffset(1, 0);

  // Create a new IRContext with the local VMContext
  auto ir_ctx_local = std::make_unique<IRContext>(&vm_ctx_local);
  auto* mod_local = ir_ctx_local->GetMainMod();
  OpBuilder builder;
  builder.SetModuleOp(mod_local);
  builder.SetInsertionPointToEnd(mod_local->GetFunctionBlock());

  std::string name = "test_ssa_toplevel";
  auto* func = builder.Create<FuncOp>(0, name);
  func->SetTopLevelFunction();
  func->Init(lepus_func);

  ConstructSSAIRPass ssa_pass(ir_ctx_local.get());
  ssa_pass.RunOnFunction(func);

  // The LoadConstInst should have toplevelVarReg set to 1
  bool found_toplevel = false;
  for (auto& block : *func) {
    for (auto& inst : block) {
      if (inst.GetToplevelVarReg() == 1) {
        found_toplevel = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found_toplevel);
}

TEST_F(LEPUSIRTestIROpts, ConstructSSAToplevelVarRequiresOffsetMapping) {
  auto lepus_func = lepus::Function::Create();
  lepus_func->SetRegisterCount(2);
  lepus_func->SetParamsSize(0);

  // Bytecode:
  // 0: LOAD_CONST 1, 0
  // 1: RET 1
  lepus_func->AddConstNumber(100);
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_LoadConst, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::TypeOp_Ret, 1, (short)0));
  lepus_func->AddInstruction(
      lepus::Instruction(lepus::OP_PLACEHOLDER, 0, (short)0));

  // Set up toplevel variable info in VMContext, but intentionally omit
  // UpdateToplevelVarRegToOffset() to simulate missing first-write offset.
  lepus::VMContext vm_ctx_local;
  vm_ctx_local.UpdateToplevelVarReg(base::String("var0"), 1);

  auto ir_ctx_local = std::make_unique<IRContext>(&vm_ctx_local);
  auto* mod_local = ir_ctx_local->GetMainMod();
  OpBuilder builder;
  builder.SetModuleOp(mod_local);
  builder.SetInsertionPointToEnd(mod_local->GetFunctionBlock());

  std::string name = "test_ssa_toplevel_missing_offset";
  auto* func = builder.Create<FuncOp>(0, name);
  func->SetTopLevelFunction();
  func->Init(lepus_func);

  ConstructSSAIRPass ssa_pass(ir_ctx_local.get());
  EXPECT_THROW(ssa_pass.RunOnFunction(func), ::lynx::lepus::CompileException);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
