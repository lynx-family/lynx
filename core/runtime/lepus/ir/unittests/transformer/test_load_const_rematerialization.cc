// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <vector>

#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/transformer/vm/hvm_register_allocator.h"
#include "core/runtime/lepus/ir/transformer/vm/load_const_rematerialization.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRLoadConstRematerializationTest : public IRTestBase {
 public:
  void SetUp() override { IRTestBase::SetUp(); }
  void TearDown() override { IRTestBase::TearDown(); }
};

namespace {

FuncOp* CreateRootFunctionForRematerializationTest(IRContext* ir_ctx,
                                                   ModuleOp* mod,
                                                   const std::string& name) {
  auto* builder = ir_ctx->GetOpBuilder();
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto func_name = name;
  auto* func = builder->Create<FuncOp>(0, func_name);
  auto lepus_func = lepus::Function::Create();
  func->Init(lepus_func);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);
  builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  return func;
}

FuncOp* CreateChildFunctionForRematerializationTest(IRContext* ir_ctx,
                                                    ModuleOp* mod,
                                                    FuncOp* parent,
                                                    const std::string& name) {
  auto* builder = ir_ctx->GetOpBuilder();
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto func_name = name;
  auto* func = builder->Create<FuncOp>(0, func_name);
  auto lepus_func = lepus::Function::Create();
  lepus_func->SetFunctionName(name);
  if (parent && parent->GetLepusFunction()) {
    parent->GetLepusFunction()->AddChildFunction(lepus_func);
  }
  func->Init(lepus_func);
  builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  return func;
}

unsigned ComputePreliminaryRegisterUsage(FuncOp* func) {
  HVMRegisterAllocator allocator(func);
  PostOrderAnalysis po(func);
  llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
  allocator.Preallocate();
  allocator.Allocate(order);
  return allocator.GetMaxRegisterUsage();
}

bool InstructionUsesValueForTest(Instruction* user, Value* value) {
  if (!user || !value) return false;
  for (unsigned i = 0, e = user->GetNumOperands(); i < e; ++i) {
    if (user->GetOperand(i) == value) return true;
  }
  return false;
}

size_t CountLoadConstWithLiteral(FuncOp* func, Literal* literal) {
  size_t count = 0;
  for (auto& bb : *func) {
    for (auto* inst : bb.InstRange()) {
      auto* load = llvh::dyn_cast<LoadConstInst>(inst);
      if (load && load->GetConst() == literal) {
        ++count;
      }
    }
  }
  return count;
}

size_t CountLoadConstWithLiteralInBlock(Block* block, Literal* literal) {
  size_t count = 0;
  if (!block) return count;
  for (auto* inst : block->InstRange()) {
    auto* load = llvh::dyn_cast<LoadConstInst>(inst);
    if (load && load->GetConst() == literal) {
      ++count;
    }
  }
  return count;
}

size_t CountMovInsts(FuncOp* func) {
  size_t count = 0;
  if (!func) return count;
  for (auto& bb : *func) {
    for (auto* inst : bb.InstRange()) {
      if (llvh::isa<MovInst>(inst)) {
        ++count;
      }
    }
  }
  return count;
}

size_t CountCallFuncMovInsts(FuncOp* func) {
  size_t count = 0;
  if (!func) return count;
  for (auto& bb : *func) {
    for (auto* inst : bb.InstRange()) {
      auto* mov = llvh::dyn_cast<MovInst>(inst);
      if (mov && mov->IsCallFuncMov()) {
        ++count;
      }
    }
  }
  return count;
}

size_t CountPhiInsts(FuncOp* func) {
  size_t count = 0;
  if (!func) return count;
  for (auto& bb : *func) {
    for (auto* inst : bb.InstRange()) {
      if (llvh::isa<PhiInst>(inst)) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationReducesOverflowingRegisterPressure) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod, "load_const_rematerialization_reduces_pressure");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(777777), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(1000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      auto* user = builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
      if (load == tracked) {
        tracked_users.push_back(user);
      }
    }
  }
  builder->Create<ReturnInst>(0, table);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(tracked_users.front(), tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  const unsigned final_usage = ComputePreliminaryRegisterUsage(func);
  EXPECT_LE(final_usage, Register::kMaxRegistersLimit);
  EXPECT_GT(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.front(), tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.back(), tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationIsNoOpWhenRegisterPressureFits) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod, "load_const_rematerialization_noop_when_under_limit");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 64;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(888888), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(2000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      auto* user = builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
      if (load == tracked) {
        tracked_users.push_back(user);
      }
    }
  }
  builder->Create<ReturnInst>(0, table);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_LE(initial_usage, Register::kMaxRegistersLimit);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(tracked_users.front(), tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_EQ(ComputePreliminaryRegisterUsage(func), initial_usage);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  EXPECT_TRUE(InstructionUsesValueForTest(tracked_users.front(), tracked));
  EXPECT_TRUE(InstructionUsesValueForTest(tracked_users.back(), tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationNoOpDoesNotLowerPhiOnOriginalIR) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_noop_preserves_original_phi_ir");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  auto* join =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 64;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(888889), TypeOp::CreateInt32(builder));
  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(22000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      auto* user = builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
      if (load == tracked) {
        tracked_users.push_back(user);
      }
    }
  }

  builder->Create<BranchInst>(0, join);
  builder->SetInsertionPointToStart(join);
  PhiInst::ValueListType values;
  PhiInst::BlockListType blocks;
  auto* phi = builder->Create<PhiInst>(0, values, blocks);
  phi->AddEntry(tracked, entry);
  builder->Create<ReturnInst>(0, phi);

  ASSERT_EQ(CountMovInsts(func), 0u);
  ASSERT_EQ(CountPhiInsts(func), 1u);
  ASSERT_EQ(phi->GetNumEntries(), 1u);

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_EQ(CountMovInsts(func), 0u);
  EXPECT_EQ(CountCallFuncMovInsts(func), 0u);
  EXPECT_EQ(CountPhiInsts(func), 1u);
  EXPECT_EQ(phi->GetNumEntries(), 1u);
  for (auto* user : tracked_users) {
    EXPECT_TRUE(InstructionUsesValueForTest(user, tracked));
  }
  EXPECT_TRUE(InstructionUsesValueForTest(phi, tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationNoOpDoesNotInsertCallFuncMovOnOriginalIR) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_noop_preserves_original_call_ir");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 64;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(888890), TypeOp::CreateInt32(builder));
  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(23000 + i), TypeOp::CreateInt32(builder)));
  }

  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
    }
  }

  auto* live0 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                               TypeOp::CreateInt32(builder));
  auto* live1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                               TypeOp::CreateInt32(builder));
  auto* live2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(3),
                                               TypeOp::CreateInt32(builder));
  auto* callee = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(100), TypeOp::CreateInt32(builder));
  ArgList args;
  auto* call = builder->Create<CallInst>(0, callee, args);
  auto* sum0 = builder->Create<BinaryOperatorInst>(
      0, live0, live1, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  auto* sum1 = builder->Create<BinaryOperatorInst>(
      0, sum0, live2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  auto* result = builder->Create<BinaryOperatorInst>(
      0, call, sum1, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, result);

  ASSERT_EQ(CountMovInsts(func), 0u);
  ASSERT_EQ(CountCallFuncMovInsts(func), 0u);
  ASSERT_EQ(call->GetFunction(), callee);

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_EQ(CountMovInsts(func), 0u);
  EXPECT_EQ(CountCallFuncMovInsts(func), 0u);
  EXPECT_EQ(call->GetFunction(), callee);
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationNoOpDoesNotShareLiteralUsesWithShadowClone) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_noop_keeps_literal_use_lists_isolated");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 64;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(888891), TypeOp::CreateInt32(builder));
  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(24000 + i), TypeOp::CreateInt32(builder)));
  }

  auto* shared_slot = builder->GetLiteralUint32(0);
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
    }
  }

  auto* set_toplevel =
      builder->Create<SetToplevelVarInst>(0, shared_slot, table);
  builder->Create<ReturnInst>(0, table);

  const unsigned initial_shared_slot_users = shared_slot->GetNumUsers();
  ASSERT_GT(initial_shared_slot_users, 0u);

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_EQ(shared_slot->GetNumUsers(), initial_shared_slot_users);
  EXPECT_NO_THROW(set_toplevel->EraseFromParent());
  EXPECT_EQ(shared_slot->GetNumUsers(), initial_shared_slot_users - 1);
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationDoesNotLeakShadowUsersToExternalFuncOp) {
  // Verify that the shadow clone's operand cleanup prevents dangling entries in
  // use-lists of values that persist beyond the shadow module's lifetime. When
  // the shadow RA processes CallInsts, it may insert MovInsts whose operands
  // reference cloned values. The cleanup loop must detach all shadow
  // instructions from their operands' use-lists before the shadow module is
  // destroyed, ensuring no Literal or other persistent value retains phantom
  // users from the shadow clone.
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_shadow_cleanup_with_call");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 64;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(888892), TypeOp::CreateInt32(builder));
  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(25000 + i), TypeOp::CreateInt32(builder)));
  }

  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
    }
  }

  // Same CallInst pattern as NoOpDoesNotInsertCallFuncMovOnOriginalIR.
  auto* live0 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                               TypeOp::CreateInt32(builder));
  auto* live1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                               TypeOp::CreateInt32(builder));
  auto* live2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(3),
                                               TypeOp::CreateInt32(builder));
  auto* callee = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(100), TypeOp::CreateInt32(builder));
  ArgList args;
  auto* call = builder->Create<CallInst>(0, callee, args);
  auto* sum0 = builder->Create<BinaryOperatorInst>(
      0, live0, live1, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  auto* sum1 = builder->Create<BinaryOperatorInst>(
      0, sum0, live2, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  auto* result = builder->Create<BinaryOperatorInst>(
      0, call, sum1, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, result);

  // Track a Literal that is heavily used as an operand.
  auto* shared_literal = builder->GetLiteralUint32(0);
  const unsigned literal_users_before = shared_literal->GetNumUsers();
  ASSERT_GT(literal_users_before, 0u);

  LoadConstRematerializationPass pass(ir_ctx.get());
  pass.RunOnModule(mod);

  // The Literal's user count must not have changed — no phantom users leaked
  // from the shadow clone or from RA-inserted MovInsts in the shadow.
  EXPECT_EQ(shared_literal->GetNumUsers(), literal_users_before);
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationRewritesNewArrayBooleanOperands) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_new_array_boolean_operands");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kFillerConstCount = 255;
  constexpr int kRounds = 4;
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralBool(true), TypeOp::CreateBoolean(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> filler_consts;
  filler_consts.reserve(kFillerConstCount);
  for (int i = 0; i < kFillerConstCount; ++i) {
    filler_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(21000 + i), TypeOp::CreateInt32(builder)));
  }

  for (int i = 0; i < kFillerConstCount; ++i) {
    builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(i)),
        filler_consts[i]);
  }

  std::vector<NewArrayInst*> tracked_users;
  tracked_users.reserve(kRounds);
  NewArrayInst* last_array = nullptr;
  for (int round = 0; round < kRounds; ++round) {
    ArgList items;
    items.push_back(tracked);
    last_array = builder->Create<NewArrayInst>(0, items);
    tracked_users.push_back(last_array);
  }
  builder->Create<ReturnInst>(0, last_array);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(tracked_users.front(), tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  const unsigned final_usage = ComputePreliminaryRegisterUsage(func);
  EXPECT_LE(final_usage, Register::kMaxRegistersLimit);
  EXPECT_GE(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.front(), tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.back(), tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationRewritesDynamicSetTableKeyOperands) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod, "load_const_rematerialization_dynamic_set_table_keys");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(31000), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> keys;
  keys.reserve(kHotConstCount);
  keys.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    keys.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(31000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    for (auto* key : keys) {
      auto* user = builder->Create<SetTableInst>(
          0, table, key,
          builder->GetLiteralInt32(32000 + round * kHotConstCount));
      if (key == tracked) {
        tracked_users.push_back(user);
      }
    }
  }
  builder->Create<ReturnInst>(0, table);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(tracked_users.front(), tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  const unsigned final_usage = ComputePreliminaryRegisterUsage(func);
  EXPECT_LE(final_usage, Register::kMaxRegistersLimit);
  EXPECT_GT(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.front(), tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.back(), tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationSupportsLongTailNullOperands) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_long_tail_null_operands");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kFillerConstCount = 255;
  auto* tracked = builder->Create<LoadConstInst>(0, builder->GetLiteralNull(),
                                                 TypeOp::CreateNull(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> first_phase_values;
  first_phase_values.reserve(kFillerConstCount);
  for (int i = 0; i < kFillerConstCount; ++i) {
    first_phase_values.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(41000 + i), TypeOp::CreateInt32(builder)));
  }

  for (int i = 0; i < kFillerConstCount; ++i) {
    builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(i)),
        first_phase_values[i]);
  }

  ArgList first_items;
  first_items.push_back(tracked);
  auto* first_array = builder->Create<NewArrayInst>(0, first_items);

  std::vector<LoadConstInst*> second_phase_values;
  second_phase_values.reserve(kFillerConstCount);
  for (int i = 0; i < kFillerConstCount; ++i) {
    second_phase_values.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(42000 + i), TypeOp::CreateInt32(builder)));
  }

  for (int i = 0; i < kFillerConstCount; ++i) {
    builder->Create<SetTableConstStringKeyInst>(
        0, table,
        builder->GetLiteralUint32(static_cast<uint32_t>(kFillerConstCount + i)),
        second_phase_values[i]);
  }

  ArgList second_items;
  second_items.push_back(tracked);
  auto* second_array = builder->Create<NewArrayInst>(0, second_items);
  builder->Create<ReturnInst>(0, second_array);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(first_array, tracked));
  ASSERT_TRUE(InstructionUsesValueForTest(second_array, tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  const unsigned final_usage = ComputePreliminaryRegisterUsage(func);
  EXPECT_LE(final_usage, Register::kMaxRegistersLimit);
  EXPECT_GE(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(first_array, tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(second_array, tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationRewritesAllRepeatedUndefinedOperands) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_repeated_undefined_operands");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kFillerConstCount = 255;
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUndefined(), TypeOp::CreateUndefined(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> filler_consts;
  filler_consts.reserve(kFillerConstCount);
  for (int i = 0; i < kFillerConstCount; ++i) {
    filler_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(43000 + i), TypeOp::CreateInt32(builder)));
  }

  for (int i = 0; i < kFillerConstCount; ++i) {
    builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(i)),
        filler_consts[i]);
  }

  ArgList repeated_array_items;
  repeated_array_items.push_back(tracked);
  repeated_array_items.push_back(tracked);
  auto* repeated_array = builder->Create<NewArrayInst>(0, repeated_array_items);

  auto* repeated_set =
      builder->Create<SetTableInst>(0, table, tracked, tracked);
  auto* value_only_user = builder->Create<SetTableConstStringKeyInst>(
      0, table, builder->GetLiteralUint32(9000), tracked);

  ArgList tail_array_items;
  tail_array_items.push_back(tracked);
  auto* tail_array = builder->Create<NewArrayInst>(0, tail_array_items);
  builder->Create<ReturnInst>(0, tail_array);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(repeated_array, tracked));
  ASSERT_TRUE(InstructionUsesValueForTest(repeated_set, tracked));
  ASSERT_TRUE(InstructionUsesValueForTest(value_only_user, tracked));
  ASSERT_TRUE(InstructionUsesValueForTest(tail_array, tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  EXPECT_LE(ComputePreliminaryRegisterUsage(func),
            Register::kMaxRegistersLimit);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(repeated_array, tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(repeated_set, tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(value_only_user, tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(tail_array, tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationSkipsPhiUsers) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod, "load_const_rematerialization_skip_phi");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  auto* join =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(999991), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;
  std::vector<LoadConstInst*> filler_consts;
  filler_consts.reserve(kHotConstCount - 1);
  for (int i = 1; i < kHotConstCount; ++i) {
    filler_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(3000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    tracked_users.push_back(builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
        tracked));
    for (auto* load : filler_consts) {
      builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
    }
  }

  builder->Create<BranchInst>(0, join);
  builder->SetInsertionPointToStart(join);
  PhiInst::ValueListType values;
  PhiInst::BlockListType blocks;
  auto* phi = builder->Create<PhiInst>(0, values, blocks);
  phi->AddEntry(tracked, entry);
  builder->Create<ReturnInst>(0, phi);

  // Do not run preliminary RA before the pass here: preliminary RA lowers Phi
  // incoming values into MovInsts and would hide the original Phi user that
  // this test expects LoadConstRematerializationPass to snapshot by itself.
  ASSERT_EQ(phi->GetNumEntries(), 1u);

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  for (auto* user : tracked_users) {
    EXPECT_TRUE(InstructionUsesValueForTest(user, tracked));
  }
  EXPECT_EQ(phi->GetNumEntries(), 1u);
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationSkipsWhenNonAggregateUserBlocksLivenessCut) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_skip_blocking_non_aggregate");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(999992), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;
  std::vector<LoadConstInst*> filler_consts;
  filler_consts.reserve(kHotConstCount - 1);
  for (int i = 1; i < kHotConstCount; ++i) {
    filler_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(4000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    tracked_users.push_back(builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
        tracked));
  }
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : filler_consts) {
      builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
    }
  }
  auto* blocking_use = builder->Create<ToStringInst>(0, tracked);
  builder->Create<ReturnInst>(0, blocking_use);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_TRUE(InstructionUsesValueForTest(blocking_use, tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  for (auto* user : tracked_users) {
    EXPECT_TRUE(InstructionUsesValueForTest(user, tracked));
  }
  EXPECT_TRUE(InstructionUsesValueForTest(blocking_use, tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationIsIdempotentAfterSuccessfulRewrite) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_idempotent_after_success");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(777778), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(5000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      auto* user = builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
      if (load == tracked) {
        tracked_users.push_back(user);
      }
    }
  }
  builder->Create<ReturnInst>(0, table);

  ASSERT_GT(ComputePreliminaryRegisterUsage(func),
            Register::kMaxRegistersLimit);

  LoadConstRematerializationPass pass(ir_ctx.get());
  ASSERT_TRUE(pass.RunOnModule(mod));

  const unsigned usage_after_first = ComputePreliminaryRegisterUsage(func);
  const size_t tracked_count_after_first =
      CountLoadConstWithLiteral(func, tracked_literal);
  ASSERT_LE(usage_after_first, Register::kMaxRegistersLimit);
  ASSERT_GT(tracked_count_after_first, 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.front(), tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.back(), tracked));

  EXPECT_FALSE(pass.RunOnModule(mod));
  EXPECT_EQ(ComputePreliminaryRegisterUsage(func), usage_after_first);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal),
            tracked_count_after_first);
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.front(), tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_users.back(), tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationFallbackHandlesSparseSingleUsers) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_fallback_sparse_single_users");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kConstCount = 270;
  constexpr int kDelayedCount = 20;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(777779), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> values;
  values.reserve(kConstCount);
  values.push_back(tracked);
  for (int i = 1; i < kConstCount; ++i) {
    values.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(6000 + i), TypeOp::CreateInt32(builder)));
  }

  SetTableConstStringKeyInst* tracked_user = nullptr;
  for (int i = kDelayedCount; i < kConstCount; ++i) {
    auto* user = builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(i)),
        values[i]);
    if (values[i] == tracked) {
      tracked_user = user;
    }
  }
  for (int i = 0; i < kDelayedCount; ++i) {
    auto* user = builder->Create<SetTableConstStringKeyInst>(
        0, table,
        builder->GetLiteralUint32(static_cast<uint32_t>(kConstCount + i)),
        values[i]);
    if (values[i] == tracked) {
      tracked_user = user;
    }
  }
  builder->Create<ReturnInst>(0, table);

  ASSERT_NE(tracked_user, nullptr);
  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(tracked_user, tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  EXPECT_LE(ComputePreliminaryRegisterUsage(func),
            Register::kMaxRegistersLimit);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(tracked_user, tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationPlacesCrossBlockCloneInUserBlock) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_independent_cross_block_"
      "clones");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  auto* then_bb =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  auto* else_bb =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  constexpr int kConstCount = 270;
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(777780), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> values;
  values.reserve(kConstCount);
  values.push_back(tracked);
  for (int i = 1; i < kConstCount; ++i) {
    values.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(7000 + i), TypeOp::CreateInt32(builder)));
  }
  builder->Create<CondBranchInst>(0, builder->GetLiteralBool(true), then_bb,
                                  else_bb);

  builder->SetInsertionPointToStart(then_bb);
  auto* then_table = builder->Create<NewTableInst>(0);
  SetTableConstStringKeyInst* then_tracked_user = nullptr;
  for (int i = 0; i < kConstCount; ++i) {
    auto* user = builder->Create<SetTableConstStringKeyInst>(
        0, then_table, builder->GetLiteralUint32(static_cast<uint32_t>(i)),
        values[i]);
    if (values[i] == tracked) {
      then_tracked_user = user;
    }
  }
  builder->Create<ReturnInst>(0, then_table);

  builder->SetInsertionPointToStart(else_bb);
  auto* else_table = builder->Create<NewTableInst>(0);
  SetTableConstStringKeyInst* else_tracked_user = nullptr;
  for (int i = 0; i < kConstCount; ++i) {
    auto* user = builder->Create<SetTableConstStringKeyInst>(
        0, else_table,
        builder->GetLiteralUint32(static_cast<uint32_t>(kConstCount + i)),
        values[i]);
    if (values[i] == tracked) {
      else_tracked_user = user;
    }
  }
  builder->Create<ReturnInst>(0, else_table);

  ASSERT_NE(then_tracked_user, nullptr);
  ASSERT_NE(else_tracked_user, nullptr);
  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  EXPECT_LE(ComputePreliminaryRegisterUsage(func),
            Register::kMaxRegistersLimit);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 2u);
  EXPECT_EQ(CountLoadConstWithLiteralInBlock(entry, tracked_literal), 0u);

  const size_t then_clone_count =
      CountLoadConstWithLiteralInBlock(then_bb, tracked_literal);
  const size_t else_clone_count =
      CountLoadConstWithLiteralInBlock(else_bb, tracked_literal);
  EXPECT_EQ(then_clone_count, 1u);
  EXPECT_EQ(else_clone_count, 1u);

  const bool then_rewired =
      !InstructionUsesValueForTest(then_tracked_user, tracked);
  const bool else_rewired =
      !InstructionUsesValueForTest(else_tracked_user, tracked);
  EXPECT_TRUE(then_rewired);
  EXPECT_TRUE(else_rewired);
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationPlacesOneFloat64ClonePerUserBlock) {
  auto* func = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_cross_block_float64_clones");
  auto* builder = ir_ctx->GetOpBuilder();
  auto* entry = &func->Front();
  auto* then_bb =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  auto* else_bb =
      builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  constexpr int kFillerConstCount = 269;
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralFloat64(3.1415926), TypeOp::CreateFloat64(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> filler_consts;
  filler_consts.reserve(kFillerConstCount);
  for (int i = 0; i < kFillerConstCount; ++i) {
    filler_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(44000 + i), TypeOp::CreateInt32(builder)));
  }
  builder->Create<CondBranchInst>(0, builder->GetLiteralBool(true), then_bb,
                                  else_bb);

  builder->SetInsertionPointToStart(then_bb);
  auto* then_table = builder->Create<NewTableInst>(0);
  for (int i = 0; i < kFillerConstCount; ++i) {
    builder->Create<SetTableConstStringKeyInst>(
        0, then_table, builder->GetLiteralUint32(static_cast<uint32_t>(i)),
        filler_consts[i]);
  }
  ArgList then_array_items;
  then_array_items.push_back(tracked);
  then_array_items.push_back(tracked);
  auto* then_array = builder->Create<NewArrayInst>(0, then_array_items);
  auto* then_set =
      builder->Create<SetTableInst>(0, then_table, tracked, tracked);
  builder->Create<ReturnInst>(0, then_array);

  builder->SetInsertionPointToStart(else_bb);
  auto* else_table = builder->Create<NewTableInst>(0);
  for (int i = 0; i < kFillerConstCount; ++i) {
    builder->Create<SetTableConstStringKeyInst>(
        0, else_table,
        builder->GetLiteralUint32(static_cast<uint32_t>(kFillerConstCount + i)),
        filler_consts[i]);
  }
  auto* else_value_user = builder->Create<SetTableConstStringKeyInst>(
      0, else_table, builder->GetLiteralUint32(9999), tracked);
  ArgList else_array_items;
  else_array_items.push_back(tracked);
  auto* else_array = builder->Create<NewArrayInst>(0, else_array_items);
  builder->Create<ReturnInst>(0, else_table);

  const unsigned initial_usage = ComputePreliminaryRegisterUsage(func);
  ASSERT_GT(initial_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(then_array, tracked));
  ASSERT_TRUE(InstructionUsesValueForTest(then_set, tracked));
  ASSERT_TRUE(InstructionUsesValueForTest(else_value_user, tracked));
  ASSERT_TRUE(InstructionUsesValueForTest(else_array, tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_TRUE(pass.RunOnModule(mod));

  EXPECT_LE(ComputePreliminaryRegisterUsage(func),
            Register::kMaxRegistersLimit);
  EXPECT_EQ(CountLoadConstWithLiteral(func, tracked_literal), 2u);
  EXPECT_EQ(CountLoadConstWithLiteralInBlock(entry, tracked_literal), 0u);
  EXPECT_EQ(CountLoadConstWithLiteralInBlock(then_bb, tracked_literal), 1u);
  EXPECT_EQ(CountLoadConstWithLiteralInBlock(else_bb, tracked_literal), 1u);
  EXPECT_FALSE(InstructionUsesValueForTest(then_array, tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(then_set, tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(else_value_user, tracked));
  EXPECT_FALSE(InstructionUsesValueForTest(else_array, tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationSkipsOverflowingChildRegisterPressure) {
  auto* root = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod, "load_const_rematerialization_child_root_anchor");
  auto* child = CreateChildFunctionForRematerializationTest(
      ir_ctx.get(), mod, root,
      "load_const_rematerialization_child_reduces_pressure");
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToStart(&root->Front());
  auto* root_value = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(42), TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, root_value);

  auto* entry = &child->Front();
  builder->SetInsertionPointToStart(entry);
  auto* table = builder->Create<NewTableInst>(0);
  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;

  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1777777), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> hot_consts;
  hot_consts.reserve(kHotConstCount);
  hot_consts.push_back(tracked);
  for (int i = 1; i < kHotConstCount; ++i) {
    hot_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(11000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : hot_consts) {
      auto* user = builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
      if (load == tracked) {
        tracked_users.push_back(user);
      }
    }
  }
  builder->Create<ReturnInst>(0, table);

  ASSERT_LE(ComputePreliminaryRegisterUsage(root),
            Register::kMaxRegistersLimit);
  const unsigned initial_child_usage = ComputePreliminaryRegisterUsage(child);
  ASSERT_GT(initial_child_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(child, tracked_literal), 1u);
  ASSERT_TRUE(InstructionUsesValueForTest(tracked_users.front(), tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_LE(ComputePreliminaryRegisterUsage(root),
            Register::kMaxRegistersLimit);
  EXPECT_EQ(ComputePreliminaryRegisterUsage(child), initial_child_usage);
  EXPECT_EQ(CountLoadConstWithLiteral(child, tracked_literal), 1u);
  EXPECT_TRUE(InstructionUsesValueForTest(tracked_users.front(), tracked));
  EXPECT_TRUE(InstructionUsesValueForTest(tracked_users.back(), tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationDoesNotRewriteOverflowingChildFunction) {
  auto* root = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_root_under_limit_child_over_"
      "limit");
  auto* child = CreateChildFunctionForRematerializationTest(
      ir_ctx.get(), mod, root,
      "load_const_rematerialization_only_child_rewritten");
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToStart(&root->Front());
  auto* root_table = builder->Create<NewTableInst>(0);
  constexpr int kRootConstCount = 64;
  constexpr int kRootRounds = 4;
  auto* root_tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1888888), TypeOp::CreateInt32(builder));
  Literal* root_literal = root_tracked->GetConst();

  std::vector<LoadConstInst*> root_consts;
  root_consts.reserve(kRootConstCount);
  root_consts.push_back(root_tracked);
  for (int i = 1; i < kRootConstCount; ++i) {
    root_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(12000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> root_tracked_users;
  root_tracked_users.reserve(kRootRounds);
  for (int round = 0; round < kRootRounds; ++round) {
    for (auto* load : root_consts) {
      auto* user = builder->Create<SetTableConstStringKeyInst>(
          0, root_table,
          builder->GetLiteralUint32(static_cast<uint32_t>(round)), load);
      if (load == root_tracked) {
        root_tracked_users.push_back(user);
      }
    }
  }
  builder->Create<ReturnInst>(0, root_table);

  auto* child_entry = &child->Front();
  builder->SetInsertionPointToStart(child_entry);
  auto* child_table = builder->Create<NewTableInst>(0);
  constexpr int kChildConstCount = 270;
  constexpr int kChildRounds = 4;
  auto* child_tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1999999), TypeOp::CreateInt32(builder));
  Literal* child_literal = child_tracked->GetConst();

  std::vector<LoadConstInst*> child_consts;
  child_consts.reserve(kChildConstCount);
  child_consts.push_back(child_tracked);
  for (int i = 1; i < kChildConstCount; ++i) {
    child_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(13000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> child_tracked_users;
  child_tracked_users.reserve(kChildRounds);
  for (int round = 0; round < kChildRounds; ++round) {
    for (auto* load : child_consts) {
      auto* user = builder->Create<SetTableConstStringKeyInst>(
          0, child_table,
          builder->GetLiteralUint32(static_cast<uint32_t>(kRootRounds + round)),
          load);
      if (load == child_tracked) {
        child_tracked_users.push_back(user);
      }
    }
  }
  builder->Create<ReturnInst>(0, child_table);

  const unsigned initial_root_usage = ComputePreliminaryRegisterUsage(root);
  const unsigned initial_child_usage = ComputePreliminaryRegisterUsage(child);
  ASSERT_LE(initial_root_usage, Register::kMaxRegistersLimit);
  ASSERT_GT(initial_child_usage, Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(root, root_literal), 1u);
  ASSERT_EQ(CountLoadConstWithLiteral(child, child_literal), 1u);
  ASSERT_TRUE(
      InstructionUsesValueForTest(root_tracked_users.front(), root_tracked));
  ASSERT_TRUE(
      InstructionUsesValueForTest(child_tracked_users.front(), child_tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_EQ(ComputePreliminaryRegisterUsage(root), initial_root_usage);
  EXPECT_EQ(CountLoadConstWithLiteral(root, root_literal), 1u);
  EXPECT_TRUE(
      InstructionUsesValueForTest(root_tracked_users.front(), root_tracked));
  EXPECT_TRUE(
      InstructionUsesValueForTest(root_tracked_users.back(), root_tracked));

  EXPECT_EQ(ComputePreliminaryRegisterUsage(child), initial_child_usage);
  EXPECT_EQ(CountLoadConstWithLiteral(child, child_literal), 1u);
  EXPECT_TRUE(
      InstructionUsesValueForTest(child_tracked_users.front(), child_tracked));
  EXPECT_TRUE(
      InstructionUsesValueForTest(child_tracked_users.back(), child_tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationSkipsChildFunctionWithPhiUsers) {
  auto* root = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod, "load_const_rematerialization_child_skip_phi_root");
  auto* child = CreateChildFunctionForRematerializationTest(
      ir_ctx.get(), mod, root, "load_const_rematerialization_child_skip_phi");
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToStart(&root->Front());
  auto* root_value = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(43), TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, root_value);

  auto* entry = &child->Front();
  auto* join =
      builder->CreateBlock(child->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1999991), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;
  std::vector<LoadConstInst*> filler_consts;
  filler_consts.reserve(kHotConstCount - 1);
  for (int i = 1; i < kHotConstCount; ++i) {
    filler_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(14000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    tracked_users.push_back(builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
        tracked));
    for (auto* load : filler_consts) {
      builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
    }
  }

  builder->Create<BranchInst>(0, join);
  builder->SetInsertionPointToStart(join);
  PhiInst::ValueListType values;
  PhiInst::BlockListType blocks;
  auto* phi = builder->Create<PhiInst>(0, values, blocks);
  phi->AddEntry(tracked, entry);
  builder->Create<ReturnInst>(0, phi);

  ASSERT_GT(ComputePreliminaryRegisterUsage(child),
            Register::kMaxRegistersLimit);
  ASSERT_EQ(phi->GetNumEntries(), 1u);

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_EQ(CountLoadConstWithLiteral(child, tracked_literal), 1u);
  for (auto* user : tracked_users) {
    EXPECT_TRUE(InstructionUsesValueForTest(user, tracked));
  }
  EXPECT_EQ(phi->GetNumEntries(), 1u);
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationSkipsChildFunctionWithBlockingUsers) {
  auto* root = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod,
      "load_const_rematerialization_child_skip_blocking_root");
  auto* child = CreateChildFunctionForRematerializationTest(
      ir_ctx.get(), mod, root,
      "load_const_rematerialization_child_skip_blocking");
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToStart(&root->Front());
  auto* root_value = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(45), TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, root_value);

  auto* entry = &child->Front();
  builder->SetInsertionPointToStart(entry);

  auto* table = builder->Create<NewTableInst>(0);
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1999993), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  constexpr int kHotConstCount = 270;
  constexpr int kRounds = 4;
  std::vector<LoadConstInst*> filler_consts;
  filler_consts.reserve(kHotConstCount - 1);
  for (int i = 1; i < kHotConstCount; ++i) {
    filler_consts.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(16000 + i), TypeOp::CreateInt32(builder)));
  }

  std::vector<SetTableConstStringKeyInst*> tracked_users;
  tracked_users.reserve(kRounds);
  for (int round = 0; round < kRounds; ++round) {
    tracked_users.push_back(builder->Create<SetTableConstStringKeyInst>(
        0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
        tracked));
  }
  for (int round = 0; round < kRounds; ++round) {
    for (auto* load : filler_consts) {
      builder->Create<SetTableConstStringKeyInst>(
          0, table, builder->GetLiteralUint32(static_cast<uint32_t>(round)),
          load);
    }
  }
  auto* blocking_use = builder->Create<ToStringInst>(0, tracked);
  builder->Create<ReturnInst>(0, blocking_use);

  ASSERT_GT(ComputePreliminaryRegisterUsage(child),
            Register::kMaxRegistersLimit);
  ASSERT_TRUE(InstructionUsesValueForTest(blocking_use, tracked));

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_EQ(CountLoadConstWithLiteral(child, tracked_literal), 1u);
  for (auto* user : tracked_users) {
    EXPECT_TRUE(InstructionUsesValueForTest(user, tracked));
  }
  EXPECT_TRUE(InstructionUsesValueForTest(blocking_use, tracked));
}

TEST_F(LEPUSIRLoadConstRematerializationTest,
       LoadConstRematerializationSkipsCrossBlockChildUserBlocks) {
  auto* root = CreateRootFunctionForRematerializationTest(
      ir_ctx.get(), mod, "load_const_rematerialization_child_cross_block_root");
  auto* child = CreateChildFunctionForRematerializationTest(
      ir_ctx.get(), mod, root,
      "load_const_rematerialization_child_cross_block");
  auto* builder = ir_ctx->GetOpBuilder();

  builder->SetInsertionPointToStart(&root->Front());
  auto* root_value = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(44), TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, root_value);

  auto* entry = &child->Front();
  auto* then_bb =
      builder->CreateBlock(child->GetSingleRegion(), BlockType::BT_INST, {});
  auto* else_bb =
      builder->CreateBlock(child->GetSingleRegion(), BlockType::BT_INST, {});
  builder->SetInsertionPointToStart(entry);

  constexpr int kConstCount = 270;
  auto* tracked = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1999992), TypeOp::CreateInt32(builder));
  Literal* tracked_literal = tracked->GetConst();

  std::vector<LoadConstInst*> values;
  values.reserve(kConstCount);
  values.push_back(tracked);
  for (int i = 1; i < kConstCount; ++i) {
    values.push_back(builder->Create<LoadConstInst>(
        0, builder->GetLiteralInt32(15000 + i), TypeOp::CreateInt32(builder)));
  }
  builder->Create<CondBranchInst>(0, builder->GetLiteralBool(true), then_bb,
                                  else_bb);

  builder->SetInsertionPointToStart(then_bb);
  auto* then_table = builder->Create<NewTableInst>(0);
  SetTableConstStringKeyInst* then_tracked_user = nullptr;
  for (int i = 0; i < kConstCount; ++i) {
    auto* user = builder->Create<SetTableConstStringKeyInst>(
        0, then_table, builder->GetLiteralUint32(static_cast<uint32_t>(i)),
        values[i]);
    if (values[i] == tracked) {
      then_tracked_user = user;
    }
  }
  builder->Create<ReturnInst>(0, then_table);

  builder->SetInsertionPointToStart(else_bb);
  auto* else_table = builder->Create<NewTableInst>(0);
  SetTableConstStringKeyInst* else_tracked_user = nullptr;
  for (int i = 0; i < kConstCount; ++i) {
    auto* user = builder->Create<SetTableConstStringKeyInst>(
        0, else_table,
        builder->GetLiteralUint32(static_cast<uint32_t>(kConstCount + i)),
        values[i]);
    if (values[i] == tracked) {
      else_tracked_user = user;
    }
  }
  builder->Create<ReturnInst>(0, else_table);

  ASSERT_NE(then_tracked_user, nullptr);
  ASSERT_NE(else_tracked_user, nullptr);
  ASSERT_GT(ComputePreliminaryRegisterUsage(child),
            Register::kMaxRegistersLimit);
  ASSERT_EQ(CountLoadConstWithLiteral(child, tracked_literal), 1u);

  LoadConstRematerializationPass pass(ir_ctx.get());
  EXPECT_FALSE(pass.RunOnModule(mod));

  EXPECT_GT(ComputePreliminaryRegisterUsage(child),
            Register::kMaxRegistersLimit);
  EXPECT_EQ(CountLoadConstWithLiteral(child, tracked_literal), 1u);
  EXPECT_EQ(CountLoadConstWithLiteralInBlock(entry, tracked_literal), 1u);
  EXPECT_EQ(CountLoadConstWithLiteralInBlock(then_bb, tracked_literal), 0u);
  EXPECT_EQ(CountLoadConstWithLiteralInBlock(else_bb, tracked_literal), 0u);
  EXPECT_TRUE(InstructionUsesValueForTest(then_tracked_user, tracked));
  EXPECT_TRUE(InstructionUsesValueForTest(else_tracked_user, tracked));
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
