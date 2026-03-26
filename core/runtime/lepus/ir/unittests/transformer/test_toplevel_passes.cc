// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/transformer/mir/change_special_attribute.h"
#include "core/runtime/lepus/ir/transformer/mir/get_toplevel_related_inst_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/process_special_mov.h"
#include "core/runtime/lepus/ir/transformer/mir/ssa_ir_verify.h"
#include "core/runtime/lepus/ir/transformer/mir/update_toplevel_closure_var.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/op_code.h"

namespace lynx {
namespace lepus {
namespace ir {

TEST(LEPUSIRFunctionTest, ClearOpCodesDoesNotClearUpvalues) {
  auto f = lepus::Function::Create();
  ASSERT_NE(f.get(), nullptr);

  f->AddUpvalue("x", 1, true);
  f->AddInstruction(lepus::Instruction::Code(lepus::TypeOp_Noop));
  ASSERT_EQ(f->UpvaluesSize(), 1u);
  ASSERT_EQ(f->OpCodeSize(), 1u);

  f->ClearOpCodes();
  EXPECT_EQ(f->OpCodeSize(), 0u);
  // Upvalues are semantic metadata and must survive bytecode reset.
  EXPECT_EQ(f->UpvaluesSize(), 1u);
}

class LEPUSIRTestToplevelPasses : public IRTestBase {
 public:
  virtual void SetUp(void) { IRTestBase::SetUp(); }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestToplevelPasses, CollectToplevelClosureRegBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  // 1. Create root function
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  mod->SetRootFunction(root_func);

  // 2. Create child function
  std::string child_name = "child";
  auto* child_func = builder.Create<FuncOp>(0, child_name);
  auto child_lepus_func = lepus::Function::Create();
  child_func->Init(child_lepus_func);

  // Add child to root
  root_lepus_func->AddChildFunction(child_lepus_func);

  // 3. Add upvalue to child referring to root's register
  child_lepus_func->AddUpvalue("x", 5, true);

  // 4. Run CollectToplevelClosureRegPass
  auto* pass = CreateCollectToplevelClosureRegPass(ir_ctx.get());
  static_cast<ModulePass*>(pass)->RunOnModule(mod);
  delete pass;

  // 5. Verify root_func has register 5 in toplevel_closure_var_regs_
  auto& regs = root_func->GetToplevelClosureVarRegs();
  EXPECT_TRUE(regs.count(5));
}

TEST_F(LEPUSIRTestToplevelPasses, UpdateToplevelClosureVarBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  // 1. Create root function as toplevel
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  // 2. Add closure variable to root
  // Create a dummy block and instruction so register allocator has something to
  // do
  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);
  auto* val = builder.Create<NopInst>(0);
  val->SetType(TypeOp::CreateAnyType(&builder));
  val->SetClosureVarReg(10);
  root_func->RecordClosureVarRegAndValue(10, val);
  root_func->InsertToplevelClosureVarReg(10);

  // 3. Create child function
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string child_name = "child";
  auto* child_func = builder.Create<FuncOp>(0, child_name);
  auto child_lepus_func = lepus::Function::Create();
  child_func->Init(child_lepus_func);
  root_lepus_func->AddChildFunction(child_lepus_func);
  child_lepus_func->AddUpvalue("x", 10, true);

  builder.SetInsertionPointToEnd(entry);
  builder.Create<ReturnInst>(0, val);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(root_func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(root_func);
  ASSERT_NE(nullptr, ra);
  unsigned physical_reg = ra->GetRegister(val).GetIndex();

  // 5. Run UpdateToplevelClosureVarPass
  auto* opt_pass = CreateUpdateToplevelClosureVarPass(ir_ctx.get());
  static_cast<ModulePass*>(opt_pass)->RunOnModule(mod);
  delete opt_pass;

  // 6. Verify child_func has the mapping
  EXPECT_EQ(child_func->GetClosureVarToplevelReg(0), physical_reg);
}

TEST_F(LEPUSIRTestToplevelPasses,
       UpdateToplevelClosureVarUsesClosureMapWhenComputingUpvalueReg) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  // 1) Create root function as toplevel.
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // Two distinct toplevel variables (old regs 2 and 12).
  auto* val2 = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(2),
                                             TypeOp::CreateInt32(&builder));
  auto* val12 = builder.Create<LoadConstInst>(1, builder.GetLiteralInt32(12),
                                              TypeOp::CreateInt32(&builder));

  ir_ctx->InsertToplevelValue(val2, 2);
  ir_ctx->InsertToplevelValue(val12, 12);

  // Simulate a stale/incorrect closure map: old reg 12 points to val2.
  // Mark val2 as "toplevel-related" to satisfy UpdateToplevelClosureVarPass's
  // consistency assertions (it expects the mapped value to carry old_reg via
  // either ClosureVarReg or ToplevelVarReg).
  val2->SetToplevelVarReg(12);
  root_func->RecordClosureVarRegAndValue(12, val2);
  root_func->InsertToplevelClosureVarReg(12);

  // 2) Create child function that captures old reg 12.
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string child_name = "child";
  auto* child_func = builder.Create<FuncOp>(0, child_name);
  auto child_lepus_func = lepus::Function::Create();
  child_func->Init(child_lepus_func);
  root_lepus_func->AddChildFunction(child_lepus_func);
  child_lepus_func->AddUpvalue("x", 12, true);

  builder.SetInsertionPointToEnd(entry);
  builder.Create<ReturnInst>(2, val12);

  // 3) Run register allocation for root.
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(root_func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(root_func);
  ASSERT_NE(nullptr, ra);

  unsigned reg_of_val2 = ra->GetRegister(val2).GetIndex();
  unsigned reg_of_val12 = ra->GetRegister(val12).GetIndex();
  ASSERT_NE(reg_of_val2, reg_of_val12);

  // 4) Run UpdateToplevelClosureVarPass.
  auto* opt_pass = CreateUpdateToplevelClosureVarPass(ir_ctx.get());
  static_cast<ModulePass*>(opt_pass)->RunOnModule(mod);
  delete opt_pass;

  // 5) The mapping follows the closure map (val2), even if it is stale.
  EXPECT_EQ(child_func->GetClosureVarToplevelReg(0),
            static_cast<int>(reg_of_val2));
}

TEST_F(LEPUSIRTestToplevelPasses, RemoveUselessGetToplevelVarBasic) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  // 1. Create root function
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  // 2. Create block and GetToplevelVarInst
  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* get_toplevel_var = builder.Create<GetToplevelVarInst>(
      0, builder.GetLiteralUint32(5), TypeOp::CreateAnyType(&builder));

  // 3. Verify it exists
  EXPECT_EQ(entry->size(), 1);
  EXPECT_EQ(get_toplevel_var->GetNumUsers(), 0);

  // 4. Run GetToplevelRelatedInstEliminationPass
  auto* pass = CreateGetToplevelRelatedInstEliminationPass(ir_ctx.get());
  static_cast<ModulePass*>(pass)->RunOnModule(mod);
  delete pass;

  // 5. Verify it is removed
  EXPECT_EQ(entry->size(), 0);
}

TEST_F(LEPUSIRTestToplevelPasses, ChangeSpecialAttributeClearsClosureOldReg) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  // 1. Create root function as toplevel
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(1, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  // 2. Create block and a SetToplevelVarInst that carries both attrs.
  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* src = builder.Create<LoadConstInst>(2, builder.GetLiteralInt32(1),
                                            TypeOp::CreateInt32(&builder));
  auto* set_inst =
      builder.Create<SetToplevelVarInst>(3, builder.GetLiteralUint32(0), src);
  set_inst->SetToplevelVarReg(0);
  set_inst->SetClosureVarReg(0);
  ir_ctx->InsertToplevelValue(set_inst, 0);
  builder.Create<ReturnInst>(4, builder.GetLiteralInt32(0));

  // 3. SSAIRVerifyPass should accept (because the two values are equal).
  SSAIRVerifyPass verify(ir_ctx.get());
  verify.RunOnModule(mod);

  // 4. ChangeSpecialAttributePass clears ClosureVarReg.
  ChangeSpecialAttributePass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  EXPECT_EQ(set_inst->GetToplevelVarReg(), 0);
  EXPECT_EQ(set_inst->GetClosureVarReg(), constants::kInvalidSignedValue);
}

TEST_F(LEPUSIRTestToplevelPasses,
       ProcessSpecialMovKeepsClosureVarMappingOnSetToplevelVarInst) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  // 1) Create root function as toplevel.
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  // 2) Build a minimal block that assigns a toplevel closure variable.
  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  // A source value for the move.
  auto* src = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(1),
                                            TypeOp::CreateInt32(&builder));

  // This MovInst models a toplevel register assignment produced by the
  // bytecode builder. Mark it as both a toplevel var and a toplevel closure var
  // (same old register id).
  auto* mov = builder.Create<MovInst>(1, src);
  mov->SetToplevelVarReg(12);
  mov->SetClosureVarReg(12);

  ir_ctx->InsertToplevelValue(mov, 12);
  root_func->RecordClosureVarRegAndValue(12, mov);
  root_func->InsertToplevelClosureVarReg(12);

  builder.Create<ReturnInst>(2, src);

  // 3) Run ProcessSpecialMovPass on root function.
  ProcessSpecialMovPass pass(ir_ctx.get());
  pass.RunOnFunction(root_func);

  // 4) Closure old-reg mapping follows IRContext::toplevel_variables_ update
  // policy. After ProcessSpecialMov, it should point to the SetToplevelVarInst
  // (and later passes are responsible for keeping the mapping updated if the
  // Set is eliminated).
  auto& map = root_func->GetClosureVarReg2ValueMap();
  auto it = map.find(12);
  ASSERT_NE(it, map.end());
  ASSERT_NE(it->second, nullptr);
  EXPECT_TRUE(llvh::isa<SetToplevelVarInst>(it->second));
}

TEST_F(LEPUSIRTestToplevelPasses,
       RemoveUselessGetToplevelVarKeepsUsedGetButClearsAttrs) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* get_toplevel_var = builder.Create<GetToplevelVarInst>(
      0, builder.GetLiteralUint32(5), TypeOp::CreateAnyType(&builder));
  // Keep it used so the pass won't delete it.
  builder.Create<ReturnInst>(0, get_toplevel_var);

  auto* pass = CreateGetToplevelRelatedInstEliminationPass(ir_ctx.get());
  static_cast<ModulePass*>(pass)->RunOnModule(mod);
  delete pass;

  // The instruction should still exist, but its special attrs should be
  // cleared.
  EXPECT_EQ(get_toplevel_var->GetToplevelVarReg(),
            constants::kInvalidSignedValue);
  EXPECT_EQ(get_toplevel_var->GetClosureVarReg(),
            constants::kInvalidSignedValue);
}

TEST_F(LEPUSIRTestToplevelPasses,
       GetToplevelRelatedElimination_NoCrash_WhenSetIsLastInstInBlock) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* src = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(1),
                                            TypeOp::CreateInt32(&builder));
  builder.Create<SetToplevelVarInst>(1, builder.GetLiteralUint32(5), src);

  auto* pass = CreateGetToplevelRelatedInstEliminationPass(ir_ctx.get());
  static_cast<ModulePass*>(pass)->RunOnModule(mod);
  delete pass;
}

TEST_F(LEPUSIRTestToplevelPasses,
       GetToplevelRelatedElimination_NoCrash_WhenNextIsNotGet) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* src = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(1),
                                            TypeOp::CreateInt32(&builder));
  builder.Create<SetToplevelVarInst>(1, builder.GetLiteralUint32(5), src);
  builder.Create<NopInst>(2);

  auto* pass = CreateGetToplevelRelatedInstEliminationPass(ir_ctx.get());
  static_cast<ModulePass*>(pass)->RunOnModule(mod);
  delete pass;
}

TEST_F(LEPUSIRTestToplevelPasses,
       GetToplevelRelatedElimination_EliminatesAdjacentGet_ForSameRegOnly) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* src = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(7),
                                            TypeOp::CreateInt32(&builder));

  // Case 1: Set(reg=5) + Get(reg=5) => Get should be eliminated.
  auto* set5 =
      builder.Create<SetToplevelVarInst>(1, builder.GetLiteralUint32(5), src);
  auto* get5 = builder.Create<GetToplevelVarInst>(
      2, builder.GetLiteralUint32(5), TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(3, get5);

  auto* pass = CreateGetToplevelRelatedInstEliminationPass(ir_ctx.get());
  static_cast<ModulePass*>(pass)->RunOnModule(mod);
  delete pass;

  // get5 should be erased; Return should use src.
  bool found_get5 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get5) found_get5 = true;
  }
  EXPECT_FALSE(found_get5);
  (void)set5;
}

TEST_F(LEPUSIRTestToplevelPasses,
       ProcessSpecialMov_ClosureOnly_LowersToGetSet) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  // 1) Create root function as toplevel.
  std::string root_name = "root";
  auto* root_func = builder.Create<FuncOp>(0, root_name);
  auto root_lepus_func = lepus::Function::Create();
  root_func->Init(root_lepus_func);
  root_func->SetTopLevelFunction();
  mod->SetRootFunction(root_func);

  // 2) Build block with a closure-only special MovInst.
  Block* entry =
      builder.CreateBlock(root_func->GetSingleRegion(), BlockType::BT_INST, {});
  builder.SetInsertionPointToStart(entry);

  auto* src = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(1),
                                            TypeOp::CreateInt32(&builder));
  auto* mov = builder.Create<MovInst>(1, src);
  mov->SetClosureVarReg(7);

  // Provide side-table mapping so UpdateSpecialAttribute can update it.
  root_func->RecordClosureVarRegAndValue(7, mov);
  root_func->InsertToplevelClosureVarReg(7);

  builder.Create<ReturnInst>(2, mov);

  ProcessSpecialMovPass pass(ir_ctx.get());
  pass.RunOnFunction(root_func);

  // The closure mapping should now point to a GetToplevelClosureVarInst.
  auto& map = root_func->GetClosureVarReg2ValueMap();
  auto it = map.find(7);
  ASSERT_NE(it, map.end());
  ASSERT_NE(it->second, nullptr);
  EXPECT_TRUE(llvh::isa<GetToplevelClosureVarInst>(it->second));
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
