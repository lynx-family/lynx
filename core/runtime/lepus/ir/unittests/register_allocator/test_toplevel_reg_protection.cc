// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRToplevelRegProtectionTest : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }

  virtual void TearDown(void) {}

  FuncOp* createToplevelFunction(std::string name) {
    auto builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

    auto* func_op = builder->Create<FuncOp>(0, name);
    func_op->SetTopLevelFunction();
    mod->SetRootFunction(func_op);
    EXPECT_NE(nullptr, func_op);

    auto region = builder->CreateRegion(func_op);
    builder->CreateBlock(region, BlockType::BT_INST, {});

    return func_op;
  }

  RegisterAllocator* runRegisterAllocation(FuncOp* func) {
    auto* ra = new RegisterAllocator(func);
    PostOrderAnalysis po(func);
    llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
    ra->Preallocate();
    ra->Allocate(order);
    return ra;
  }
};

TEST_F(LEPUSIRToplevelRegProtectionTest, ProtectToplevelRegFromReuse) {
  auto* func = createToplevelFunction("toplevel_main");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // 1. Create a toplevel variable instruction
  auto* toplevel_val = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(100), TypeOp::CreateInt32(builder));
  ir_ctx->InsertToplevelValue(toplevel_val, 0);  // Register 0

  // 2. Use it immediately so its interval ends early if not protected
  auto* use1 = builder->Create<BinaryOperatorInst>(
      0, toplevel_val, toplevel_val, ValueKind::BinaryAddInstKind,
      TypeOp::CreateInt32(builder));

  // 3. Create many more instructions to force register allocation
  // These instructions should NOT reuse register 0 because it's preallocated
  // for toplevel_val
  std::vector<Instruction*> more_insts;
  for (int i = 0; i < 10; ++i) {
    auto* c = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(i),
                                             TypeOp::CreateInt32(builder));
    more_insts.push_back(c);
  }

  builder->Create<ReturnInst>(0, use1);

  // 4. Run register allocation
  auto* ra = runRegisterAllocation(func);

  // 5. Verify toplevel_val has register 0
  ASSERT_TRUE(ra->IsAllocated(toplevel_val));
  auto toplevel_reg = ra->GetRegister(toplevel_val);
  EXPECT_EQ(toplevel_reg.GetIndex(), 0u);
  EXPECT_TRUE(toplevel_val->IsFixReg());

  // 6. Verify NO other instruction uses register 0
  for (auto* inst : more_insts) {
    if (ra->IsAllocated(inst)) {
      EXPECT_NE(ra->GetRegister(inst).GetIndex(), 0u)
          << "Instruction should not reuse preallocated toplevel register 0";
    }
  }

  delete ra;
}

TEST_F(LEPUSIRToplevelRegProtectionTest,
       IRContextToplevelVariablesIterateInAscendingKeyOrder) {
  auto* func = createToplevelFunction("toplevel_ctx_order");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* v0 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(10),
                                            TypeOp::CreateInt32(builder));
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(11),
                                            TypeOp::CreateInt32(builder));
  auto* v2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(12),
                                            TypeOp::CreateInt32(builder));

  // Intentionally insert out-of-order.
  ir_ctx->InsertToplevelValue(v2, 2);
  ir_ctx->InsertToplevelValue(v0, 0);
  ir_ctx->InsertToplevelValue(v1, 1);
  builder->Create<ReturnInst>(0, v0);

  const auto& tv = ir_ctx->GetToplevelVariables();
  ASSERT_EQ(tv.size(), 3u);

  unsigned expected = 0;
  for (const auto& kv : tv) {
    EXPECT_EQ(kv.first, expected) << "IRContext::GetToplevelVariables must "
                                     "iterate keys in ascending order";
    ++expected;
  }
}

TEST_F(LEPUSIRToplevelRegProtectionTest,
       ToplevelPreallocateIsDeterministicAndMatchesOriginalIndex) {
  auto* func = createToplevelFunction("toplevel_prealloc_deterministic");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Create a few toplevel variables, but insert them into IRContext in a
  // deliberately shuffled order to simulate non-deterministic map iteration.
  auto* v0 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(10),
                                            TypeOp::CreateInt32(builder));
  auto* v1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(11),
                                            TypeOp::CreateInt32(builder));
  auto* v2 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(12),
                                            TypeOp::CreateInt32(builder));

  ir_ctx->InsertToplevelValue(v2, 2);
  ir_ctx->InsertToplevelValue(v0, 0);
  ir_ctx->InsertToplevelValue(v1, 1);

  builder->Create<ReturnInst>(0, v0);

  auto* ra = runRegisterAllocation(func);

  ASSERT_TRUE(ra->IsAllocated(v0));
  ASSERT_TRUE(ra->IsAllocated(v1));
  ASSERT_TRUE(ra->IsAllocated(v2));

  // Toplevel variables must be preallocated to their original indices to keep
  // VM toplevel register layout stable.
  EXPECT_EQ(ra->GetRegister(v0).GetIndex(), 0u);
  EXPECT_EQ(ra->GetRegister(v1).GetIndex(), 1u);
  EXPECT_EQ(ra->GetRegister(v2).GetIndex(), 2u);

  EXPECT_TRUE(v0->IsFixReg());
  EXPECT_TRUE(v1->IsFixReg());
  EXPECT_TRUE(v2->IsFixReg());

  delete ra;
}

TEST_F(LEPUSIRToplevelRegProtectionTest,
       ToplevelPreallocateMatchesKeyForManyVariables) {
  auto* func = createToplevelFunction("toplevel_prealloc_many");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Build many toplevel variables in reverse order to maximize the chance of
  // catching non-deterministic iteration if the container ever regresses.
  static constexpr unsigned kN = 64;
  std::vector<Instruction*> vals(kN, nullptr);
  for (int i = static_cast<int>(kN) - 1; i >= 0; --i) {
    auto* v = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(i),
                                             TypeOp::CreateInt32(builder));
    vals[static_cast<unsigned>(i)] = v;
    ir_ctx->InsertToplevelValue(v, static_cast<unsigned>(i));
  }

  builder->Create<ReturnInst>(0, vals[0]);
  auto* ra = runRegisterAllocation(func);

  for (unsigned i = 0; i < kN; ++i) {
    ASSERT_NE(vals[i], nullptr);
    ASSERT_TRUE(ra->IsAllocated(vals[i]));
    EXPECT_EQ(ra->GetRegister(vals[i]).GetIndex(), i)
        << "toplevel variable must stay on its original register index";
    EXPECT_TRUE(vals[i]->IsFixReg());
  }

  delete ra;
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
