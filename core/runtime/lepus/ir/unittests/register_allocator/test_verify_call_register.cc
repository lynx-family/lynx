// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"
#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"
#include "core/runtime/lepus/ir/transformer/vm/verify_call_register_pass.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRVerifyCallRegisterTest : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }

  FuncOp* createTestFunction(std::string name) {
    auto builder = ir_ctx->GetOpBuilder();
    builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

    auto* func_op = builder->Create<FuncOp>(0, name);
    auto region = builder->CreateRegion(func_op);
    builder->CreateBlock(region, BlockType::BT_INST, {});
    return func_op;
  }
};

TEST_F(LEPUSIRVerifyCallRegisterTest, PassVerifiesCorrectAllocation) {
  auto* func = createTestFunction("test_correct_alloc");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Live variable r1
  auto* val = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));

  // Callee
  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));

  // Call
  ArgList args;
  args.push_back(val);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, callee, args);

  // Use val after call to keep it live (across call)
  builder->Create<ReturnInst>(0, val);

  // Manual allocation to simulate RA
  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  Value* func_val = call->GetFunction();

  // Setup safe allocation:
  // val -> r1
  // callee -> r3 (clobber range [4, 4] does not hit val)
  ra->UpdateRegister(val, Register(1));
  ra->UpdateRegister(func_val, Register(3));

  VerifyCallRegisterPass verify(ir_ctx.get());
  // Should not crash
  verify.RunOnFunction(func);
}

TEST_F(LEPUSIRVerifyCallRegisterTest, PassFailsIncorrectAllocation) {
  auto* func = createTestFunction("test_bad_alloc");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Live variable r5
  auto* val = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));

  // Callee r2 ( < r5 )
  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));

  // Argument value.
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(200),
                                             TypeOp::CreateInt32(builder));

  // Call
  ArgList args;
  args.push_back(arg);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, callee, args);

  // Use val after call
  builder->Create<ReturnInst>(0, val);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);

  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  Value* func_val = call->GetFunction();

  // Setup UNSAFE allocation:
  // callee -> r5, argc=1 => clobber range [6, 6]
  // val -> r6 (live across call, but NOT an argument)
  // arg -> r7
  // clobber overlaps live register r6 => should fail.
  ra->UpdateRegister(val, Register(6));
  ra->UpdateRegister(arg, Register(7));
  ra->UpdateRegister(func_val, Register(5));

  VerifyCallRegisterPass verify(ir_ctx.get());
  try {
    verify.RunOnFunction(func);
    FAIL() << "Expected VerifyCallRegisterPass to fail";
  } catch (const ::lynx::lepus::CompileException& e) {
    const std::string msg = e.message();
    ASSERT_NE(msg.find("VerifyCallRegisterPass detected"), std::string::npos)
        << msg;
  }
}

TEST_F(LEPUSIRVerifyCallRegisterTest, PassSucceedsIfArgAlreadyInDestAndLive) {
  auto* func = createTestFunction("test_arg_in_dest_live");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                             TypeOp::CreateInt32(builder));
  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));

  ArgList args;
  args.push_back(arg);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, callee, args);
  // Keep arg live across call.
  builder->Create<ReturnInst>(0, arg);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  Value* func_val = call->GetFunction();

  // callee -> r5, argc=1 => destination for arg is r6.
  // Place arg directly into destination register, so materialization is a
  // no-op (self-copy) and should be allowed even if arg is live.
  ra->UpdateRegister(callee, Register(5));
  ra->UpdateRegister(arg, Register(6));
  ra->UpdateRegister(func_val, Register(5));

  VerifyCallRegisterPass verify(ir_ctx.get());
  verify.RunOnFunction(func);
}

TEST_F(LEPUSIRVerifyCallRegisterTest,
       PassSucceedsIfLiveValueAboveClobberRange) {
  auto* func = createTestFunction("test_no_clobber_overlap");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* live = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                              TypeOp::CreateInt32(builder));
  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));

  ArgList args;
  args.push_back(live);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, callee, args);
  builder->Create<ReturnInst>(0, live);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);

  // live -> r100 (live across call)
  // callee -> r5, argc=1 => clobber range [6, 6]
  // live is above clobber range, should succeed.
  ra->UpdateRegister(live, Register(100));
  ra->UpdateRegister(callee, Register(5));

  VerifyCallRegisterPass verify(ir_ctx.get());
  verify.RunOnFunction(func);
}

TEST_F(LEPUSIRVerifyCallRegisterTest,
       PassSucceedsIfMultipleArgsAlreadyInDestAndLive) {
  auto* func = createTestFunction("test_multi_args_in_dest_live");
  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* arg0 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(100),
                                              TypeOp::CreateInt32(builder));
  auto* arg1 = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(200),
                                              TypeOp::CreateInt32(builder));
  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));

  ArgList args;
  args.push_back(arg0);
  args.push_back(arg1);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, callee, args);

  // Keep both args live across call.
  auto* sum = builder->Create<BinaryOperatorInst>(0, arg0, arg1,
                                                  ValueKind::BinaryAddInstKind,
                                                  TypeOp::CreateInt32(builder));
  builder->Create<ReturnInst>(0, sum);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  Value* func_val = call->GetFunction();

  // callee -> r5, argc=2 => destinations are r6 and r7.
  // Place args directly into destination registers, so materialization is
  // no-op (self-copy) for both slots.
  ra->UpdateRegister(func_val, Register(5));
  ra->UpdateRegister(arg0, Register(6));
  ra->UpdateRegister(arg1, Register(7));

  VerifyCallRegisterPass verify(ir_ctx.get());
  verify.RunOnFunction(func);
}

TEST_F(LEPUSIRVerifyCallRegisterTest, PassFailsIfToplevelVarInClobberRange) {
  auto* func = createTestFunction("test_toplevel_var_clobber_conflict");
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);

  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  auto* toplevel_val = builder->Create<LoadConstInst>(
      0, builder->GetLiteralInt32(1), TypeOp::CreateInt32(builder));
  // Insert toplevel mapping so VerifyCallRegisterPass checks it.
  ir_ctx->InsertToplevelValue(toplevel_val, 0);

  auto* callee =
      builder->Create<GetUpvalueInst>(0, func, builder->GetLiteralUint8(0));
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(2),
                                             TypeOp::CreateInt32(builder));

  ArgList args;
  args.push_back(arg);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, callee, args);

  builder->Create<ReturnInst>(0, arg);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);
  Value* func_val = call->GetFunction();

  // callee -> r5, argc=1 => clobber range [6, 6]
  // Put toplevel var at r6 so it must not be clobbered.
  ra->UpdateRegister(func_val, Register(5));
  ra->UpdateRegister(arg, Register(7));
  ra->UpdateRegister(toplevel_val, Register(6));

  VerifyCallRegisterPass verify(ir_ctx.get());
  try {
    verify.RunOnFunction(func);
    FAIL() << "Expected VerifyCallRegisterPass to fail";
  } catch (const ::lynx::lepus::CompileException& e) {
    const std::string msg = e.message();
    ASSERT_NE(msg.find("VerifyCallRegisterPass detected"), std::string::npos)
        << msg;
  }
}

TEST_F(LEPUSIRVerifyCallRegisterTest, PassFailsIfParamInClobberRange) {
  auto* func = createTestFunction("test_param_clobber_conflict");
  // Create 2 parameters to simulate a param prefix.
  auto* p0 = func->CreateParam(0);
  auto* p1 = func->CreateParam(1);

  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  // Call with 1 argument.
  ArgList args;
  auto* arg = builder->Create<LoadConstInst>(0, builder->GetLiteralInt32(123),
                                             TypeOp::CreateInt32(builder));
  args.push_back(arg);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, p0, args);
  builder->Create<ReturnInst>(0, arg);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);

  // RegisterAllocationPass may have already fixed the call by inserting a
  // call-func MovInst. Force an UNSAFE state to validate the verifier.
  call->SetOperand(p0, CallInst::methodIdx);

  // Place callee in r0, argc=1 => clobber range [1, 1].
  // Put p1 in r1 so it would be clobbered by argument materialization.
  ra->UpdateRegister(p0, Register(0));
  ra->UpdateRegister(p1, Register(1));
  ra->UpdateRegister(arg, Register(2));

  // Ensure the CallInst has an instruction number.
  ra->GetInstructionNumber(call);

  VerifyCallRegisterPass verify(ir_ctx.get());
  try {
    verify.RunOnFunction(func);
    FAIL() << "Expected VerifyCallRegisterPass to fail";
  } catch (const ::lynx::lepus::CompileException& e) {
    const std::string msg = e.message();
    ASSERT_NE(msg.find("VerifyCallRegisterPass detected"), std::string::npos)
        << msg;
  }
}

TEST_F(LEPUSIRVerifyCallRegisterTest,
       PassSucceedsIfParamIsArgAndAlreadyInDest) {
  auto* func = createTestFunction("test_param_as_arg_in_dest");
  auto* p0 = func->CreateParam(0);
  auto* p1 = func->CreateParam(1);

  auto builder = ir_ctx->GetOpBuilder();
  Block* block = &func->Front();
  builder->SetInsertionPointToEnd(block);

  ArgList args;
  // Use p1 as the argument; when dest is r1 and p1 is already in r1,
  // materialization is a no-op (self-copy).
  args.push_back(p1);
  [[maybe_unused]] auto* call = builder->Create<CallInst>(0, p0, args);
  builder->Create<ReturnInst>(0, p1);

  RegisterAllocationPass ra_pass(ir_ctx.get());
  ra_pass.RunOnFunction(func);
  auto* ra = ir_ctx->GetTargetContext()->GetRegisterAllocAnalysis(func);

  ra->UpdateRegister(p0, Register(0));
  ra->UpdateRegister(p1, Register(1));

  VerifyCallRegisterPass verify(ir_ctx.get());
  verify.RunOnFunction(func);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
