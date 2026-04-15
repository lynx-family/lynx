// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestLoadStoreEliminationComplex : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       NestedLoopConditionalInvalidation) {
  // entry -> l1_header -> l2_header -> l2_cond -> l2_store -> l2_header
  //                                   -> l2_header
  //                       -> l1_latch -> exit
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "nested_cond_test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  auto* l1_header = builder.CreateBlock(func->GetSingleRegion(),
                                        BlockType::BT_INST, 0, "l1_header");
  auto* l2_header = builder.CreateBlock(func->GetSingleRegion(),
                                        BlockType::BT_INST, 0, "l2_header");
  auto* l2_cond = builder.CreateBlock(func->GetSingleRegion(),
                                      BlockType::BT_INST, 0, "l2_cond");
  auto* l2_store = builder.CreateBlock(func->GetSingleRegion(),
                                       BlockType::BT_INST, 0, "l2_store");
  auto* l1_latch = builder.CreateBlock(func->GetSingleRegion(),
                                       BlockType::BT_INST, 0, "l1_latch");
  auto* exit = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                   0, "exit");

  builder.SetInsertionPointToEnd(entry);
  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  builder.Create<BranchInst>(0, l1_header);

  builder.SetInsertionPointToEnd(l1_header);
  // Outer loop load
  auto* get_outer = builder.Create<GetTableInst>(
      0, obj, (Literal*)builder.GetLiteralInt32(100));
  builder.Create<BranchInst>(0, l2_header);

  builder.SetInsertionPointToEnd(l2_header);
  // This load should be preserved because l2_header is a join point.
  auto* get_inner = builder.Create<GetTableInst>(
      0, obj, (Literal*)builder.GetLiteralInt32(100));
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), l2_cond,
                                 l1_latch);

  builder.SetInsertionPointToEnd(l2_cond);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(false), l2_store,
                                 l2_header);

  builder.SetInsertionPointToEnd(l2_store);
  builder.Create<SetTableInst>(0, obj, (Literal*)builder.GetLiteralInt32(100),
                               builder.GetLiteralInt32(99));
  builder.Create<BranchInst>(0, l2_header);

  builder.SetInsertionPointToEnd(l1_latch);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(false), l1_header,
                                 exit);

  builder.SetInsertionPointToEnd(exit);
  builder.Create<ReturnInst>(0, get_outer);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_inner = false;
  for (auto* inst : l2_header->InstRange()) {
    if (inst == get_inner) found_get_inner = true;
  }
  EXPECT_TRUE(found_get_inner)
      << "Inner load should be preserved due to join point";
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex, CrossTypeLiteralAliasing) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "cross_type_test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Use mock lepus function to provide constant values for LiteralUint32 if
  // needed.
  auto lepus_func = lepus::Function::Create();
  uint32_t str1_idx = lepus_func->AddConstString("1");
  func->Init(lepus_func);  // init creates a region

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));

  // LoadConst with string "1"
  auto* key_str = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(str1_idx), TypeOp::CreateString(&builder));
  // Literal Int32 1
  auto* key_int = builder.GetLiteralInt32(1);

  [[maybe_unused]] auto* get1 = builder.Create<GetTableInst>(0, obj, key_str);

  // Set with integer 1. Should invalidate Get with string "1".
  builder.Create<SetTableInst>(0, obj, key_int, builder.GetLiteralInt32(42));

  auto* get2 = builder.Create<GetTableInst>(0, obj, key_str);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "String key load should be invalidated by numeric key store";
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       JoinKeepsDominatingLoadAcrossConstKeyStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "join_keep_dominating_load";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table strings so that LiteralUint32 in
  // SetTableConstStringKeyInst can be resolved to a real string key.
  auto lepus_func = lepus::Function::Create();
  uint32_t padding_idx = lepus_func->AddConstString("padding");
  uint32_t k35_idx = lepus_func->AddConstString("35");
  func->Init(lepus_func);  // init creates a region

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  auto* then_bb = builder.CreateBlock(func->GetSingleRegion(),
                                      BlockType::BT_INST, 0, "then");
  auto* else_bb = builder.CreateBlock(func->GetSingleRegion(),
                                      BlockType::BT_INST, 0, "else");
  auto* merge = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "merge");

  builder.SetInsertionPointToEnd(entry);
  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  auto* key_padding = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(padding_idx), TypeOp::CreateString(&builder));
  auto* get1 = builder.Create<GetTableInst>(0, obj, key_padding);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), then_bb,
                                 else_bb);

  builder.SetInsertionPointToEnd(then_bb);
  builder.Create<BranchInst>(0, merge);

  builder.SetInsertionPointToEnd(else_bb);
  builder.Create<BranchInst>(0, merge);

  builder.SetInsertionPointToEnd(merge);
  auto* tmp = builder.Create<NewTableInst>(0);
  // Write to a fresh object with a const-string key (does not affect
  // obj[padding]).
  builder.Create<SetTableConstStringKeyInst>(
      0, tmp, builder.GetLiteralUint32(k35_idx), builder.GetLiteralInt32(1));
  auto* get2 = builder.Create<GetTableInst>(0, obj, key_padding);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  bool found_get1 = false;
  for (auto* inst : merge->InstRange()) {
    if (inst == get2) found_get2 = true;
    if (inst == get1) found_get1 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Redundant GetTable in join block should be eliminated";
  // get1 lives in entry, but we still assert it's not removed.
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
  }
  EXPECT_TRUE(found_get1);
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       StoreToLoadForwarding_WithGetTableConstStringKeyInst_AndLoadConstKey) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "forwarding_gettable_const_string_key";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table strings so that the const-string index can be
  // interpreted consistently.
  auto lepus_func = lepus::Function::Create();
  uint32_t foo_idx = lepus_func->AddConstString("foo");
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  auto* key_lit = builder.GetLiteralUint32(foo_idx);
  auto* key_load_const =
      builder.Create<LoadConstInst>(0, key_lit, TypeOp::CreateString(&builder));
  auto* val = builder.GetLiteralInt32(42);

  // Store via normal SetTableInst (key is LoadConstInst(string)).
  builder.Create<SetTableInst>(0, obj, key_load_const, val);
  // Load via GetTableConstStringKeyInst (key is const-table index literal).
  auto* get = builder.Create<GetTableConstStringKeyInst>(
      0, obj, key_lit, TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get) found_get = true;
  }
  EXPECT_FALSE(found_get) << "GetTableConstStringKeyInst should be forwarded "
                             "from SetTableInst with equivalent LoadConst key";

  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  EXPECT_EQ(ret->GetOperand(0), val);
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       RedundantGetTableConstStringKeyInstIsEliminated) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "redundant_gettable_const_string_key";
  auto* func = builder.Create<FuncOp>(0, name);

  auto lepus_func = lepus::Function::Create();
  uint32_t padding_idx = lepus_func->AddConstString("padding");
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  auto* key_lit = builder.GetLiteralUint32(padding_idx);

  auto* get1 = builder.Create<GetTableConstStringKeyInst>(
      0, obj, key_lit, TypeOp::CreateAnyType(&builder));
  auto* get2 = builder.Create<GetTableConstStringKeyInst>(
      0, obj, key_lit, TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get1);
  EXPECT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       StoreToLoadForwardingDifferentObjects) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "forwarding_test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* o1 = builder.Create<NewTableInst>(0);
  auto* o2 = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val = builder.GetLiteralInt32(100);

  // o1[1] = 100
  builder.Create<SetTableInst>(0, o1, key, val);

  // o2[1] = 200 (should NOT invalidate o1[1] cache because o1 and o2 are
  // distinct allocations)
  builder.Create<SetTableInst>(0, o2, key, builder.GetLiteralInt32(200));

  // Load o1[1]. Should be eliminated and replaced by 100.
  auto* get = builder.Create<GetTableInst>(0, o1, key);
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get) found_get = true;
  }
  EXPECT_FALSE(found_get)
      << "Load from o1 should be forwarded despite store to o2";

  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  EXPECT_EQ(ret->GetOperand(0), val);
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex, InvalidationByExecuteJS) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "execute_js_test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  auto* key = builder.GetLiteralInt32(1);

  builder.Create<GetTableInst>(0, obj, key);

  // Generic instruction with ExecuteJS side effect
  // We need a real instruction with ExecuteJS. CallInst is one.
  ArgList args;
  builder.Create<CallInst>(0, builder.GetLiteralInt32(0), args);
  // CallInst by default has ExecuteJS side effect.

  auto* get2 = builder.Create<GetTableInst>(0, obj, key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "Load should be invalidated by CallInst (ExecuteJS)";
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex, CrossBlockAllocationAlias) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "cross_block_alloc";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  auto* next = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                   0, "next");

  builder.SetInsertionPointToEnd(entry);
  auto* o1 = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<SetTableInst>(0, o1, key, builder.GetLiteralInt32(100));
  builder.Create<BranchInst>(0, next);

  builder.SetInsertionPointToEnd(next);
  auto* o2 = builder.Create<NewTableInst>(0);  // Distinct from o1
  builder.Create<SetTableInst>(0, o2, key, builder.GetLiteralInt32(200));

  // Load o1[1]. Should still be 100 and eliminated.
  auto* get = builder.Create<GetTableInst>(0, o1, key);
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : next->InstRange()) {
    if (inst == get) found_get = true;
  }
  EXPECT_FALSE(found_get) << "Load from o1 in 'next' block should be "
                             "eliminated despite NewTable o2";
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       ConstStringKeyStoreInvalidatesNumericKeyOnSameObject) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "const_string_key_invalidate_numeric_key";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table strings so that SetTableConstStringKeyInst can be
  // decoded to a real string key.
  auto lepus_func = lepus::Function::Create();
  uint32_t one_idx = lepus_func->AddConstString("1");
  func->Init(lepus_func);  // Init creates a region

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  auto* key_num = builder.GetLiteralInt32(1);

  // 1) Load obj[1] to populate cache.
  builder.Create<GetTableInst>(0, obj, key_num);

  // 2) Store obj["1"] via const-string-key store. This must invalidate obj[1].
  builder.Create<SetTableConstStringKeyInst>(
      0, obj, builder.GetLiteralUint32(one_idx), builder.GetLiteralInt32(42));

  // 3) Load obj[1] again. Should NOT be eliminated due to invalidation.
  auto* get2 = builder.Create<GetTableInst>(0, obj, key_num);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2) << "Numeric key load must be invalidated by "
                             "const-string-key store on same object";
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       ConstStringKeyStoreDoesNotInvalidateNumericKeyForNonNumericString) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name =
      "const_string_key_no_invalidate_numeric_key_non_numeric_string";
  auto* func = builder.Create<FuncOp>(0, name);

  auto lepus_func = lepus::Function::Create();
  uint32_t foo_idx = lepus_func->AddConstString("foo");
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  auto* key_num = builder.GetLiteralInt32(1);

  // 1) Load obj[1] to populate cache.
  auto* get1 = builder.Create<GetTableInst>(0, obj, key_num);

  // 2) Store obj["foo"] via const-string-key store.
  // With constant ToPropertyKey equivalence, "foo" does NOT alias key 1.
  builder.Create<SetTableConstStringKeyInst>(
      0, obj, builder.GetLiteralUint32(foo_idx), builder.GetLiteralInt32(7));

  // 3) Load obj[1] again. Should be eliminated (redundant) and replaced by
  // get1.
  auto* get2 = builder.Create<GetTableInst>(0, obj, key_num);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Non-numeric const-string-key store must not invalidate numeric key 1";

  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  EXPECT_EQ(ret->GetOperand(0), get1);
}

TEST_F(LEPUSIRTestLoadStoreEliminationComplex,
       DynamicKeyStoreOnFreshAllocDoesNotInvalidateOtherObjectCache) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "dynamic_key_store_fresh_alloc_no_cross_invalidation";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* o1 = builder.Create<NewTableInst>(0);
  auto* o2 = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  auto* val = builder.GetLiteralInt32(100);

  // Populate cache for o1[1].
  builder.Create<SetTableInst>(0, o1, key, val);

  // Store to a truly dynamic key on a fresh allocation (o2). This should only
  // affect entries on o2, and must NOT clear o1's cache.
  auto* dynamic_key = builder.Create<GetGlobalInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  builder.Create<SetTableInst>(0, o2, dynamic_key,
                               builder.GetLiteralInt32(200));

  // Load o1[1] again: should be forwarded to 100 and eliminated.
  auto* get = builder.Create<GetTableInst>(0, o1, key);
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get) found_get = true;
  }
  EXPECT_FALSE(found_get) << "Dynamic-key store on fresh alloc must not "
                             "invalidate other object's cache";

  auto* ret = llvh::dyn_cast<ReturnInst>(entry->GetTerminator());
  EXPECT_EQ(ret->GetOperand(0), val);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
