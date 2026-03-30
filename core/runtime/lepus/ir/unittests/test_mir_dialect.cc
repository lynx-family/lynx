// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <sstream>
#include <string>

#include "core/runtime/lepus/ir/dialects/mir/mir_dialect.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_dumper.h"
#include "core/runtime/lepus/ir/ir_dumper.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestMIRDialect : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestMIRDialect, testBasic) {
  auto dialect = MIRDialect::SharedInstance();
  ASSERT_NE(nullptr, dialect);
  ASSERT_EQ("mir", dialect->GetName());
  ASSERT_EQ(69, dialect->GetOpsSize());
}

TEST_F(LEPUSIRTestMIRDialect, testMIRPrinterSmoke) {
#ifdef LEPUS_TEST
  ASSERT_NE(nullptr, mod);
  auto* builder = ir_ctx->GetOpBuilder();
  ASSERT_NE(nullptr, builder);

  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
  std::string func_name = "mir_printer_smoke";
  auto* func = builder->Create<FuncOp>(0, func_name);
  ASSERT_NE(nullptr, func);
  auto* region = builder->CreateRegion(func);
  ASSERT_NE(nullptr, region);

  auto* entry = builder->CreateBlock(region, BlockType::BT_INST, 0, "entry");
  auto* then_bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "then");
  auto* else_bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "else");
  auto* merge_bb = builder->CreateBlock(region, BlockType::BT_INST, 0, "merge");
  ASSERT_NE(nullptr, entry);
  ASSERT_NE(nullptr, then_bb);
  ASSERT_NE(nullptr, else_bb);
  ASSERT_NE(nullptr, merge_bb);

  builder->SetInsertionPointToEnd(entry);
  auto* cond = builder->Create<LoadConstInst>(0, builder->GetLiteralBool(true),
                                              TypeOp::CreateBoolean(builder));
  builder->Create<CondBranchInst>(0, cond, then_bb, else_bb);

  builder->SetInsertionPointToEnd(then_bb);
  auto* v_then = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(1),
                                                TypeOp::CreateInt32(builder));
  builder->Create<BranchInst>(0, merge_bb);

  builder->SetInsertionPointToEnd(else_bb);
  auto* v_else = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                                TypeOp::CreateInt32(builder));
  builder->Create<BranchInst>(0, merge_bb);

  builder->SetInsertionPointToEnd(merge_bb);
  // Use a phi to make the printed IR non-trivial.
  auto* phi = builder->Create<PhiInst>(0, PhiInst::ValueListType(),
                                       PhiInst::BlockListType());
  ASSERT_NE(nullptr, phi);
  phi->AddEntry(v_then, then_bb);
  phi->AddEntry(v_else, else_bb);
  phi->SetType(TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, phi);

  std::ostringstream os;
  MIRPrinter printer(ir_ctx.get(), os);
  printer.VisitFuncOp(*func);
  const std::string out = os.str();

  EXPECT_NE(std::string::npos, out.find("function_end"));
  EXPECT_NE(std::string::npos, out.find("%BB"));
#endif
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
