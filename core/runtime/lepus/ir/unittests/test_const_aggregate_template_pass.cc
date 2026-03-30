// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <string>

#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"
#include "core/runtime/lepus/ir/transformer/vm/instruction_selection.h"
#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/parser.h"
#include "core/runtime/lepus/scanner.h"
#include "core/runtime/lepus/semantic_analysis.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

static lepus::Value EmptyBuiltinForConstAggregate(lepus::MTSContext* context,
                                                  lepus::Value*, int) {
  return lepus::Value();
}

static void RegisterConstAggregateBuiltins(lepus::VMContext* context) {
  lepus::RegisterCFunction(context, tasm::kCFunctionSetStyleObject,
                           &EmptyBuiltinForConstAggregate);
}

class LEPUSIRConstAggregateTemplateTest : public IRTestBase {
 protected:
  FuncOp* CreateFunc(std::string name, fml::RefPtr<lepus::Function>& lepus_func,
                     Block*& entry) {
    auto* builder = ir_ctx->GetOpBuilder();
    builder->SetModuleOp(mod);
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

    auto* func = builder->Create<FuncOp>(0, name);
    lepus_func = lepus::Function::Create();
    func->Init(lepus_func);

    entry =
        builder->CreateBlock(func->GetSingleRegion(), BlockType::BT_INST, {});
    builder->SetInsertionPointToStart(entry);
    return func;
  }

  void RunConstAggregateTemplate() {
    PassManager pm(ir_ctx.get());
    pm.SetMode(StageMode::SM_MIR);
    pm.AddConstAggregateTemplatePass();
    pm.Run(mod);
  }

  ModuleOp* CompileToIRWithConstAggregate(lepus::VMContext* context,
                                          const std::string& source) {
    parser::InputStream input;
    input.Write(source);
    Scanner scanner(&input);
    scanner.SetSdkVersion("2.6");
    Parser parser(&scanner);
    parser.SetSdkVersion("2.6");
    SemanticAnalysis semantic_analysis;
    semantic_analysis.SetInput(&scanner);
    semantic_analysis.SetSdkVersion("2.6");
    semantic_analysis.SetClosureFix(context->GetClosureFix());

    CodeGenerator code_generator(context, &semantic_analysis);
    std::unique_ptr<ASTree> root;
    root.reset(parser.Parse());
    root->Accept(&semantic_analysis, nullptr);
    root->Accept(&code_generator, nullptr);

    auto root_func = context->GetRootFunction();
    root_func->SetSource(source);
    root_func->SetTopLevelFunction(true);

    ir_ctx->Init(root_func, context);
    auto* compiled_mod = ir_ctx->GetMainMod();

    PassManager pm(ir_ctx.get());
    pm.SetMode(StageMode::SM_MIR);
    pm.AddCollectToplevelClosureRegPass();
    pm.AddConstructSSAIRPass();
    pm.AddNormalizePhiPass();
    pm.AddProcessSpecialMovPass();
    pm.AddGetToplevelRelatedInstEliminationPass();
    pm.AddSSAIRVerifyPass();
    pm.AddChangeSpecialAttributePass();
    pm.AddTypeSpecificationPass();
    pm.AddNormalizePhiPass();
    pm.AddSimplifyCFG();
    pm.AddInstCombinePass();
    pm.AddSimplifyCFG();
    pm.AddDCE();
    pm.AddCSE();
    pm.AddLoadStoreElimination();
    pm.AddDCE();
    pm.AddInstCombinePass();
    pm.AddLoadStoreElimination();
    pm.AddSimplifyCFG();
    pm.AddInstCombinePass();
    pm.AddConstAggregateTemplatePass();
    pm.AddDCE();
    pm.Run(compiled_mod);
    return compiled_mod;
  }
};

TEST_F(LEPUSIRConstAggregateTemplateTest,
       SetStyleObjectSecondArgUsesLoadConstAndDropsArray) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_set_style_object_arg", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t one_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(1));
  const uint32_t two_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(2));

  auto* callee = builder->Create<GetGlobalInst>(0, builder->GetLiteralUint32(0),
                                                TypeOp::CreateAnyType(builder));
  auto* one = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(one_idx), TypeOp::CreateInt64(builder));
  auto* two = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(two_idx), TypeOp::CreateInt64(builder));
  auto* arr = builder->Create<NewArrayInst>(0, ArgList{one, two});

  auto* call = builder->Create<CallInst>(0, callee, ArgList{one, arr});
  call->SetBuiltinFuncName(tasm::kCFunctionSetStyleObject);
  builder->Create<ReturnInst>(0, call);

  RunConstAggregateTemplate();

  bool saw_new_array = false;
  bool found_target_call = false;
  bool found_array_load_const = false;
  uint32_t aggregate_idx = 0;

  for (auto* inst : entry->InstRange()) {
    saw_new_array |= llvh::isa<NewArrayInst>(inst);
    if (auto* load = llvh::dyn_cast<LoadConstInst>(inst)) {
      if (load->GetType()->IsArrayType()) {
        auto* lit = llvh::dyn_cast<LiteralUint32>(load->GetConst());
        ASSERT_NE(lit, nullptr);
        aggregate_idx = lit->GetValue();
        found_array_load_const = true;
      }
    }
    if (auto* rewritten_call = llvh::dyn_cast<CallInst>(inst)) {
      found_target_call =
          rewritten_call->GetNumArguments() >= 2 &&
          llvh::isa<LoadConstInst>(rewritten_call->GetArgument(1)) &&
          rewritten_call->GetArgument(1)->GetType()->IsArrayType();
    }
  }

  EXPECT_FALSE(saw_new_array);
  EXPECT_TRUE(found_target_call);
  ASSERT_TRUE(found_array_load_const);
  ASSERT_LT(aggregate_idx, lepus_func->GetConstValue().size());
  auto* aggregate = lepus_func->GetConstValue(aggregate_idx);
  ASSERT_NE(aggregate, nullptr);
  ASSERT_TRUE(aggregate->IsArray());
  ASSERT_EQ(aggregate->Array()->size(), 2u);
  EXPECT_EQ(aggregate->Array()->get(0).Int64(), 1);
  EXPECT_EQ(aggregate->Array()->get(1).Int64(), 2);
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       EscapingArrayUsesLoadConstMaterialize) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_escape_array", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t one_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(1));
  const uint32_t two_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(2));
  const size_t const_pool_size = lepus_func->GetConstValue().size();

  auto* one = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(one_idx), TypeOp::CreateInt64(builder));
  auto* two = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(two_idx), TypeOp::CreateInt64(builder));
  auto* arr = builder->Create<NewArrayInst>(0, ArgList{one, two});
  builder->Create<ReturnInst>(0, arr);

  RunConstAggregateTemplate();

  bool found_new_array = false;
  bool found_array_load_mat = false;
  uint32_t aggregate_idx = 0;
  for (auto* inst : entry->InstRange()) {
    found_new_array |= llvh::isa<NewArrayInst>(inst);
    if (auto* load = llvh::dyn_cast<LoadConstMaterializeInst>(inst)) {
      if (load->GetType()->IsArrayType()) {
        auto* lit = llvh::dyn_cast<LiteralUint32>(load->GetConst());
        ASSERT_NE(lit, nullptr);
        aggregate_idx = lit->GetValue();
        found_array_load_mat = true;
      }
    }
  }

  EXPECT_FALSE(found_new_array);
  EXPECT_TRUE(found_array_load_mat);
  ASSERT_EQ(lepus_func->GetConstValue().size(), const_pool_size + 1);
  ASSERT_LT(aggregate_idx, lepus_func->GetConstValue().size());
  auto* aggregate = lepus_func->GetConstValue(aggregate_idx);
  ASSERT_NE(aggregate, nullptr);
  ASSERT_TRUE(aggregate->IsArray());
  ASSERT_EQ(aggregate->Array()->size(), 2u);
  EXPECT_EQ(aggregate->Array()->get(0).Int64(), 1);
  EXPECT_EQ(aggregate->Array()->get(1).Int64(), 2);
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       NonEscapingConstTableUsesLoadConstAndDropsInit) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_non_escape_table", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t k1_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k"));
  const uint32_t v1_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(7));
  const uint32_t k2_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k2"));
  const uint32_t v2_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(8));

  auto* t = builder->Create<NewTableInst>(0);
  auto* k1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k1_idx), TypeOp::CreateString(builder));
  auto* v1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v1_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k1, v1);
  auto* k2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k2_idx), TypeOp::CreateString(builder));
  auto* v2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v2_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k2, v2);
  auto* read = builder->Create<GetTableInst>(0, t, k1);
  builder->Create<ReturnInst>(0, read);

  RunConstAggregateTemplate();

  bool saw_new_table = false;
  bool saw_set_table = false;
  bool saw_load_const_table = false;
  for (auto* inst : entry->InstRange()) {
    saw_new_table |= llvh::isa<NewTableInst>(inst);
    saw_set_table |= llvh::isa<SetTableInst>(inst);
    if (auto* load = llvh::dyn_cast<LoadConstInst>(inst)) {
      saw_load_const_table |= load->GetType()->IsTableType();
    }
  }
  EXPECT_FALSE(saw_new_table);
  EXPECT_FALSE(saw_set_table);
  EXPECT_TRUE(saw_load_const_table);
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       EscapingConstTableUsesLoadConstMaterialize) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_escape_table", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t k1_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k"));
  const uint32_t v1_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(7));
  const uint32_t k2_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k2"));
  const uint32_t v2_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(8));

  auto* t = builder->Create<NewTableInst>(0);
  auto* k1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k1_idx), TypeOp::CreateString(builder));
  auto* v1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v1_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k1, v1);
  auto* k2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k2_idx), TypeOp::CreateString(builder));
  auto* v2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v2_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k2, v2);
  builder->Create<ReturnInst>(0, t);

  RunConstAggregateTemplate();

  bool saw_new_table = false;
  bool saw_set_table = false;
  bool saw_load_mat_table = false;
  for (auto* inst : entry->InstRange()) {
    saw_new_table |= llvh::isa<NewTableInst>(inst);
    saw_set_table |= llvh::isa<SetTableInst>(inst);
    if (auto* load = llvh::dyn_cast<LoadConstMaterializeInst>(inst)) {
      saw_load_mat_table |= load->GetType()->IsTableType();
    }
  }
  EXPECT_FALSE(saw_new_table);
  EXPECT_FALSE(saw_set_table);
  EXPECT_TRUE(saw_load_mat_table);
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       EscapingConstTableWithEarlyToplevelBindUsesLoadConstMaterialize) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_escape_table_toplevel_bind", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t k1_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k"));
  const uint32_t v1_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(7));
  const uint32_t k2_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k2"));
  const uint32_t v2_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(8));

  auto* t = builder->Create<NewTableInst>(0);
  // Bind to a toplevel var before finishing initialization.
  auto* reg = builder->GetLiteralUint32(0);
  builder->Create<SetToplevelVarInst>(0, reg, t);

  auto* k1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k1_idx), TypeOp::CreateString(builder));
  auto* v1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v1_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k1, v1);
  auto* k2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k2_idx), TypeOp::CreateString(builder));
  auto* v2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v2_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k2, v2);
  builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));

  RunConstAggregateTemplate();

  bool saw_new_table = false;
  bool saw_set_table = false;
  bool saw_load_mat_table = false;
  bool saw_set_toplevel = false;
  for (auto* inst : entry->InstRange()) {
    saw_new_table |= llvh::isa<NewTableInst>(inst);
    saw_set_table |= llvh::isa<SetTableInst>(inst);
    if (auto* load = llvh::dyn_cast<LoadConstMaterializeInst>(inst)) {
      saw_load_mat_table |= load->GetType()->IsTableType();
    }
    if (auto* set_tv = llvh::dyn_cast<SetToplevelVarInst>(inst)) {
      saw_set_toplevel = true;
      // After rewrite it should store the materialized table.
      EXPECT_TRUE(llvh::isa<LoadConstMaterializeInst>(set_tv->GetSrc()));
    }
  }

  EXPECT_FALSE(saw_new_table);
  EXPECT_FALSE(saw_set_table);
  EXPECT_TRUE(saw_load_mat_table);
  EXPECT_TRUE(saw_set_toplevel);
}

TEST_F(LEPUSIRConstAggregateTemplateTest, ConstTableDuplicateKeyLastWins) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_table_duplicate_key", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t k_idx = static_cast<uint32_t>(lepus_func->AddConstString("k"));
  const uint32_t v1_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(7));
  const uint32_t v2_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(9));

  auto* t = builder->Create<NewTableInst>(0);
  auto* k = builder->Create<LoadConstInst>(0, builder->GetLiteralUint32(k_idx),
                                           TypeOp::CreateString(builder));
  auto* v1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v1_idx), TypeOp::CreateInt64(builder));
  auto* v2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v2_idx), TypeOp::CreateInt64(builder));

  builder->Create<SetTableInst>(0, t, k, v1);
  builder->Create<SetTableInst>(0, t, k, v2);
  auto* read = builder->Create<GetTableInst>(0, t, k);
  builder->Create<ReturnInst>(0, read);

  RunConstAggregateTemplate();

  uint32_t table_idx = 0;
  bool found_table_load = false;
  for (auto* inst : entry->InstRange()) {
    if (auto* load = llvh::dyn_cast<LoadConstInst>(inst)) {
      if (load->GetType()->IsTableType()) {
        auto* lit = llvh::dyn_cast<LiteralUint32>(load->GetConst());
        ASSERT_NE(lit, nullptr);
        table_idx = lit->GetValue();
        found_table_load = true;
        break;
      }
    }
  }
  ASSERT_TRUE(found_table_load);
  ASSERT_LT(table_idx, lepus_func->GetConstValue().size());
  auto* aggregate = lepus_func->GetConstValue(table_idx);
  ASSERT_NE(aggregate, nullptr);
  ASSERT_TRUE(aggregate->IsTable());
  auto dict = aggregate->Table();
  ASSERT_NE(dict, nullptr);
  auto v = dict->GetValue(base::String("k"));
  ASSERT_TRUE(v);
  ASSERT_TRUE(v->IsNumber());
  EXPECT_EQ(v->Int64(), 9);
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       TableInitWriteAfterNonInitUseSkipsConstAggregate) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_table_write_after_use", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t k1_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k"));
  const uint32_t v1_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(7));
  const uint32_t k2_idx =
      static_cast<uint32_t>(lepus_func->AddConstString("k2"));
  const uint32_t v2_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(8));

  auto* t = builder->Create<NewTableInst>(0);
  auto* k1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k1_idx), TypeOp::CreateString(builder));
  auto* v1 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v1_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k1, v1);

  // Non-init use ends the initialization window.
  auto* read = builder->Create<GetTableInst>(0, t, k1);
  (void)read;

  // A write after a non-init use should make the pass bail out.
  auto* k2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(k2_idx), TypeOp::CreateString(builder));
  auto* v2 = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(v2_idx), TypeOp::CreateInt64(builder));
  builder->Create<SetTableInst>(0, t, k2, v2);
  builder->Create<ReturnInst>(0, builder->GetLiteralInt32(0));

  const size_t before = lepus_func->GetConstValue().size();
  RunConstAggregateTemplate();

  // No rewrite should happen: NewTable + SetTableInst remain, and const pool
  // should not grow due to a synthesized aggregate.
  bool saw_new_table = false;
  int set_table_count = 0;
  bool saw_table_load = false;
  for (auto* inst : entry->InstRange()) {
    saw_new_table |= llvh::isa<NewTableInst>(inst);
    set_table_count += llvh::isa<SetTableInst>(inst) ? 1 : 0;
    if (auto* load = llvh::dyn_cast<LoadConstInst>(inst)) {
      saw_table_load |= load->GetType()->IsTableType();
    }
  }
  EXPECT_TRUE(saw_new_table);
  EXPECT_EQ(set_table_count, 2);
  EXPECT_FALSE(saw_table_load);
  EXPECT_EQ(lepus_func->GetConstValue().size(), before);
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       NonPureStyleArraySkipsConstAggregate) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  CreateFunc("const_aggregate_non_pure_style_array", lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t one_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(1));
  const uint32_t two_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(2));
  const size_t const_pool_size = lepus_func->GetConstValue().size();

  auto* callee = builder->Create<GetGlobalInst>(0, builder->GetLiteralUint32(0),
                                                TypeOp::CreateAnyType(builder));
  auto* one = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(one_idx), TypeOp::CreateInt64(builder));
  auto* two = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(two_idx), TypeOp::CreateInt64(builder));
  auto* inner = builder->Create<NewArrayInst>(0, ArgList{one});
  auto* arr = builder->Create<NewArrayInst>(0, ArgList{inner, two});

  auto* call = builder->Create<CallInst>(0, callee, ArgList{one, arr});
  call->SetBuiltinFuncName(tasm::kCFunctionSetStyleObject);
  builder->Create<ReturnInst>(0, call);

  RunConstAggregateTemplate();

  int new_array_count = 0;
  bool found_array_load_const = false;
  bool found_array_load_materialize = false;
  for (auto* inst : entry->InstRange()) {
    new_array_count += llvh::isa<NewArrayInst>(inst) ? 1 : 0;
    if (auto* load = llvh::dyn_cast<LoadConstInst>(inst)) {
      found_array_load_const |= load->GetType()->IsArrayType();
    }
    if (auto* load = llvh::dyn_cast<LoadConstMaterializeInst>(inst)) {
      found_array_load_materialize |= load->GetType()->IsArrayType();
    }
  }

  // Tiny aggregates (size <= 1) are skipped.
  EXPECT_EQ(new_array_count, 2);
  EXPECT_FALSE(found_array_load_const);
  EXPECT_FALSE(found_array_load_materialize);
  EXPECT_EQ(lepus_func->GetConstValue().size(), const_pool_size);
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       SetStyleObjectSecondArgInInnerClosureUsesConstTemplateLoad) {
  auto* context = new lepus::VMContext();
  context->Initialize();
  context->SetClosureFix(true);
  RegisterConstAggregateBuiltins(context);

  const std::string source = R"(
    function outer(el) {
      let styleSetter = __SetStyleObject;
      function inner() {
        styleSetter(el, [1, 2]);
      }
      return inner;
    }
  )";

  auto* compiled_mod = CompileToIRWithConstAggregate(context, source);
  ASSERT_NE(compiled_mod, nullptr);

  FuncOp* inner_func_op = nullptr;
  for (auto* func : *compiled_mod) {
    if (func->GetName() == "inner") {
      inner_func_op = func;
      break;
    }
  }
  ASSERT_NE(inner_func_op, nullptr);

  bool found_target_call = false;
  bool found_const_template_load_array = false;
  for (auto& block : *inner_func_op) {
    for (auto& inst : block) {
      if (auto* load = llvh::dyn_cast<LoadConstInst>(&inst)) {
        if (load->GetType()->IsArrayType())
          found_const_template_load_array = true;
      } else if (auto* load = llvh::dyn_cast<LoadConstMaterializeInst>(&inst)) {
        if (load->GetType()->IsArrayType())
          found_const_template_load_array = true;
      }
      if (auto* call = llvh::dyn_cast<CallInst>(&inst)) {
        if (call->GetNumArguments() >= 2 && call->GetArgument(1) != nullptr &&
            call->GetArgument(1)->GetType()->IsArrayType() &&
            (llvh::isa<LoadConstInst>(call->GetArgument(1)) ||
             llvh::isa<LoadConstMaterializeInst>(call->GetArgument(1)))) {
          found_target_call = true;
        }
      }
    }
  }

  EXPECT_TRUE(found_target_call);
  EXPECT_TRUE(found_const_template_load_array);
  delete context;
}

TEST_F(LEPUSIRConstAggregateTemplateTest,
       SetStyleObjectSecondArgCompilesToLoadConstOpcode) {
  fml::RefPtr<lepus::Function> lepus_func;
  Block* entry = nullptr;
  auto* func = CreateFunc("const_aggregate_opcode_without_new_bytecode",
                          lepus_func, entry);

  auto* builder = ir_ctx->GetOpBuilder();
  const uint32_t one_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(1));
  const uint32_t two_idx = static_cast<uint32_t>(lepus_func->AddConstNumber(2));

  auto* callee = builder->Create<GetGlobalInst>(0, builder->GetLiteralUint32(0),
                                                TypeOp::CreateAnyType(builder));
  auto* one = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(one_idx), TypeOp::CreateInt64(builder));
  auto* two = builder->Create<LoadConstInst>(
      0, builder->GetLiteralUint32(two_idx), TypeOp::CreateInt64(builder));
  auto* arr = builder->Create<NewArrayInst>(0, ArgList{one, two});

  auto* call = builder->Create<CallInst>(0, callee, ArgList{one, arr});
  call->SetBuiltinFuncName(tasm::kCFunctionSetStyleObject);
  builder->Create<ReturnInst>(0, call);

  RunConstAggregateTemplate();

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  InstructionSelectionPass is_pass(ir_ctx.get());
  is_pass.RunOnFunction(func);

  bool found_array_load_const_opcode = false;
  for (size_t i = 0; i < lepus_func->OpCodeSize(); ++i) {
    auto* inst = lepus_func->GetInstruction(i);
    ASSERT_NE(inst, nullptr);
    if (lepus::Instruction::GetOpCode(*inst) != lepus::TypeOp_LoadConst) {
      continue;
    }

    const auto idx = static_cast<size_t>(lepus::Instruction::GetParamBx(*inst));
    ASSERT_LT(idx, lepus_func->GetConstValue().size());
    auto* aggregate = lepus_func->GetConstValue(idx);
    if (aggregate != nullptr && aggregate->IsArray()) {
      found_array_load_const_opcode = true;
      EXPECT_EQ(aggregate->Array()->size(), 2u);
    }
  }

  EXPECT_TRUE(found_array_load_const_opcode);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
