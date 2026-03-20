// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "base/include/value/base_string.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/load_store_elimination.h"
#include "core/runtime/lepus/ir/transformer/mir/type_specification.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestLoadStoreElimination : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

namespace {
// A dummy builtin implementation for unit tests.
RestrictedValue DummyReadonlyBuiltin(VMContext*) { return RestrictedValue(); }
}  // namespace

class LEPUSIRTestLoadStoreEliminationReadonlyBuiltinCall
    : public ::testing::Test {
 protected:
  void SetUp() override {
    vm_ctx_ = std::make_unique<lepus::VMContext>();
    // Minimal registration: only names are used by LSE to classify read-only
    // builtins, so the implementation body is irrelevant here.
    RegisterBuiltinFunction(vm_ctx_.get(), constants::kParseFloat,
                            &DummyReadonlyBuiltin);
    RegisterBuiltinFunction(vm_ctx_.get(), "print", &DummyReadonlyBuiltin);

    ir_ctx_ = std::make_unique<IRContext>(vm_ctx_.get());
    mod_ = ir_ctx_->GetMainMod();
    ASSERT_NE(nullptr, mod_);
    ASSERT_NE(nullptr, ir_ctx_->GetOpBuilder());
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

class LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall
    : public ::testing::Test {
 protected:
  void SetUp() override {
    vm_ctx_ = std::make_unique<lepus::VMContext>();
    // Register builtins to populate VMContext::global() with e.g. Math/console.
    RegisterBuiltin(vm_ctx_.get());

    ir_ctx_ = std::make_unique<IRContext>(vm_ctx_.get());
    mod_ = ir_ctx_->GetMainMod();
    ASSERT_NE(nullptr, mod_);
    ASSERT_NE(nullptr, ir_ctx_->GetOpBuilder());
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

TEST_F(LEPUSIRTestLoadStoreElimination, GlobalRedundancy) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* global_idx = builder.GetLiteralUint32(5);
  auto* get1 = builder.Create<GetGlobalInst>(0, (Literal*)global_idx,
                                             TypeOp::CreateAnyType(&builder));
  auto* get2 = builder.Create<GetGlobalInst>(
      0, (Literal*)global_idx, TypeOp::CreateAnyType(&builder));  // Redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get1);
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreElimination, ToplevelClosureRedundancy) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* reg_idx = builder.GetLiteralInt32(10);
  auto* get1 = builder.Create<GetToplevelClosureVarInst>(
      0, (Literal*)reg_idx, TypeOp::CreateAnyType(&builder));
  auto* get2 = builder.Create<GetToplevelClosureVarInst>(
      0, (Literal*)reg_idx, TypeOp::CreateAnyType(&builder));  // Redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get1);
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreElimination, UpvalueRedundancy) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* index = builder.GetLiteralInt32(1);
  auto* get1 = builder.Create<GetUpvalueInst>(0, func, (Literal*)index);
  auto* get2 =
      builder.Create<GetUpvalueInst>(0, func, (Literal*)index);  // Redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get1);
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreElimination, StoreToLoadForwardingGlobal) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* reg_idx = builder.GetLiteralUint32(10);
  auto* val = builder.GetLiteralInt32(42);

  builder.Create<SetToplevelVarInst>(0, (Literal*)reg_idx, val);
  auto* get = builder.Create<GetToplevelVarInst>(
      0, (Literal*)reg_idx, TypeOp::CreateAnyType(&builder));
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get) found_get = true;
  }
  ASSERT_FALSE(found_get);  // Should be removed as it's redundant
}

TEST_F(LEPUSIRTestLoadStoreElimination, InvalidationByCall) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));
  auto* key = builder.GetLiteralUint32(5);
  builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call invalidates everything mutable (like tables)
  ArgList args;
  builder.Create<CallInst>(0, builder.GetLiteralInt32(0), args);

  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);  // Should NOT be removed
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyBuiltinCall,
       ReadonlyBuiltinCallDoesNotInvalidateCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // Prepare a cached load.
  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  auto* key = builder.GetLiteralInt32(5);
  auto* get1 = builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call a read-only builtin loaded via GetBuiltinInst.
  int parse_float_idx =
      vm_ctx_->builtin()->Search(base::String(constants::kParseFloat));
  ASSERT_GE(parse_float_idx, 0);
  auto* builtin_idx_lit =
      builder.GetLiteralUint32(static_cast<uint32_t>(parse_float_idx));
  auto* get_builtin = builder.Create<GetBuiltinInst>(
      0, (Literal*)builtin_idx_lit, TypeOp::CreateAnyType(&builder));
  ArgList args;
  builder.Create<CallInst>(0, get_builtin, args);

  // If caches are NOT invalidated by this call, this load is redundant.
  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get1);
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyBuiltinCall,
       NonReadonlyBuiltinCallStillInvalidatesCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);

  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  auto* key = builder.GetLiteralInt32(5);
  builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Call a builtin not in the read-only allowlist.
  int print_idx = vm_ctx_->builtin()->Search(base::String("print"));
  ASSERT_GE(print_idx, 0);
  auto* builtin_idx_lit =
      builder.GetLiteralUint32(static_cast<uint32_t>(print_idx));
  auto* get_builtin = builder.Create<GetBuiltinInst>(
      0, (Literal*)builtin_idx_lit, TypeOp::CreateAnyType(&builder));
  ArgList args;
  builder.Create<CallInst>(0, get_builtin, args);

  // Since caches are invalidated, this load must remain.
  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyMathMethodCallDoesNotInvalidateCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst(constants::kMathAbs).
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t abs_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kMathAbs)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // A cached load on a fresh table.
  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  // Call Math.abs via GetGlobalInst + GetTableInst +
  // LoadConstInst(constants::kMathAbs).
  int math_idx =
      vm_ctx_->global()->Search(base::String(constants::kGlobalMath));
  ASSERT_GE(math_idx, 0);
  auto* math_global = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(math_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* abs_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(abs_idx),
      TypeOp::CreateString(&builder));
  auto* abs_func = builder.Create<GetTableInst>(0, math_global, abs_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(123));
  builder.Create<CallInst>(0, abs_func, args);

  // If caches are NOT invalidated, this load is redundant.
  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyStringStaticMethodCallDoesNotInvalidateCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst(constants::kStringLength).
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t length_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kStringLength)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // A cached load on a fresh table.
  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  // Call String.length via the operand order produced by the compiler:
  //   GetTable(object=GetGlobal(String),
  //   prop=LoadConst(constants::kStringLength))
  int string_idx =
      vm_ctx_->global()->Search(base::String(constants::kGlobalString));
  ASSERT_GE(string_idx, 0);
  auto* string_global = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(string_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* length_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(length_idx),
      TypeOp::CreateString(&builder));
  auto* length_func =
      builder.Create<GetTableInst>(0, string_global, length_key);

  ArgList args;
  args.push_back(builder.GetLiteralInt32(123));
  builder.Create<CallInst>(0, length_func, args);

  // If caches are NOT invalidated, this load is redundant.
  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyBuiltinTablePropertyCallDoesNotInvalidateCaches_JSONParse) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst(constants::kJSONParse).
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t parse_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kJSONParse)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  int json_idx =
      vm_ctx_->builtin()->Search(base::String(constants::kBuiltinJSON));
  ASSERT_GE(json_idx, 0);
  auto* json_builtin = builder.Create<GetBuiltinInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(json_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* parse_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(parse_idx),
      TypeOp::CreateString(&builder));
  auto* parse_func = builder.Create<GetTableInst>(0, json_builtin, parse_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(0));
  builder.Create<CallInst>(0, parse_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyBuiltinTablePropertyCallDoesNotInvalidateCaches_DateNow) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst(constants::kDateNow).
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t now_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kDateNow)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  int date_idx =
      vm_ctx_->builtin()->Search(base::String(constants::kBuiltinDate));
  ASSERT_GE(date_idx, 0);
  auto* date_builtin = builder.Create<GetBuiltinInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(date_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* now_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(now_idx),
      TypeOp::CreateString(&builder));
  auto* now_func = builder.Create<GetTableInst>(0, date_builtin, now_key);
  ArgList args;
  builder.Create<CallInst>(0, now_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyBuiltinTablePropertyCallDoesNotInvalidateCaches_JSONStringify) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst(constants::kJSONStringify).
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t stringify_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kJSONStringify)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  int json_idx =
      vm_ctx_->builtin()->Search(base::String(constants::kBuiltinJSON));
  ASSERT_GE(json_idx, 0);
  auto* json_builtin = builder.Create<GetBuiltinInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(json_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* stringify_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(stringify_idx),
      TypeOp::CreateString(&builder));
  auto* stringify_func =
      builder.Create<GetTableInst>(0, json_builtin, stringify_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(0));
  builder.Create<CallInst>(0, stringify_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyBuiltinTablePropertyCallDoesNotInvalidateCaches_ObjectKeys) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst(constants::kObjectKeys).
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t keys_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kObjectKeys)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  int obj_idx =
      vm_ctx_->builtin()->Search(base::String(constants::kBuiltinObject));
  ASSERT_GE(obj_idx, 0);
  auto* object_builtin = builder.Create<GetBuiltinInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(obj_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* keys_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(keys_idx),
      TypeOp::CreateString(&builder));
  auto* keys_func = builder.Create<GetTableInst>(0, object_builtin, keys_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(0));
  builder.Create<CallInst>(0, keys_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ObjectAssignCallStillInvalidatesCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst(constants::kObjectAssign).
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t assign_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kObjectAssign)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  int obj_idx =
      vm_ctx_->builtin()->Search(base::String(constants::kBuiltinObject));
  ASSERT_GE(obj_idx, 0);
  auto* object_builtin = builder.Create<GetBuiltinInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(obj_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* assign_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(assign_idx),
      TypeOp::CreateString(&builder));
  auto* assign_func =
      builder.Create<GetTableInst>(0, assign_key, object_builtin);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(0));
  args.push_back(builder.GetLiteralInt32(1));
  builder.Create<CallInst>(0, assign_func, args);

  // This should remain because Object.assign must NOT be treated as read-only.
  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ConsoleLogCallStillInvalidatesCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Provide const-table for LoadConstInst("log").
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t log_idx =
      static_cast<uint32_t>(lepus_func->AddConstString(base::String("log")));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  int console_idx = vm_ctx_->global()->Search(base::String("console"));
  ASSERT_GE(console_idx, 0);
  auto* console_global = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(console_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* log_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(log_idx),
      TypeOp::CreateString(&builder));
  auto* log_func = builder.Create<GetTableInst>(0, log_key, console_global);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(1));
  builder.Create<CallInst>(0, log_func, args);

  // This should remain because console.log is not treated as read-only.
  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       StoreForwardingSurvivesReadonlyCallAndForwardsIntoConsoleLogArg) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Const table entries: "x", constants::kStringLength, "log".
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t x_idx =
      static_cast<uint32_t>(lepus_func->AddConstString(base::String("x")));
  uint32_t length_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kStringLength)));
  uint32_t log_idx =
      static_cast<uint32_t>(lepus_func->AddConstString(base::String("log")));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // obj = {}
  auto* obj = builder.Create<NewTableInst>(0);
  // obj.x = 1  (use const-string-key store)
  builder.Create<SetTableConstStringKeyInst>(
      0, obj, (Literal*)builder.GetLiteralUint32(x_idx),
      builder.GetLiteralInt32(1));

  // readonly call: String.length(a)
  int string_idx =
      vm_ctx_->global()->Search(base::String(constants::kGlobalString));
  ASSERT_GE(string_idx, 0);
  auto* string_global = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(string_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* length_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(length_idx),
      TypeOp::CreateString(&builder));
  // Call String.length via the operand order produced by the compiler.
  auto* length_func =
      builder.Create<GetTableInst>(0, string_global, length_key);
  ArgList length_args;
  length_args.push_back(builder.Create<LoadConstInst>(
      0,
      (Literal*)builder.GetLiteralUint32(
          static_cast<uint32_t>(lepus_func->AddConstString(base::String("")))),
      TypeOp::CreateString(&builder)));
  builder.Create<CallInst>(0, length_func, length_args);

  // console.log(obj.x)
  int console_idx = vm_ctx_->global()->Search(base::String("console"));
  ASSERT_GE(console_idx, 0);
  auto* console_global = builder.Create<GetGlobalInst>(
      0, (Literal*)builder.GetLiteralUint32(static_cast<uint32_t>(console_idx)),
      TypeOp::CreateAnyType(&builder));
  auto* log_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(log_idx),
      TypeOp::CreateString(&builder));
  auto* log_func = builder.Create<GetTableInst>(0, log_key, console_global);

  // The load for obj.x should be forwarded to constant 1.
  auto* x_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(x_idx),
      TypeOp::CreateString(&builder));
  auto* get_x = builder.Create<GetTableInst>(0, obj, x_key);
  ArgList log_args;
  log_args.push_back(get_x);
  builder.Create<CallInst>(0, log_func, log_args);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // After LSE, the GetTableInst(obj, "x") should be eliminated.
  bool found_get_x = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get_x) found_get_x = true;
  }
  ASSERT_FALSE(found_get_x);
}

TEST_F(LEPUSIRTestLoadStoreElimination, AliasInvalidationBySetTable) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj1 =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  auto* obj2 =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(1),
                                    TypeOp::CreateAnyType(&builder));
  auto* key = builder.GetLiteralInt32(100);

  // Load from obj1.x
  builder.Create<GetTableInst>(0, obj1, (Literal*)key);

  // Store to obj2.x. This must invalidate obj1.x because they might be the same
  // object.
  builder.Create<SetTableInst>(0, obj2, (Literal*)key,
                               builder.GetLiteralInt32(100));

  // Load from obj1.x again. Should NOT be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, obj1, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreElimination, LoopInvariantLoadInvalidation) {
  // Test that a load in a loop is NOT eliminated if there's a store in the
  // loop, even if the store is to a different property (conservative
  // invalidation).
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  auto* loop_header = builder.CreateBlock(func->GetSingleRegion(),
                                          BlockType::BT_INST, 0, "loop");
  auto* exit = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                   0, "exit");

  builder.SetInsertionPointToEnd(entry);
  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  builder.Create<BranchInst>(0, loop_header);

  builder.SetInsertionPointToEnd(loop_header);
  // In a join point (loop header), AvailableValues are cleared.
  auto* get = builder.Create<GetTableInst>(
      0, obj, (Literal*)builder.GetLiteralInt32(100));
  builder.Create<SetTableInst>(0, obj, (Literal*)builder.GetLiteralInt32(101),
                               get);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), loop_header,
                                 exit);

  builder.SetInsertionPointToEnd(exit);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  // The GetTable in the loop header should remain because it's a join point.
  bool found_get = false;
  for (auto* inst : loop_header->InstRange()) {
    if (inst == get) found_get = true;
  }
  ASSERT_TRUE(found_get);
}

TEST_F(LEPUSIRTestLoadStoreElimination, PhiNodeSafety) {
  // BB1: obj.x = 1; jmp BB3
  // BB2: obj.x = 2; jmp BB3
  // BB3: val = obj.x; ret val
  // 'val = obj.x' should NOT be eliminated because it depends on which branch
  // was taken.
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  auto* bb1 = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                  0, "bb1");
  auto* bb2 = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                  0, "bb2");
  auto* bb3 = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                  0, "bb3");

  builder.SetInsertionPointToEnd(entry);
  auto* obj =
      builder.Create<GetGlobalInst>(0, (Literal*)builder.GetLiteralUint32(0),
                                    TypeOp::CreateAnyType(&builder));
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), bb1, bb2);

  builder.SetInsertionPointToEnd(bb1);
  builder.Create<SetTableInst>(0, obj, (Literal*)builder.GetLiteralInt32(100),
                               builder.GetLiteralInt32(1));
  builder.Create<BranchInst>(0, bb3);

  builder.SetInsertionPointToEnd(bb2);
  builder.Create<SetTableInst>(0, obj, (Literal*)builder.GetLiteralInt32(100),
                               builder.GetLiteralInt32(2));
  builder.Create<BranchInst>(0, bb3);

  builder.SetInsertionPointToEnd(bb3);
  auto* get = builder.Create<GetTableInst>(
      0, obj, (Literal*)builder.GetLiteralInt32(100));
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : bb3->InstRange()) {
    if (inst == get) found_get = true;
  }
  ASSERT_TRUE(found_get);  // Should NOT be removed as bb3 is a join point.
}

TEST_F(LEPUSIRTestLoadStoreElimination, ContextSlotRedundancy) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* depth = builder.GetLiteralUint8(0);
  auto* index = builder.GetLiteralUint8(1);
  auto* get1 = builder.Create<GetContextSlotInst>(0, (LiteralUint8*)depth,
                                                  (LiteralUint8*)index);
  auto* get2 = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth, (LiteralUint8*)index);  // Redundant
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get1 = false;
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get1) found_get1 = true;
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get1);
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreElimination, ContextSlotForwarding) {
  OpBuilder builder;
  builder.SetModuleOp(mod);

  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* depth = builder.GetLiteralUint8(0);
  auto* index = builder.GetLiteralUint8(1);
  auto* val = builder.GetLiteralInt32(42);

  builder.Create<SetContextSlotInst>(0, (LiteralUint8*)depth,
                                     (LiteralUint8*)index, val);
  auto* get = builder.Create<GetContextSlotInst>(
      0, (LiteralUint8*)depth,
      (LiteralUint8*)index);  // Should be replaced by 'val'
  builder.Create<ReturnInst>(0, get);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get) found_get = true;
  }
  ASSERT_FALSE(found_get);
}

TEST_F(LEPUSIRTestLoadStoreElimination, NestedLoopInvalidation) {
  // Test that a load in an outer loop is invalidated by a store in an inner
  // loop. entry -> loop1_header -> loop2_header -> loop2_body -> loop2_header
  //                       -> loop1_latch -> exit
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "nested_loop_test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  auto* l1_header = builder.CreateBlock(func->GetSingleRegion(),
                                        BlockType::BT_INST, 0, "l1_header");
  auto* l2_header = builder.CreateBlock(func->GetSingleRegion(),
                                        BlockType::BT_INST, 0, "l2_header");
  auto* l2_body = builder.CreateBlock(func->GetSingleRegion(),
                                      BlockType::BT_INST, 0, "l2_body");
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
  // Initial load in outer loop
  builder.Create<GetTableInst>(0, obj, (Literal*)builder.GetLiteralInt32(100));
  builder.Create<BranchInst>(0, l2_header);

  builder.SetInsertionPointToEnd(l2_header);
  // Redundant load in inner loop
  auto* get_inner = builder.Create<GetTableInst>(
      0, obj, (Literal*)builder.GetLiteralInt32(100));
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(true), l2_body,
                                 l1_latch);

  builder.SetInsertionPointToEnd(l2_body);
  // Store in inner loop
  builder.Create<SetTableInst>(0, obj, (Literal*)builder.GetLiteralInt32(100),
                               builder.GetLiteralInt32(42));
  builder.Create<BranchInst>(0, l2_header);

  builder.SetInsertionPointToEnd(l1_latch);
  builder.Create<CondBranchInst>(0, builder.GetLiteralBool(false), l1_header,
                                 exit);

  builder.SetInsertionPointToEnd(exit);
  builder.Create<ReturnInst>(0, builder.GetLiteralInt32(0));

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get_inner = false;
  for (auto* inst : l2_header->InstRange()) {
    if (inst == get_inner) found_get_inner = true;
  }
  // Because l2_header is a join point, available values should be cleared.
  EXPECT_TRUE(found_get_inner)
      << "Load in inner loop header (join point) should NOT be eliminated";
}

TEST_F(LEPUSIRTestLoadStoreElimination, ArrayNumericVsStringAliasIR) {
  // Test that SetTable(obj, 1, val) invalidates GetTable(obj, "1") in IR
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "alias_test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<GetGlobalInst>(0, builder.GetLiteralUint32(0),
                                            TypeOp::CreateAnyType(&builder));

  // 1. GetTable(obj, "1")
  // For keysMayAlias to work with string vs number, it needs to see
  // LiteralString or equivalent. In our IR, LoadConst usually loads a value
  // from constant pool. We can also use Literal directly if we had a way to
  // create LiteralString in builder. Since builder doesn't have it, we'll mock
  // the aliasing check by using what keysMayAlias expects.

  // Check if SetTable with a non-constant key invalidates everything.
  auto* dynamic_key = builder.Create<GetGlobalInst>(
      0, builder.GetLiteralUint32(1), TypeOp::CreateAnyType(&builder));
  builder.Create<GetTableInst>(0, obj, builder.GetLiteralInt32(1));

  // 2. SetTable with dynamic key
  builder.Create<SetTableInst>(0, obj, dynamic_key,
                               builder.GetLiteralInt32(42));

  // 3. Load again. Must NOT be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, obj, builder.GetLiteralInt32(1));
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_TRUE(found_get2)
      << "Load should be invalidated by store to dynamic key";
}

TEST_F(LEPUSIRTestLoadStoreElimination, DistinctAllocationsNoAlias) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // o1 = {}
  auto* o1 = builder.Create<NewTableInst>(0);
  // o2 = {}
  auto* o2 = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(100);

  // Load from o1.x
  [[maybe_unused]] auto* get1 =
      builder.Create<GetTableInst>(0, o1, (Literal*)key);

  // Store to o2.x. This should NOT invalidate o1.x
  builder.Create<SetTableInst>(0, o2, (Literal*)key,
                               builder.GetLiteralInt32(100));

  // Load from o1.x again. SHOULD be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, o1, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Load from o1 should be eliminated despite store to o2";
}

TEST_F(LEPUSIRTestLoadStoreElimination, MixedAllocationsNoAlias) {
  OpBuilder builder;
  builder.SetModuleOp(mod);
  builder.SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);
  builder.CreateRegion(func);
  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* obj = builder.Create<NewTableInst>(0);
  auto* arr = builder.Create<NewArrayInst>(0, ArgList{});
  auto* key = builder.GetLiteralInt32(0);

  // Load obj[0]
  builder.Create<GetTableInst>(0, obj, (Literal*)key);

  // Store arr[0] = 10. Should NOT invalidate obj[0]
  builder.Create<SetTableInst>(0, arr, (Literal*)key,
                               builder.GetLiteralInt32(10));

  // Load obj[0] again. SHOULD be eliminated.
  auto* get2 = builder.Create<GetTableInst>(0, obj, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  LoadStoreElimination pass(ir_ctx.get());
  pass.RunOnFunction(func);

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  EXPECT_FALSE(found_get2)
      << "Load from table should be eliminated despite store to array";
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyStringPrototypeMethodCallDoesNotInvalidateCaches_Slice) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Const table entries: receiver string and method name.
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t hello_idx =
      static_cast<uint32_t>(lepus_func->AddConstString(base::String("hello")));
  uint32_t slice_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kStringSlice)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  // A cached load on a fresh table.
  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  // Call "hello".slice(1) via GetTable + LoadConst(constants::kStringSlice).
  auto* recv = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(hello_idx),
      TypeOp::CreateString(&builder));
  auto* slice_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(slice_idx),
      TypeOp::CreateString(&builder));
  auto* slice_func = builder.Create<GetTableInst>(0, recv, slice_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(1));
  auto* call = builder.Create<CallInst>(0, slice_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  EXPECT_TRUE(call->IsReadonlyCall());

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       StringPrototypeReplaceMayCallUserClosure_StillInvalidatesCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Const table entries: receiver string and method name.
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t hello_idx =
      static_cast<uint32_t>(lepus_func->AddConstString(base::String("hello")));
  uint32_t replace_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kStringReplace)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  auto* recv = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(hello_idx),
      TypeOp::CreateString(&builder));
  auto* replace_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(replace_idx),
      TypeOp::CreateString(&builder));
  auto* replace_func = builder.Create<GetTableInst>(0, recv, replace_key);

  // Even without modeling the args precisely, `replace` is conservatively
  // treated as non-readonly.
  ArgList args;
  args.push_back(builder.GetLiteralInt32(0));
  args.push_back(builder.GetLiteralInt32(1));
  [[maybe_unused]] auto* call = builder.Create<CallInst>(0, replace_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // Cache should be invalidated, so the load must remain.
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyArrayPrototypeMethodCallDoesNotInvalidateCaches_Slice) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Const table entry for method name.
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t slice_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kStringSlice)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  auto* arr = builder.Create<NewArrayInst>(0, ArgList{});
  auto* slice_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(slice_idx),
      TypeOp::CreateString(&builder));
  auto* slice_func = builder.Create<GetTableInst>(0, arr, slice_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(0));
  auto* call = builder.Create<CallInst>(0, slice_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  EXPECT_TRUE(call->IsReadonlyCall());

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ArrayPrototypePushMutatesReceiver_StillInvalidatesCaches) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Const table entry for method name.
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t push_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kArrayPush)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  auto* arr = builder.Create<NewArrayInst>(0, ArgList{});
  auto* push_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(push_idx),
      TypeOp::CreateString(&builder));
  auto* push_func = builder.Create<GetTableInst>(0, arr, push_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(1));
  [[maybe_unused]] auto* call = builder.Create<CallInst>(0, push_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  // Cache must be invalidated.
  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_TRUE(found_get2);
}

TEST_F(LEPUSIRTestLoadStoreEliminationReadonlyGlobalPropertyCall,
       ReadonlyNumberPrototypeMethodCallDoesNotInvalidateCaches_toFixed) {
  OpBuilder builder;
  builder.SetModuleOp(mod_);
  builder.SetInsertionPointToEnd(mod_->GetFunctionBlock());

  std::string name = "test";
  auto* func = builder.Create<FuncOp>(0, name);

  // Const table entry for method name.
  fml::RefPtr<lepus::Function> lepus_func = lepus::Function::Create();
  uint32_t to_fixed_idx = static_cast<uint32_t>(
      lepus_func->AddConstString(base::String(constants::kNumberToFixed)));
  func->Init(lepus_func);

  auto* entry = builder.CreateBlock(func->GetSingleRegion(), BlockType::BT_INST,
                                    0, "entry");
  builder.SetInsertionPointToEnd(entry);

  auto* tbl = builder.Create<NewTableInst>(0);
  auto* key = builder.GetLiteralInt32(1);
  builder.Create<GetTableInst>(0, tbl, (Literal*)key);

  // 123.toFixed(2)
  auto* num = builder.GetLiteralInt32(123);
  auto* to_fixed_key = builder.Create<LoadConstInst>(
      0, (Literal*)builder.GetLiteralUint32(to_fixed_idx),
      TypeOp::CreateString(&builder));
  auto* to_fixed_func = builder.Create<GetTableInst>(0, num, to_fixed_key);
  ArgList args;
  args.push_back(builder.GetLiteralInt32(2));
  auto* call = builder.Create<CallInst>(0, to_fixed_func, args);

  auto* get2 = builder.Create<GetTableInst>(0, tbl, (Literal*)key);
  builder.Create<ReturnInst>(0, get2);

  TypeSpecification type_spec(ir_ctx_.get());
  type_spec.RunOnFunction(func);
  LoadStoreElimination pass(ir_ctx_.get());
  pass.RunOnFunction(func);

  EXPECT_TRUE(call->IsReadonlyCall());

  bool found_get2 = false;
  for (auto* inst : entry->InstRange()) {
    if (inst == get2) found_get2 = true;
  }
  ASSERT_FALSE(found_get2);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
