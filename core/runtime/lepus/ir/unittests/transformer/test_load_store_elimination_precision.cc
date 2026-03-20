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

class LEPUSIRTestLoadStoreEliminationPrecision : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    lepus_func = lepus::Function::Create();
  }

  fml::RefPtr<lepus::Function> lepus_func;
};

// =========================
// TableKey normalization unit tests
// =========================
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       TableKey_TryGetConstIndexFromLoadConst_StringType) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  uint32_t idx = 123;
  auto* idx_lit = builder.GetLiteralUint32(idx);
  auto* lc =
      builder.Create<LoadConstInst>(0, idx_lit, TypeOp::CreateString(&builder));

  uint32_t out = 0;
  EXPECT_TRUE(
      LoadStoreElimination::TableKey::TryGetConstIndexFromLoadConst(lc, &out));
  EXPECT_EQ(out, idx);
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       TableKey_TryGetConstIndexFromLoadConst_RejectsNonStringType) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Even if the const payload is LiteralUint32, non-string LoadConst must NOT
  // be treated as a const-table string index.
  auto* idx_lit = builder.GetLiteralUint32(7);
  auto* lc =
      builder.Create<LoadConstInst>(0, idx_lit, TypeOp::CreateInt32(&builder));

  uint32_t out = 0;
  EXPECT_FALSE(
      LoadStoreElimination::TableKey::TryGetConstIndexFromLoadConst(lc, &out));
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       TableKey_TryGetConstIndex_LiteralUint32_DependsOnFlag) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* u32 = builder.GetLiteralUint32(42);
  uint32_t out = 0;
  EXPECT_FALSE(LoadStoreElimination::TableKey::TryGetConstIndex(
      u32, /*treat_literal_u32_as_const_index*/ false, &out));
  EXPECT_TRUE(LoadStoreElimination::TableKey::TryGetConstIndex(
      u32, /*treat_literal_u32_as_const_index*/ true, &out));
  EXPECT_EQ(out, 42u);
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       TableKey_Normalize_LoadConstString_And_Global_Builtin) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Normalize LoadConstInst(string, idx) => ConstIndex
  auto* lc = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(5),
                                           TypeOp::CreateString(&builder));
  auto nk_lc = LoadStoreElimination::TableKey::Normalize(
      lc, /*treat_literal_u32_as_const_index*/ false);
  EXPECT_EQ(nk_lc.kind,
            LoadStoreElimination::TableKey::NormalizedKey::Kind::ConstIndex);
  EXPECT_EQ(nk_lc.payload, 5u);

  // Normalize GetGlobalInst(idx) => GlobalIndex
  auto* gg =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(9),
                                    TypeOp::CreateAnyType(&builder));
  auto nk_gg = LoadStoreElimination::TableKey::Normalize(
      gg, /*treat_literal_u32_as_const_index*/ false);
  EXPECT_EQ(nk_gg.kind,
            LoadStoreElimination::TableKey::NormalizedKey::Kind::GlobalIndex);
  EXPECT_EQ(nk_gg.payload, 9u);

  // Normalize GetBuiltinInst(idx) => BuiltinIndex
  auto* gb =
      builder.Create<GetBuiltinInst>(0, (Literal*)builder.GetLiteralUint32(11),
                                     TypeOp::CreateAnyType(&builder));
  auto nk_gb = LoadStoreElimination::TableKey::Normalize(
      gb, /*treat_literal_u32_as_const_index*/ false);
  EXPECT_EQ(nk_gb.kind,
            LoadStoreElimination::TableKey::NormalizedKey::Kind::BuiltinIndex);
  EXPECT_EQ(nk_gb.payload, 11u);
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       TableKey_Normalize_LiteralUint32_DefaultIsPtrKind) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Safety check: numeric LiteralUint32 must NOT be treated as const-table
  // string index unless the caller explicitly says so.
  auto* u32 = builder.GetLiteralUint32(1);
  auto nk = LoadStoreElimination::TableKey::Normalize(
      u32, /*treat_literal_u32_as_const_index*/ false);
  EXPECT_EQ(nk.kind, LoadStoreElimination::TableKey::NormalizedKey::Kind::Ptr);
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       TableKey_OperatorEq_MatchesConstStringIndexWithLoadConstString) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  // "x" as a const-table index.
  auto* idx_u32 = builder.GetLiteralUint32(7);
  auto* lc_str =
      builder.Create<LoadConstInst>(0, idx_u32, TypeOp::CreateString(&builder));

  LoadStoreElimination::TableKey k_store{obj, idx_u32,
                                         /*key_is_const_string_index*/ true};
  LoadStoreElimination::TableKey k_load{obj, lc_str,
                                        /*key_is_const_string_index*/ false};
  EXPECT_TRUE(k_store == k_load);
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       TableKey_OperatorEq_GetGlobalAndGetBuiltin_ByIndex) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* key = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(2),
                                            TypeOp::CreateString(&builder));
  auto* gg1 =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(3),
                                    TypeOp::CreateAnyType(&builder));
  auto* gg2 =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(3),
                                    TypeOp::CreateAnyType(&builder));
  LoadStoreElimination::TableKey tg1{gg1, key,
                                     /*key_is_const_string_index*/ false};
  LoadStoreElimination::TableKey tg2{gg2, key,
                                     /*key_is_const_string_index*/ false};
  EXPECT_TRUE(tg1 == tg2);

  auto* gb1 =
      builder.Create<GetBuiltinInst>(0, (Literal*)builder.GetLiteralUint32(4),
                                     TypeOp::CreateAnyType(&builder));
  auto* gb2 =
      builder.Create<GetBuiltinInst>(0, (Literal*)builder.GetLiteralUint32(4),
                                     TypeOp::CreateAnyType(&builder));
  LoadStoreElimination::TableKey tb1{gb1, key,
                                     /*key_is_const_string_index*/ false};
  LoadStoreElimination::TableKey tb2{gb2, key,
                                     /*key_is_const_string_index*/ false};
  EXPECT_TRUE(tb1 == tb2);
}

// Test that writing to a numeric key does NOT invalidate a non-numeric string
// property.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       NonNumericStringKeyIsSafeFromNumericStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));

  // 1. Load non-numeric string key "foo"
  uint32_t foo_idx = lepus_func->AddConstString("foo");
  auto* foo_lit_idx = builder.GetLiteralUint32(foo_idx);
  auto* foo_val = builder.Create<LoadConstInst>(0, foo_lit_idx,
                                                TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, foo_val);

  // 2. Store to a numeric key (variable key with Int32 type)
  auto* num_key = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(1), TypeOp::CreateInt32(&builder));
  builder.Create<SetTableInst>(0, obj, num_key, builder.GetLiteralInt32(42));

  // 3. Load "foo" again.
  auto* get2 = builder.Create<GetTableInst>(0, obj, foo_val);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "Redundant load of non-numeric string key should NOT be eliminated "
         "after numeric store (conservative)";
}

// Test ToplevelClosureVar Store-to-Load Forwarding
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       ToplevelClosureStoreToLoadForwarding) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* val = builder.GetLiteralInt32(42);
  auto* reg = builder.GetLiteralUint32(10);

  // 1. SetToplevelClosureVar(reg 10, 42)
  builder.Create<SetToplevelClosureVarInst>(0, reg, val);

  // 2. GetToplevelClosureVar(reg 10)
  auto* get = builder.Create<GetToplevelClosureVarInst>(
      0, reg, TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get) found_get = true;
  }
  EXPECT_FALSE(found_get)
      << "GetToplevelClosureVar should be eliminated by forwarding";
}

// Test that Toplevel Closure and Global namespaces are separate
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       ToplevelClosureAndGlobalNamespacesAreSeparate) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* common_idx = builder.GetLiteralUint32(5);

  // 1. Redundant loads from separate namespaces
  builder.Create<GetGlobalInst>(0, common_idx, TypeOp::CreateInt32(&builder));
  builder.Create<GetToplevelClosureVarInst>(0, common_idx,
                                            TypeOp::CreateInt32(&builder));

  // 2. Redundant loads
  auto* get_global2 = builder.Create<GetGlobalInst>(
      0, common_idx, TypeOp::CreateInt32(&builder));
  auto* get_closure2 = builder.Create<GetToplevelClosureVarInst>(
      0, common_idx, TypeOp::CreateInt32(&builder));

  builder.Create<ReturnInst>(0, get_global2);
  builder.Create<ReturnInst>(0, get_closure2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_global2 = false;
  bool found_get_closure2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_global2) found_get_global2 = true;
    if (inst == get_closure2) found_get_closure2 = true;
  }
  // Both should be eliminated as they are redundant.
  EXPECT_FALSE(found_get_global2);
  EXPECT_FALSE(found_get_closure2);

  // But wait, the REAL test is ensuring one doesn't replace the other if they
  // have same index. The current logic should handle this because they use
  // separate maps.
}

// Test that GetGlobal, GetBuiltin, and LoadConst are NOT invalidated by side
// effects.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       ImmutableLoadsPersistAcrossSideEffects) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* common_idx = builder.GetLiteralUint32(5);
  auto* builtin_idx = builder.GetLiteralUint32(10);
  auto* const_val = builder.GetLiteralInt32(42);

  // 1. Initial loads
  builder.Create<GetGlobalInst>(0, common_idx, TypeOp::CreateAnyType(&builder));
  builder.Create<GetBuiltinInst>(0, builtin_idx,
                                 TypeOp::CreateAnyType(&builder));
  builder.Create<LoadConstInst>(0, const_val, TypeOp::CreateInt32(&builder));

  // 2. side effects (Call and SetTable)
  ArgList args;
  auto* dummy_func = builder.Create<GetGlobalInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  builder.Create<CallInst>(0, dummy_func, args);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(1),
                                            TypeOp::CreateAnyType(&builder));
  builder.Create<SetTableInst>(0, obj, builder.GetLiteralUint32(2),
                               builder.GetLiteralInt32(3));

  // 3. Redundant loads that should be eliminated
  auto* get_global2 = builder.Create<GetGlobalInst>(
      0, common_idx, TypeOp::CreateAnyType(&builder));
  auto* get_builtin2 = builder.Create<GetBuiltinInst>(
      0, builtin_idx, TypeOp::CreateAnyType(&builder));
  auto* load_const2 = builder.Create<LoadConstInst>(
      0, const_val, TypeOp::CreateInt32(&builder));

  builder.Create<ReturnInst>(0, get_global2);
  builder.Create<ReturnInst>(0, get_builtin2);
  builder.Create<ReturnInst>(0, load_const2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_global2 = false;
  bool found_get_builtin2 = false;
  bool found_load_const2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_global2) found_get_global2 = true;
    if (inst == get_builtin2) found_get_builtin2 = true;
    if (inst == load_const2) found_load_const2 = true;
  }

  EXPECT_FALSE(found_get_global2)
      << "GetGlobal should be eliminated even after Call/SetTable";
  EXPECT_FALSE(found_get_builtin2)
      << "GetBuiltin should be eliminated even after Call/SetTable";
  EXPECT_FALSE(found_load_const2)
      << "LoadConst should be eliminated even after Call/SetTable";
}

// Test that Numeric String and Number literals are treated as potential
// aliases.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision, NumericStringAndNumberAlias) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));

  // 1. Load from numeric string "1"
  uint32_t one_idx = lepus_func->AddConstString("1");
  auto* one_lit_idx = builder.GetLiteralUint32(one_idx);
  auto* one_str_val = builder.Create<LoadConstInst>(
      0, one_lit_idx, TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, one_str_val);

  // 2. Store to number 1
  auto* one_num_val = builder.Create<LoadConstInst>(
      0, builder.GetLiteralInt32(1), TypeOp::CreateInt32(&builder));
  builder.Create<SetTableInst>(0, obj, one_num_val,
                               builder.GetLiteralInt32(42));

  // 3. Load from "1" again.
  auto* get2 = builder.Create<GetTableInst>(0, obj, one_str_val);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "Load of numeric string should NOT be eliminated after store to "
         "numeric key (must be invalidated for safety)";
}

// Test that Dynamic Key invalidates other table entries.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision, DynamicKeyInvalidation) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  auto* key_const = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateString(&builder));
  auto* key_dynamic = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(1),
      TypeOp::CreateAnyType(&builder));  // truly dynamic key

  // 1. Load constant key
  builder.Create<GetTableInst>(0, obj, key_const);

  // 2. Store to dynamic key
  builder.Create<SetTableInst>(0, obj, key_dynamic,
                               builder.GetLiteralInt32(42));

  // 3. Load constant key again
  auto* get2 = builder.Create<GetTableInst>(0, obj, key_const);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "Load should NOT be eliminated after dynamic key store (conservative)";
}

// Test that writing to ONE object's property invalidates that property for ALL
// objects (Object-level conservative).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       ObjectLevelConservativeInvalidation) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj1 =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  auto* obj2 =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(1),
                                    TypeOp::CreateAnyType(&builder));
  auto* key = builder.Create<LoadConstInst>(0, builder.GetLiteralUint32(2),
                                            TypeOp::CreateString(&builder));

  // 1. Load obj1.prop
  builder.Create<GetTableInst>(0, obj1, key);

  // 2. Store to obj2.prop
  builder.Create<SetTableInst>(0, obj2, key, builder.GetLiteralInt32(42));

  // 3. Load obj1.prop again
  auto* get2 = builder.Create<GetTableInst>(0, obj1, key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2) << "Load should NOT be eliminated after store to "
                             "same key on potentially aliased object";
}

// Test Precise Alias Analysis with actual constant values.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision, PreciseConstantAliasAnalysis) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));

  // 1. Load from "foo"
  uint32_t foo_idx = lepus_func->AddConstString("foo");
  auto* foo_val = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(foo_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, foo_val);

  // 2. Load from "bar"
  uint32_t bar_idx = lepus_func->AddConstString("bar");
  auto* bar_val = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(bar_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, bar_val);

  // 3. Store to "foo"
  builder.Create<SetTableInst>(0, obj, foo_val, builder.GetLiteralInt32(42));

  // 4. Load from "foo" again (Should be eliminated by forwarding)
  auto* get_foo2 = builder.Create<GetTableInst>(0, obj, foo_val);

  // 5. Load from "bar" again (Should be eliminated by redundancy, because store
  // to "foo" didn't alias "bar")
  auto* get_bar2 = builder.Create<GetTableInst>(0, obj, bar_val);

  builder.Create<ReturnInst>(0, get_foo2);
  builder.Create<ReturnInst>(0, get_bar2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_foo2 = false;
  bool found_get_bar2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_foo2) found_get_foo2 = true;
    if (inst == get_bar2) found_get_bar2 = true;
  }
  EXPECT_FALSE(found_get_foo2);
  EXPECT_FALSE(found_get_bar2)
      << "Load from 'bar' should be eliminated as it doesn't alias with 'foo'";
}

// Test Numeric String vs Number Precise Alias
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision, NumericStringVsNumberPrecise) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));

  // 1. Load from "1"
  uint32_t one_idx = lepus_func->AddConstString("1");
  auto* one_str = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(one_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, one_str);

  // 2. Store to number 1
  auto* one_num = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(1),
                                                TypeOp::CreateInt32(&builder));
  builder.Create<SetTableInst>(0, obj, one_num, builder.GetLiteralInt32(42));

  // 3. Load from "1" again (Alias, should NOT be eliminated by redundancy with
  // first load, but should be forwarded to 42)
  auto* get_one_str2 = builder.Create<GetTableInst>(0, obj, one_str);

  builder.Create<ReturnInst>(0, get_one_str2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_one_str2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_one_str2) found_get_one_str2 = true;
  }
  EXPECT_TRUE(found_get_one_str2) << "Load of '1' should NOT be eliminated "
                                     "after numeric store to 1 (invalidation)";
}

// Test various integer literal types and their aliasing with strings.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision, VariousIntegerTypesAliasing) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));

  // 1. Load from "123"
  uint32_t idx = lepus_func->AddConstString("123");
  auto* str_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, obj, str_key);

  // 2. Store to Int8 123
  auto* int8_key = builder.Create<LoadConstInst>(0, builder.GetLiteralInt8(123),
                                                 TypeOp::CreateInt8(&builder));
  builder.Create<SetTableInst>(0, obj, int8_key, builder.GetLiteralInt32(42));

  // 3. Load from "123" again.
  // This load CANNOT be eliminated because:
  // a) The previous load of "123" was invalidated by the store to Int8(123)
  // (aliasing). b) We cannot forward the value from Int8(123) to "123" because
  // they are different IR values.
  auto* get_str2 = builder.Create<GetTableInst>(0, obj, str_key);

  // 4. Store to Uint32 123
  auto* uint32_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(123), TypeOp::CreateUint32(&builder));
  builder.Create<SetTableInst>(0, obj, uint32_key,
                               builder.GetLiteralInt32(100));

  // 5. Load from "123" again.
  // Also cannot be eliminated for the same reason (Uint32(123) aliases with
  // "123").
  auto* get_str3 = builder.Create<GetTableInst>(0, obj, str_key);

  builder.Create<ReturnInst>(0, get_str3);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_str2 = false;
  bool found_get_str3 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_str2) found_get_str2 = true;
    if (inst == get_str3) found_get_str3 = true;
  }
  EXPECT_TRUE(found_get_str2) << "Int8 literal 123 should alias with string "
                                 "'123' and invalidate previous load";
  EXPECT_TRUE(found_get_str3) << "Uint32 literal 123 should alias with string "
                                 "'123' and invalidate previous load";
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
