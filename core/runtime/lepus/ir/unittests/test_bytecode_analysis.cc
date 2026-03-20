// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/transformer/mir/bytecode_analysis.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_builder.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_iterator.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"
#include "core/runtime/lepus/op_code.h"

namespace lynx {
namespace lepus {
namespace ir {
namespace test {

using BytecodeInst = lynx::lepus::Instruction;

class LEPUSIRTestBytecodeAnalysis : public IRTestBase {
 public:
  virtual void SetUp(void) override { IRTestBase::SetUp(); }
  virtual void TearDown(void) override { IRTestBase::TearDown(); }

  void SetupFuncOp(std::string name, FuncOp*& func_op) {
    auto mod = ir_ctx->GetMainMod();
    auto builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());
    func_op = builder->Create<FuncOp>(0, name);
  }
};

TEST_F(LEPUSIRTestBytecodeAnalysis, BytecodeBuilderSSA) {
  std::string name = "ssa_builder";
  FuncOp* func = nullptr;

  // LoadConst uses const-table indices. Populate const table and use returned
  // indices to avoid out-of-range access.
  SetupFuncOp(name, func);
  auto lepus_func = lynx::lepus::Function::Create();
  const uint32_t c10 = lepus_func->AddConstNumber(10);
  const uint32_t c20 = lepus_func->AddConstNumber(20);

  // 0: LoadConst R0, 10
  // 1: LoadConst R1, 20
  // 2: Add R2, R0, R1
  // 3: Ret R2
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, c10),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, c20),
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Add, 2, 0, 1),
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 2),
      BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)};

  for (auto& inst : code) {
    lepus_func->AddInstruction(inst);
  }
  lepus_func->SetRegisterCount(10);  // set enough registers
  func->Init(lepus_func);

  SSABuilder ssaBuilder(ir_ctx.get(), func);
  ssaBuilder.Build();

  // Verify IR
  // It should have at least one block (entry)
  ASSERT_FALSE(func->GetSingleRegion()->GetBlocks().empty());

  int total_insts = 0;
  for (auto& bb : *func) {
    total_insts += bb.size();
  }

  // LoadConst x 2, Add, Ret, plus maybe some BranchInst or Placeholder
  EXPECT_GE(total_insts, 4);
}

TEST_F(LEPUSIRTestBytecodeAnalysis, BytecodeIteratorBasic) {
  // Test decoding of some common instructions
  // LoadConst R0, 10
  BytecodeInst inst1 =
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 10);
  // Add R2, R0, R1
  BytecodeInst inst2 = BytecodeInst::ABCCode(lynx::lepus::TypeOp_Add, 2, 0, 1);
  // Jmp 5
  BytecodeInst inst3 = BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, 5);

  std::vector<BytecodeInst> code = {inst1, inst2, inst3};
  std::vector<int64_t> line_cols = {0, 0, 0};

  BytecodeIterator iter;
  iter.Reset(&code[0], &code[0] + code.size(), line_cols);

  EXPECT_FALSE(iter.Done());
  EXPECT_EQ(iter.GetOpcode(), LepusOpcode::OP_TypeOp_LoadConst);
  EXPECT_EQ(iter.GetOperand0(), 0);
  EXPECT_EQ(iter.GetOperand1x(), 10);

  iter.Next();
  EXPECT_FALSE(iter.Done());
  EXPECT_EQ(iter.GetOpcode(), LepusOpcode::OP_TypeOp_Add);
  EXPECT_EQ(iter.GetOperand0(), 2);
  EXPECT_EQ(iter.GetOperand1(), 0);
  EXPECT_EQ(iter.GetOperand2(), 1);

  iter.Next();
  EXPECT_FALSE(iter.Done());
  EXPECT_EQ(iter.GetOpcode(), LepusOpcode::OP_TypeOp_Jmp);
  EXPECT_EQ(iter.JumpOffset(), 5);

  iter.Next();
  EXPECT_TRUE(iter.Done());
}

TEST_F(LEPUSIRTestBytecodeAnalysis, BytecodeIteratorSignedCmpJumpOffset) {
  // Regression: Equal/UnEqualJmp* use an 8-bit signed jump offset in operand1.
  // BytecodeIterator::JumpOffset() must sign-extend it.
  BytecodeInst inst1 = BytecodeInst::ABCCode(lynx::lepus::TypeOp_EqualJmpFalse,
                                             0, static_cast<uint8_t>(-3), 1);
  BytecodeInst inst2 = BytecodeInst::ABCCode(lynx::lepus::TypeOp_UnEqualJmpTrue,
                                             0, static_cast<uint8_t>(5), 1);

  std::vector<BytecodeInst> code = {inst1, inst2};
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeIterator iter;
  iter.Reset(&code[0], &code[0] + code.size(), line_cols);

  EXPECT_FALSE(iter.Done());
  EXPECT_EQ(iter.GetOpcode(), LepusOpcode::OP_TypeOp_EqualJmpFalse);
  EXPECT_EQ(iter.JumpOffset(), -3);

  iter.Next();
  EXPECT_FALSE(iter.Done());
  EXPECT_EQ(iter.GetOpcode(), LepusOpcode::OP_TypeOp_UnEqualJmpTrue);
  EXPECT_EQ(iter.JumpOffset(), 5);

  iter.Next();
  EXPECT_TRUE(iter.Done());
}

TEST_F(LEPUSIRTestBytecodeAnalysis, SetUpvalueOperandLayout) {
  // Regression test for TypeOp_SetUpvalue operand metadata.
  // CodeGen encodes SetUpvalue as: A = src register, B = upvalue index.
  // Liveness analysis must treat only A as SrcReg and must NOT treat B as a
  // source register.
  std::string name = "set_upvalue_operand_layout";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  std::vector<BytecodeInst> code = {
      BytecodeInst::ABCode(lynx::lepus::TypeOp_SetUpvalue, 3, 9),
      BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER),
  };
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 16);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  auto bb0 = analysis.GetBlockItem(0);
  ASSERT_NE(bb0, nullptr);
  auto live_in = bb0->GetLiveIn();
  ASSERT_NE(live_in, nullptr);
  EXPECT_TRUE(live_in->test(3));
  EXPECT_FALSE(live_in->test(9));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, SetToplevelClosureVarOperandLayout) {
  // Regression test for TypeOp_SetToplevelClosureVar operand metadata.
  // VM executes it as: A = src register, B = toplevel index (immediate).
  std::string name = "set_toplevel_closure_var_operand_layout";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  std::vector<BytecodeInst> code = {
      BytecodeInst::ABCode(lynx::lepus::TypeOp_SetToplevelClosureVar, 3, 9),
      BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER),
  };
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 16);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  auto bb0 = analysis.GetBlockItem(0);
  ASSERT_NE(bb0, nullptr);
  auto live_in = bb0->GetLiveIn();
  ASSERT_NE(live_in, nullptr);
  EXPECT_TRUE(live_in->test(3));
  EXPECT_FALSE(live_in->test(9));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, ImmediateOperandsNotTreatedAsRegisters) {
  // This test covers a set of bytecodes that contain immediate fields.
  // It verifies that liveness (live-in) only tracks real source registers and
  // never treats immediates (const index / upvalue index / jump offset /
  // depth/index, etc.) as registers.
  struct Case {
    const char* name;
    std::vector<BytecodeInst> code;
    std::vector<int> must_live;
    std::vector<int> must_not_live;
    int reg_count;
  };

  std::vector<Case> cases;

  // LoadNil: A=dst, B=type(imm)
  cases.push_back({"LoadNil",
                   {BytecodeInst::ABCode(lynx::lepus::TypeOp_LoadNil, 2, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {},
                   {2, 9},
                   16});

  // LoadConst: A=dst, Bx=const index(imm)
  cases.push_back({"LoadConst",
                   {BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 2, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {},
                   {2, 9},
                   16});

  // GetUpvalue: A=dst, B=upvalue index(imm)
  cases.push_back({"GetUpvalue",
                   {BytecodeInst::ABCode(lynx::lepus::TypeOp_GetUpvalue, 2, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {},
                   {2, 9},
                   16});

  // GetGlobal: A=dst, Bx=global index(imm)
  cases.push_back({"GetGlobal",
                   {BytecodeInst::ABxCode(lynx::lepus::TypeOp_GetGlobal, 2, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {},
                   {2, 9},
                   16});

  // GetBuiltin: A=dst, Bx=builtin index(imm)
  cases.push_back({"GetBuiltin",
                   {BytecodeInst::ABxCode(lynx::lepus::TypeOp_GetBuiltin, 2, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {},
                   {2, 9},
                   16});

  // Closure: A=dst, Bx=proto index(imm)
  cases.push_back({"Closure",
                   {BytecodeInst::ABxCode(lynx::lepus::TypeOp_Closure, 2, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {},
                   {2, 9},
                   16});

  // CreateContext: A=dst, B=size(imm)
  cases.push_back(
      {"CreateContext",
       {BytecodeInst::ABCode(lynx::lepus::TypeOp_CreateContext, 2, 9),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {},
       {2, 9},
       16});

  // GetContextSlotMove: A=dst, B=index(imm), C=context(src)
  cases.push_back(
      {"GetContextSlotMove",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_GetContextSlotMove, 2, 9, 3),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3},
       {2, 9},
       16});

  // SetContextSlotMove: A=src, B=index(imm), C=context(src)
  cases.push_back(
      {"SetContextSlotMove",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_SetContextSlotMove, 3, 9, 4),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3, 4},
       {9},
       16});

  // GetContextSlot: A=dst, B=index(imm), C=depth(imm)
  cases.push_back(
      {"GetContextSlot",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_GetContextSlot, 2, 9, 4),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {},
       {2, 4, 9},
       16});

  // SetContextSlot: A=src, B=index(imm), C=depth(imm)
  cases.push_back(
      {"SetContextSlot",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_SetContextSlot, 3, 9, 4),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3},
       {4, 9},
       16});

  // Jmp/JmpFalse/JmpTrue + BoolJmp*: offset is immediate, should not be live.
  cases.push_back({"Jmp",
                   {BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {},
                   {9},
                   16});
  cases.push_back({"JmpFalse",
                   {BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpFalse, 3, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {3},
                   {9},
                   16});
  cases.push_back({"JmpTrue",
                   {BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpTrue, 3, 9),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {3},
                   {9},
                   16});
  cases.push_back(
      {"BoolJmpFalse",
       {BytecodeInst::ABxCode(lynx::lepus::TypeOp_BoolJmpFalse, 3, 9),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3},
       {9},
       16});
  cases.push_back(
      {"BoolJmpTrue",
       {BytecodeInst::ABxCode(lynx::lepus::TypeOp_BoolJmpTrue, 3, 9),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3},
       {9},
       16});

  // Equal/UnEqualJmp*: B is 8-bit jump offset immediate, A/C are regs.
  cases.push_back(
      {"EqualJmpFalse",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_EqualJmpFalse, 3, 9, 4),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3, 4},
       {9},
       16});
  cases.push_back(
      {"EqualJmpTrue",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_EqualJmpTrue, 3, 9, 4),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3, 4},
       {9},
       16});
  cases.push_back(
      {"UnEqualJmpFalse",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_UnEqualJmpFalse, 3, 9, 4),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3, 4},
       {9},
       16});
  cases.push_back(
      {"UnEqualJmpTrue",
       {BytecodeInst::ABCCode(lynx::lepus::TypeOp_UnEqualJmpTrue, 3, 9, 4),
        BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
       {3, 4},
       {9},
       16});

  // Call: operand B is argc (immediate). Use a high A value to avoid
  // accidentally overlapping with low immediates in assertions.
  cases.push_back({"Call",
                   {BytecodeInst::ABCCode(lynx::lepus::TypeOp_Call, 10, 2, 1),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {10, 11, 12},
                   {2},
                   20});

  // NewArray: operand B is array_size (immediate). Use a high dst value to
  // avoid accidentally overlapping with low immediates in assertions.
  cases.push_back({"NewArray",
                   {BytecodeInst::ABCode(lynx::lepus::TypeOp_NewArray, 10, 2),
                    BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)},
                   {11, 12},
                   {2, 10},
                   20});

  for (const auto& tc : cases) {
    FuncOp* func = nullptr;
    SetupFuncOp(std::string(tc.name), func);
    std::vector<int64_t> line_cols(tc.code.size(), 0);

    BytecodeAnalysis analysis(func, tc.reg_count);
    analysis.Run(const_cast<BytecodeInst*>(&tc.code[0]),
                 const_cast<BytecodeInst*>(&tc.code[0]) + tc.code.size(),
                 line_cols);

    auto bb0 = analysis.GetBlockItem(0);
    ASSERT_NE(bb0, nullptr) << tc.name;
    auto live_in = bb0->GetLiveIn();
    ASSERT_NE(live_in, nullptr) << tc.name;

    for (int r : tc.must_live) {
      EXPECT_TRUE(live_in->test(r)) << tc.name << " missing live reg " << r;
    }
    for (int r : tc.must_not_live) {
      if (r >= 0 && r < tc.reg_count) {
        EXPECT_FALSE(live_in->test(r))
            << tc.name << " unexpected live reg " << r;
      }
    }
  }
}

TEST_F(LEPUSIRTestBytecodeAnalysis, CfgEdgesForExtendedJumps) {
  // Verify BytecodeAnalysis can recognize the extended jump opcodes supported
  // by VM:
  // - BoolJmp* uses Bx as offset
  // - Equal/UnEqualJmp* uses B as offset
  std::string name = "cfg_edges_extended_jumps";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  // 0: LoadConst R0, 1
  // 1: BoolJmpFalse R0, 2  (target = 3)
  // 2: Jmp 2               (target = 4)
  // 3: LoadConst R1, 2
  // 4: EqualJmpFalse R0, 2, R1  (target = 6)
  // 5: Jmp 1               (target = 6)
  // 6: Ret R0
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 1),     // 0
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_BoolJmpFalse, 0, 2),  // 1 -> 3
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, 2),           // 2 -> 4
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, 2),     // 3
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_EqualJmpFalse, 0, 2,
                            1),                              // 4 -> 6
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, 1),  // 5 -> 6
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 0),       // 6
      BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER),       // 7
  };
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 16);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  EXPECT_TRUE(analysis.IsBBHeader(0));
  EXPECT_TRUE(analysis.IsBBHeader(2));
  EXPECT_TRUE(analysis.IsBBHeader(3));
  EXPECT_TRUE(analysis.IsBBHeader(4));
  EXPECT_TRUE(analysis.IsBBHeader(6));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, StraightLineLiveness) {
  std::string name = "straight_line";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  // R0 = 10
  // R1 = 20
  // R2 = R0 + R1
  // Ret R2
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 10),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, 20),
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Add, 2, 0, 1),
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 2),
      BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)};
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 10);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  auto block0 = analysis.GetBlockItem(0);
  ASSERT_NE(block0, nullptr);

  // liveIn at start of block 0 should be empty
  EXPECT_EQ(block0->GetLiveIn()->count(), 0);

  // liveOut at end of block 0 (after LoadConst R0) should contain R0
  EXPECT_EQ(block0->GetLiveOut()->count(), 1);
  EXPECT_TRUE(block0->GetLiveOut()->test(0));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, BranchLiveness) {
  std::string name = "branch";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  // 0: LoadConst R0, 1
  // 1: JmpFalse R0, 3 (jump to 4)
  // 2: LoadConst R1, 10
  // 3: Jmp 2 (jump to 5)
  // 4: LoadConst R1, 20
  // 5: Ret R1
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 1),  // 0
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpFalse, 0,
                            3),  // 1 -> jumps to 1 + 3 = 4
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, 10),  // 2
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0,
                            2),  // 3 -> jumps to 3 + 2 = 5
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, 20),  // 4
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 1)               // 5
  };
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 10);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  // bb headers should be 0, 2, 4, 5
  EXPECT_TRUE(analysis.IsBBHeader(0));
  EXPECT_TRUE(analysis.IsBBHeader(2));
  EXPECT_TRUE(analysis.IsBBHeader(4));
  EXPECT_TRUE(analysis.IsBBHeader(5));

  auto bb5 = analysis.GetBlockItem(5);
  // liveIn of BB5 should contain R1
  EXPECT_TRUE(bb5->GetLiveIn()->test(1));

  auto bb2 = analysis.GetBlockItem(2);
  // liveOut of BB2 should contain R1 (assigned in BB2, used in BB5)
  EXPECT_TRUE(bb2->GetLiveOut()->test(1));

  auto bb4 = analysis.GetBlockItem(4);
  EXPECT_TRUE(bb4->GetLiveOut()->test(1));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, LoopAnalysisBasic) {
  std::string name = "loop";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  // 0: LoadConst R0, 0
  // 1: Less R1, R0, 10 (HEADER)
  // 2: JmpFalse R1, 3 (jump to 5)
  // 3: Add R0, R0, 1
  // 4: Jmp -3 (jump to 1)
  // 5: Ret R0
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 0),  // 0
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Less, 1, 0, 10),   // 1 (HEADER)
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpFalse, 1,
                            3),  // 2 -> jumps to 2 + 3 = 5
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Add, 0, 0, 1),  // 3
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0,
                            -3),  // 4 -> jumps to 4 - 3 = 1
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 0)  // 5
  };
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 10);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  EXPECT_TRUE(analysis.HasLoop());
  EXPECT_TRUE(analysis.IsLoopHeader(1));

  const auto& loop_info = analysis.GetLoopInfo(1);
  EXPECT_EQ(loop_info.Header(), 1);
  EXPECT_EQ(loop_info.End(), 4);

  // R0 and R1 are assigned within the loop
  EXPECT_TRUE(loop_info.GetAssignments()->test(0));
  EXPECT_TRUE(loop_info.GetAssignments()->test(1));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, NestedLoops) {
  std::string name = "nested_loops";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  // 0: LoadConst R0, 0
  // 1: Less R1, R0, 10 (OUTER HEADER)
  // 2: JmpFalse R1, 7 (jump to 9)
  // 3: LoadConst R2, 0
  // 4: Less R3, R2, 10 (INNER HEADER)
  // 5: JmpFalse R3, 3 (jump to 8)
  // 6: Add R2, R2, 1
  // 7: Jmp -3 (jump to 4)
  // 8: Jmp -7 (jump to 1)
  // 9: Ret R0
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 0),  // 0
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Less, 1, 0,
                            10),  // 1 (OUTER HEADER)
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpFalse, 1, 7),   // 2 -> 9
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 2, 0),  // 3
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Less, 3, 2,
                            10),  // 4 (INNER HEADER)
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpFalse, 3, 3),  // 5 -> 8
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Add, 2, 2, 1),    // 6
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, -3),      // 7 -> 4
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, -7),      // 8 -> 1
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 0)             // 9
  };
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 10);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  EXPECT_TRUE(analysis.IsLoopHeader(1));
  EXPECT_TRUE(analysis.IsLoopHeader(4));

  const auto& outer_loop = analysis.GetLoopInfo(1);
  // Outer loop should include assignments from inner loop
  EXPECT_TRUE(outer_loop.GetAssignments()->test(1));
  EXPECT_TRUE(outer_loop.GetAssignments()->test(2));
  EXPECT_TRUE(outer_loop.GetAssignments()->test(3));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, RedefinitionLiveness) {
  std::string name = "redefinition";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  // 0: LoadConst R0, 10
  // 1: LoadConst R0, 20
  // 2: Ret R0
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 10),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 20),
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 0)};
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 10);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  // At offset 1 (second LoadConst), R0 should NOT be live-in
  // because it's redefined in this block (instruction 1).
  auto bb1 = analysis.GetBlockItem(1);
  EXPECT_FALSE(bb1->GetLiveIn()->test(0));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, SpecialLiveness) {
  std::string name = "special_liveness";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);

  // 0: LoadConst R0, func
  // 1: LoadConst R1, arg1
  // 2: LoadConst R2, arg2
  // 3: Call R0, 2 (args are R1, R2), result to R3
  // 4: NewArray R4, 2 (items are R5, R6)
  // 5: Ret R3
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 100),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, 101),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 2, 102),
      BytecodeInst::ABCCode(lynx::lepus::TypeOp_Call, 0, 2, 3),
      BytecodeInst::ABCode(lynx::lepus::TypeOp_NewArray, 4, 2),
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 3)};
  std::vector<int64_t> line_cols(code.size(), 0);

  BytecodeAnalysis analysis(func, 10);
  analysis.Run(&code[0], &code[0] + code.size(), line_cols);

  // At offset 3 (Call), R1 and R2 should be live-in (as arguments)
  auto bb3 = analysis.GetBlockItem(3);
  EXPECT_TRUE(bb3->GetLiveIn()->test(1));
  EXPECT_TRUE(bb3->GetLiveIn()->test(2));

  // At offset 4 (NewArray), R5 and R6 should be live-in
  auto bb4 = analysis.GetBlockItem(4);
  EXPECT_TRUE(bb4->GetLiveIn()->test(5));
  EXPECT_TRUE(bb4->GetLiveIn()->test(6));
}

TEST_F(LEPUSIRTestBytecodeAnalysis, BytecodeBuilderComplex) {
  std::string name = "ssa_builder_branch";
  FuncOp* func = nullptr;

  SetupFuncOp(name, func);
  auto lepus_func = lynx::lepus::Function::Create();
  const uint32_t c1 = lepus_func->AddConstNumber(1);
  const uint32_t c10 = lepus_func->AddConstNumber(10);
  const uint32_t c20 = lepus_func->AddConstNumber(20);

  // 0: LoadConst R0, 1
  // 1: JmpFalse R0, 3
  // 2: LoadConst R1, 10
  // 3: Jmp 2
  // 4: LoadConst R1, 20
  // 5: Ret R1
  // 6: Placeholder
  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, c1),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_JmpFalse, 0, 3),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, c10),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_Jmp, 0, 2),
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 1, c20),
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 1),
      BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)};

  for (auto& inst : code) {
    lepus_func->AddInstruction(inst);
  }
  lepus_func->SetRegisterCount(10);  // set enough registers
  func->Init(lepus_func);

  SSABuilder ssaBuilder(ir_ctx.get(), func);
  ssaBuilder.Build();

  // Verify IR
  // It should have multiple blocks due to branching
  int num_blocks = 0;
  for (const auto& bb : *func) {
    (void)bb;
    num_blocks++;
  }
  EXPECT_GT(num_blocks, 1);

  // The Ret block (offset 5) should have a Phi instruction for R1
  // because R1 is merged from two paths.
  bool found_phi = false;
  for (auto& bb : *func) {
    for (auto& inst : bb) {
      if (llvh::isa<PhiInst>(&inst)) {
        found_phi = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found_phi);
}

TEST_F(LEPUSIRTestBytecodeAnalysis, BytecodeBuilderLoadConst_OutOfRangeThrows) {
  std::string name = "ssa_builder_bad_const";
  FuncOp* func = nullptr;
  SetupFuncOp(name, func);
  auto lepus_func = lynx::lepus::Function::Create();
  lepus_func->SetRegisterCount(10);

  // const table has only 1 entry, but bytecode references index 7.
  (void)lepus_func->AddConstNumber(123);

  std::vector<BytecodeInst> code = {
      BytecodeInst::ABxCode(lynx::lepus::TypeOp_LoadConst, 0, 7),
      BytecodeInst::ACode(lynx::lepus::TypeOp_Ret, 0),
      BytecodeInst::Code(lynx::lepus::OP_PLACEHOLDER)};
  for (auto& inst : code) {
    lepus_func->AddInstruction(inst);
  }
  func->Init(lepus_func);

  SSABuilder ssaBuilder(ir_ctx.get(), func);
  EXPECT_THROW(ssaBuilder.Build(), ::lynx::lepus::CompileException);
}

}  // namespace test
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
