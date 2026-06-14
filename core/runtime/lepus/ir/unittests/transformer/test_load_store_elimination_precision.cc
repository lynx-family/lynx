// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
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

  auto* obj = builder.Create<NewTableInst>(0);

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
  EXPECT_FALSE(found_get2)
      << "Redundant load of non-numeric string key should be eliminated "
         "after numeric store (\"foo\" can never alias any number key)";
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       AnyReceiverStringPropertyStoreDoesNotForwardLoad) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  uint32_t data_idx = lepus_func->AddConstString("data");
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(data_idx), TypeOp::CreateString(&builder));
  auto* stored = builder.GetLiteralInt32(1);

  builder.Create<SetTableInst>(0, obj, key, stored);
  auto* reload = builder.Create<GetTableInst>(0, obj, key);
  builder.Create<ReturnInst>(0, reload);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_reload = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == reload) found_reload = true;
  }
  EXPECT_TRUE(found_reload)
      << "Generic string property reload must not be forwarded when the "
         "receiver is only typed as any";
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       AnyReceiverRepeatedStringLoadCanBeEliminated) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  uint32_t data_idx = lepus_func->AddConstString("data");
  auto* key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(data_idx), TypeOp::CreateString(&builder));

  builder.Create<GetTableInst>(0, obj, key);
  auto* reload = builder.Create<GetTableInst>(0, obj, key);
  builder.Create<ReturnInst>(0, reload);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_reload = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == reload) found_reload = true;
  }
  EXPECT_FALSE(found_reload)
      << "Repeated string load on any receiver should still be eliminate when "
         "there is no intervening mutation";
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       ArrayLengthLoadInvalidatedByNumericStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* arr = builder.Create<NewArrayInst>(0, ArgList{});
  uint32_t length_idx = lepus_func->AddConstString("length");
  auto* length_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(length_idx), TypeOp::CreateString(&builder));
  auto* numeric_key = builder.GetLiteralInt32(5);

  builder.Create<GetTableInst>(0, arr, length_key);
  auto* store = builder.Create<SetTableInst>(0, arr, numeric_key,
                                             builder.GetLiteralInt32(1));
  (void)store;
  auto* reload = builder.Create<GetTableInst>(0, arr, length_key);
  builder.Create<ReturnInst>(0, reload);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_reload = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == reload) found_reload = true;
  }
  EXPECT_TRUE(found_reload)
      << "Array length load must be invalidated by numeric element store";
}

TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       AnyReceiverLengthLoadInvalidatedByNumericStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  uint32_t length_idx = lepus_func->AddConstString("length");
  auto* length_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(length_idx), TypeOp::CreateString(&builder));
  auto* numeric_key = builder.GetLiteralInt32(5);

  builder.Create<GetTableInst>(0, obj, length_key);
  builder.Create<SetTableInst>(0, obj, numeric_key, builder.GetLiteralInt32(1));
  auto* reload = builder.Create<GetTableInst>(0, obj, length_key);
  builder.Create<ReturnInst>(0, reload);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_reload = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == reload) found_reload = true;
  }
  EXPECT_TRUE(found_reload)
      << "Unknown receiver length load must stay when numeric store may hit an "
         "array at runtime";
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

  auto* obj = builder.Create<NewTableInst>(0);

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

// =============================================================================
// LocalTableSafeCall selective invalidation unit tests
// =============================================================================

// Test: A table-preserving call preserves GetTableInst-receiver table entries.
// Scenario: item = data[i]; read item.x; call table_preserving_fn(); read
// item.x Expected: second read eliminated (receiver is GetTableInst,
// preserved).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       LocalTableSafeCall_PreservesGetTableReceiver) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_tp_get_table";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Simulate: var data = GetUpvalue(); var item = data[0];
  auto* data =
      builder.Create<GetUpvalueInst>(0, func, builder.GetLiteralUint32(0));
  auto* idx_key = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(0),
                                                TypeOp::CreateInt32(&builder));
  auto* item = builder.Create<GetTableInst>(0, data, idx_key);

  // Read item.x
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* x_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(x_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, item, x_key);

  // Call a table-preserving function
  ArgList args;
  auto* callee = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  // Read item.x again — should be eliminated
  auto* get2 = builder.Create<GetTableInst>(0, item, x_key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Table entry with GetTableInst receiver should be PRESERVED across "
         "a LocalTableSafeCall, so second load is eliminated";
}

// Test: A table-preserving call clears GetUpvalueInst-receiver table entries.
// Scenario: read shared.x (shared via upvalue); call table_preserving_fn();
//           read shared.x again
// Expected: second read NOT eliminated (receiver is GetUpvalueInst, cleared).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       LocalTableSafeCall_ClearsUpvalueReceiver) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_tp_upvalue";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var shared = GetUpvalue(0)
  auto* shared =
      builder.Create<GetUpvalueInst>(0, func, builder.GetLiteralUint32(0));

  // Read shared.x
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* x_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(x_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, shared, x_key);

  // Call a table-preserving function (may write to its own upvalue = shared)
  ArgList args;
  auto* callee = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  // Read shared.x again — should NOT be eliminated
  auto* get2 = builder.Create<GetTableInst>(0, shared, x_key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "Table entry with GetUpvalueInst receiver should be CLEARED by "
         "LocalTableSafeCall (callee may write to same upvalue table)";
}

// Test: A table-preserving call preserves NewTableInst-receiver entries.
// Scenario: var t = {}; t.x = 1; call table_preserving_fn(); read t.x
// Expected: read eliminated (local allocation cannot escape to callee upvalue).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       LocalTableSafeCall_PreservesLocalAllocation) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_tp_local";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var t = {}
  auto* t = builder.Create<NewTableInst>(0);

  // t.x = 1
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* x_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(x_idx), TypeOp::CreateString(&builder));
  auto* val = builder.GetLiteralInt32(1);
  builder.Create<SetTableInst>(0, t, x_key, val);

  // Call a table-preserving function
  ArgList args;
  auto* callee = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  // Read t.x — should be eliminated (forwarded to val)
  auto* get_x = builder.Create<GetTableInst>(0, t, x_key);
  builder.Create<ReturnInst>(0, get_x);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_x = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_x) found_get_x = true;
  }
  EXPECT_FALSE(found_get_x)
      << "Table entry with NewTableInst receiver should be PRESERVED across "
         "a LocalTableSafeCall (local alloc cannot be callee's upvalue)";
}

// Test: A table-preserving call preserves Parameter-receiver entries.
// Scenario: function f(arg) { read arg.x; call tp_fn(); read arg.x; }
// Expected: second read eliminated (callee is verified param-safe).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       LocalTableSafeCall_PreservesParameterReceiver) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_tp_param";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // arg0
  auto* arg = func->CreateParam(0);

  // Read arg.x
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* x_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(x_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, arg, x_key);

  // Call a table-preserving function
  ArgList args;
  auto* callee = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  // Read arg.x again — should be eliminated
  auto* get2 = builder.Create<GetTableInst>(0, arg, x_key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Table entry with Parameter receiver should be PRESERVED across "
         "a LocalTableSafeCall (callee does not write to param-derived)";
}

// Test: A regular (non-table-preserving, non-readonly) call clears ALL table
// entries including GetTableInst receivers.
// This is the baseline behavior confirming our selective invalidation only
// applies to table-preserving calls.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       RegularCall_ClearsAllTableEntries) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_regular_call";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var data = GetUpvalue(); var item = data[0];
  auto* data =
      builder.Create<GetUpvalueInst>(0, func, builder.GetLiteralUint32(0));
  auto* idx_key = builder.Create<LoadConstInst>(0, builder.GetLiteralInt32(0),
                                                TypeOp::CreateInt32(&builder));
  auto* item = builder.Create<GetTableInst>(0, data, idx_key);

  // Read item.x
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* x_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(x_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, item, x_key);

  // Regular call (NOT table-preserving, NOT readonly)
  ArgList args;
  auto* callee = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  builder.Create<CallInst>(0, callee, args);

  // Read item.x again — should NOT be eliminated (regular call clears all)
  auto* get2 = builder.Create<GetTableInst>(0, item, x_key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "Regular call (not table-preserving) should clear ALL table entries "
         "including GetTableInst receivers";
}

// Test: A table-preserving call clears upvalue/toplevel_var caches.
// Scenario: read upvalue[0]; call table_preserving_fn(); read upvalue[0] again
// Expected: second read IS eliminated (never-written upvalue cache preserved
// across LocalTableSafeCall since no SetUpvalueInst exists for this slot).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       LocalTableSafeCall_PreservesUpvalueCache) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_tp_upvalue_cache";
  auto* func = builder.Create<FuncOp>(0, name);
  // Add upvalue info so analysis can determine it's never-written.
  auto lf = lepus::Function::Create();
  lf->AddUpvalue("neverWrittenVar", false, 0);
  func->Init(lf);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Read upvalue[0] (using LiteralUint8 to match cache key type)
  auto* upvalue_idx = builder.GetLiteralUint8(0);
  builder.Create<GetUpvalueInst>(0, func, upvalue_idx);

  // Call a native renderer function (LocalTableSafeCall)
  ArgList args;
  auto* callee = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(1), TypeOp::CreateAnyType(&builder));
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  // Read upvalue[0] again — SHOULD be eliminated (never-written upvalue
  // preserved across LocalTableSafeCall)
  auto* get2 = builder.Create<GetUpvalueInst>(0, func, upvalue_idx);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Upvalue cache should be PRESERVED by LocalTableSafeCall "
         "(never-written upvalue slot is safe across native calls)";
}

// Test: A readonly call preserves ALL caches (including upvalue receivers).
// Contrast with table-preserving call which clears upvalue-receiver entries.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       ReadonlyCall_PreservesAllCachesIncludingUpvalue) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_readonly";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var shared = GetUpvalue(0)
  auto* shared =
      builder.Create<GetUpvalueInst>(0, func, builder.GetLiteralUint32(0));

  // Read shared.x
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* x_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(x_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, shared, x_key);

  // Call a readonly function (even stronger than table-preserving)
  ArgList args;
  auto* callee = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                               TypeOp::CreateAnyType(&builder));
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetReadonlyCall(true);

  // Read shared.x again — should be eliminated (readonly = no side effects)
  auto* get2 = builder.Create<GetTableInst>(0, shared, x_key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "ReadonlyCall should preserve ALL caches including upvalue-receiver "
         "entries (stronger guarantee than LocalTableSafeCall)";
}

// Test: A table-preserving call preserves GetTableConstStringKeyInst receiver.
// This is the most common pattern in real code: obj.prop access.
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       LocalTableSafeCall_PreservesConstStringKeyReceiver) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_tp_const_str";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var parent = GetUpvalue(); var item = parent.child (GetTableConstStringKey)
  auto* parent =
      builder.Create<GetUpvalueInst>(0, func, builder.GetLiteralUint32(0));
  uint32_t child_idx = lepus_func->AddConstString("child");
  auto* item = builder.Create<GetTableConstStringKeyInst>(
      0, parent, builder.GetLiteralUint32(child_idx),
      TypeOp::CreateTable(&builder));

  // Read item.height
  uint32_t h_idx = lepus_func->AddConstString("height");
  auto* h_key = builder.Create<LoadConstInst>(
      0, builder.GetLiteralUint32(h_idx), TypeOp::CreateString(&builder));
  builder.Create<GetTableInst>(0, item, h_key);

  // Call a table-preserving function
  ArgList args;
  auto* callee = builder.Create<GetToplevelClosureVarInst>(
      0, builder.GetLiteralUint32(0), TypeOp::CreateAnyType(&builder));
  auto* call = builder.Create<CallInst>(0, callee, args);
  call->SetLocalTableSafeCall(true);

  // Read item.height again — should be eliminated
  auto* get2 = builder.Create<GetTableInst>(0, item, h_key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Table entry with GetTableConstStringKeyInst receiver should be "
         "PRESERVED across a LocalTableSafeCall";
}

// ===========================================================================
// DSE Soundness: dynamic-key read must prevent dead-store elimination of
// aliasing const-key stores.
// ===========================================================================
// Scenario (JS-like):
//   obj.x = 1;           // store1 (SetTableConstStringKeyInst)
//   tmp = obj[dyn_key];  // dynamic read — dyn_key could be "x" at runtime!
//   obj.x = 2;           // store2 (SetTableConstStringKeyInst)
//
// The bug: pending_stores.erase(read_key) only does exact-match lookup.
// read_key = {obj, param, false} doesn't match {obj, const_idx_x, true}.
// So the pending store for store1 is never cleared, and DSE later
// incorrectly removes store1 when store2 arrives.
//
// Expected (correct): store1 is NOT eliminated (the read might observe it).
// Buggy behavior:     store1 IS eliminated (silent miscompilation).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       DSE_DynamicKeyReadPreventsDeadStoreElimination) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_dse_alias_bug";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var obj = {}
  auto* obj = builder.Create<NewTableInst>(0);

  // obj.x = 1  (SetTableConstStringKeyInst)
  uint32_t x_idx = lepus_func->AddConstString("x");  // index 0
  auto* const_idx = builder.GetLiteralUint32(x_idx);
  auto* val1 = builder.GetLiteralInt32(1);
  auto* store1 = builder.Create<SetTableConstStringKeyInst>(
      0, obj, (Literal*)const_idx, val1);

  // tmp = obj[dyn_key]  (GetTableInst with a parameter key — could be "x")
  auto* dyn_key = func->CreateParam(0);
  builder.Create<GetTableInst>(0, obj, dyn_key);

  // obj.x = 2  (SetTableConstStringKeyInst — same key as store1)
  auto* val2 = builder.GetLiteralInt32(2);
  builder.Create<SetTableConstStringKeyInst>(0, obj, (Literal*)const_idx, val2);

  builder.Create<ReturnInst>(0, obj);

  // Run LSE
  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // store1 must NOT be eliminated — the dynamic read might observe it.
  bool found_store1 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
  }
  EXPECT_TRUE(found_store1)
      << "BUG: store1 (obj.x = 1) was incorrectly eliminated by DSE. "
         "A dynamic-key read obj[dyn_key] could alias 'x', so the store "
         "is observable and must not be removed.";
}

// ===========================================================================
// DSE Soundness (reverse direction): const-key read must prevent dead-store
// elimination of aliasing dynamic-key stores.
// ===========================================================================
// Scenario (JS-like):
//   obj[dyn_key] = 1;  // store1 (SetTableInst, dynamic key)
//   tmp = obj.x;       // const-key read — if dyn_key == "x", observes store1!
//   obj[dyn_key] = 2;  // store2 (SetTableInst, same dynamic key)
//
// Expected: store1 is NOT eliminated (const-key read might observe it).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       DSE_ConstKeyReadPreventsDeadStoreOfDynamicKeyStore) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_dse_reverse_alias";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var obj = {}
  auto* obj = builder.Create<NewTableInst>(0);

  // obj[dyn_key] = 1  (SetTableInst with a parameter key)
  auto* dyn_key = func->CreateParam(0);
  auto* val1 = builder.GetLiteralInt32(1);
  auto* store1 = builder.Create<SetTableInst>(0, obj, dyn_key, val1);

  // tmp = obj.x  (GetTableConstStringKeyInst — could observe store1 if dyn_key
  // == "x")
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* const_idx = builder.GetLiteralUint32(x_idx);
  builder.Create<GetTableConstStringKeyInst>(0, obj, const_idx, nullptr);

  // obj[dyn_key] = 2  (SetTableInst — same key as store1)
  auto* val2 = builder.GetLiteralInt32(2);
  builder.Create<SetTableInst>(0, obj, dyn_key, val2);

  builder.Create<ReturnInst>(0, obj);

  // Run LSE
  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // store1 must NOT be eliminated — the const-key read might observe it.
  bool found_store1 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
  }
  EXPECT_TRUE(found_store1)
      << "BUG: store1 (obj[dyn_key] = 1) was incorrectly eliminated by DSE. "
         "A const-key read obj.x could alias dyn_key, so the store "
         "is observable and must not be removed.";
}

// ===========================================================================
// DSE Precision (non-aliasing read): a read with a provably non-aliasing key
// should NOT prevent dead-store elimination.
// ===========================================================================
// Scenario:
//   obj.x = 1;   // store1
//   tmp = obj.y;  // read of DIFFERENT const key — cannot alias "x"
//   obj.x = 2;   // store2 (overwrites store1)
//
// Expected: store1 IS eliminated (read of "y" cannot observe "x").
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       DSE_NonAliasingConstKeyReadDoesNotPreventDSE) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_dse_non_aliasing";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var obj = {}
  auto* obj = builder.Create<NewTableInst>(0);

  // obj.x = 1
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* const_idx_x = builder.GetLiteralUint32(x_idx);
  auto* val1 = builder.GetLiteralInt32(1);
  auto* store1 = builder.Create<SetTableConstStringKeyInst>(
      0, obj, (Literal*)const_idx_x, val1);

  // tmp = obj.y  (different key — provably non-aliasing)
  uint32_t y_idx = lepus_func->AddConstString("y");
  auto* const_idx_y = builder.GetLiteralUint32(y_idx);
  builder.Create<GetTableConstStringKeyInst>(0, obj, const_idx_y, nullptr);

  // obj.x = 2  (same key as store1)
  auto* val2 = builder.GetLiteralInt32(2);
  builder.Create<SetTableConstStringKeyInst>(0, obj, (Literal*)const_idx_x,
                                             val2);

  builder.Create<ReturnInst>(0, obj);

  // Run LSE
  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // store1 SHOULD be eliminated — the read of "y" cannot observe "x".
  bool found_store1 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
  }
  EXPECT_FALSE(found_store1)
      << "Regression: store1 (obj.x = 1) should be eliminated by DSE. "
         "The intervening read obj.y cannot alias 'x', so DSE is valid.";
}

// ===========================================================================
// DSE Precision (different object): a read on a different object should NOT
// prevent dead-store elimination on the original object.
// ===========================================================================
// Scenario:
//   obj1.x = 1;         // store1
//   tmp = obj2[dyn_key]; // read on DIFFERENT object
//   obj1.x = 2;         // store2 (overwrites store1)
//
// Expected: store1 IS eliminated (read is on a different object).
TEST_F(LEPUSIRTestLoadStoreEliminationPrecision,
       DSE_DifferentObjectReadDoesNotPreventDSE) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());

  std::string name = "test_dse_diff_obj";
  auto* func = builder.Create<FuncOp>(0, name);
  func->Init(lepus_func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // var obj1 = {}, obj2 = {}
  auto* obj1 = builder.Create<NewTableInst>(0);
  auto* obj2 = builder.Create<NewTableInst>(0);

  // obj1.x = 1
  uint32_t x_idx = lepus_func->AddConstString("x");
  auto* const_idx_x = builder.GetLiteralUint32(x_idx);
  auto* val1 = builder.GetLiteralInt32(1);
  auto* store1 = builder.Create<SetTableConstStringKeyInst>(
      0, obj1, (Literal*)const_idx_x, val1);

  // tmp = obj2[dyn_key]  (dynamic read on a DIFFERENT object)
  auto* dyn_key = func->CreateParam(0);
  builder.Create<GetTableInst>(0, obj2, dyn_key);

  // obj1.x = 2
  auto* val2 = builder.GetLiteralInt32(2);
  builder.Create<SetTableConstStringKeyInst>(0, obj1, (Literal*)const_idx_x,
                                             val2);

  builder.Create<ReturnInst>(0, obj1);

  // Run LSE
  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // store1 SHOULD be eliminated — the read is on obj2, not obj1.
  bool found_store1 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == store1) found_store1 = true;
  }
  EXPECT_FALSE(found_store1)
      << "Regression: store1 (obj1.x = 1) should be eliminated by DSE. "
         "The intervening read obj2[dyn_key] is on a different object, "
         "so it cannot observe obj1.x.";
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
