// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// clang-format off
#include "core/runtime/lepus/ir/value_forward_declare.h"
// clang-format on

#include "core/runtime/lepus/ir/func_op.h"

#ifdef LEPUS_TEST
#include "core/runtime/lepus/bytecode_print.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_dumper.h"
#endif
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {
FuncOp::FuncOp(Block* parent, OpBuilder* builder, int64_t location,
               std::string& name)
    : Operation(ValueKind::FuncOpKind, parent, builder, location), name_(name) {
  ret_ty_ = TypeOp::CreateAnyType(builder);
  SetType(ret_ty_);
  [[maybe_unused]] auto op = parent->GetParent()->GetParent();
  if (LEPUS_UNLIKELY(!op)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FuncOp constructed with invalid parent chain (missing "
        "ModuleOp)");
  }
}

FuncOp::~FuncOp() {}

Parameter* FuncOp::CreateParam(int32_t param_index) {
  auto* param = new Parameter(this, param_index);
  if (LEPUS_UNLIKELY(!param)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FuncOp::CreateParam failed to allocate Parameter");
  }
  auto builder = ir_ctx_->GetOpBuilder();
  param->SetType(TypeOp::CreateAnyType(builder));
  this->AddParam(param);
  return param;
}

void FuncOp::AddParam(Parameter* p) { params_.push_back(p); }

void FuncOp::Init(fml::RefPtr<Function>& function) {
  lepus_function_ = function;

  auto* ir_ctx = GetIRCtx();
  // 0. create region and block
  auto builder = ir_ctx->GetOpBuilder();
  builder->CreateRegion(this);
}

Operation* FuncOp::GetParentOp() const {
  return GetParentRegion()->GetParent();
}

void FuncOp::Dump(StageMode mode, std::ostream& os) const {
  if (mode == StageMode::SM_MIR) {
#ifdef LEPUS_TEST
    MIRPrinter printer(ir_ctx_, os);
    printer.VisitFuncOp(*this);
#endif
  } else if (mode == StageMode::SM_REG_ALLOC) {
#ifdef LEPUS_TEST
    FuncOp* this_func = const_cast<FuncOp*>(this);
    if (LEPUS_UNLIKELY(!ir_ctx_->GetTargetContext())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: FuncOp::Dump(SM_REG_ALLOC) requires non-null "
          "TargetContext");
    }
    auto ra = ir_ctx_->GetTargetContext()->GetRegisterAllocAnalysis(this_func);
    if (LEPUS_UNLIKELY(!ra)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: FuncOp::Dump(SM_REG_ALLOC) requires "
          "RegisterAllocator analysis");
    }
    ra->Dump(os);
#endif
  } else if (mode == StageMode::SM_VM_TARGET) {
#ifdef LEPUS_TEST
    FuncOp* this_func = const_cast<FuncOp*>(this);
    auto lepus_function = this_func->GetLepusFunction();
    Dumper dumper(nullptr, os);
    dumper.DumpFunction(lepus_function);
#endif
  }
  return;
}

void FuncOp::RecordClosureVarRegAndValue(uint32_t closure_reg, Value* value) {
  if (closure_reg_to_value_.find(closure_reg) != closure_reg_to_value_.end()) {
    if (LEPUS_UNLIKELY(closure_reg_to_value_[closure_reg] != value)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: FuncOp::RecordClosureVarRegAndValue found "
          "conflicting value for the same old register");
    }
    return;
  }
  closure_reg_to_value_[closure_reg] = value;
}

void FuncOp::UpdateClosureVar(Value* origin, Value* update) {
  auto reg = origin->GetClosureVarReg();
  if (reg == -1) return;
  update->SetClosureVarReg(reg);
  if (LEPUS_UNLIKELY(closure_reg_to_value_.find(reg) ==
                     closure_reg_to_value_.end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FuncOp::UpdateClosureVar cannot find closure var "
        "mapping for old reg");
  }
  if (closure_reg_to_value_[reg] == origin) {
    closure_reg_to_value_[reg] = update;
  }
}

Value* FuncOp::GetClosureVarGivenReg(uint32_t closure_reg) {
  if (LEPUS_UNLIKELY(closure_reg_to_value_.find(closure_reg) ==
                     closure_reg_to_value_.end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FuncOp::GetClosureVarGivenReg cannot find closure var "
        "for old reg");
  }
  return closure_reg_to_value_[closure_reg];
}

long FuncOp::GetClosureVarToplevelReg(long index) {
  auto it = upvalue_index_to_toplevel_reg_.find(index);
  if (it == upvalue_index_to_toplevel_reg_.end()) {
    return -1;
  }
  return it->second;
}

void FuncOp::RecordUpvalueIndex2ToplevelReg(long index, long reg_index) {
  upvalue_index_to_toplevel_reg_[index] = reg_index;
}

std::string FuncOp::GetNameForDump() const {
  return GetBaseName() + "(" + name_ + ")";
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

void llvh::ilist_alloc_traits<::lynx::lepus::ir::FuncOp>::deleteNode(
    ::lynx::lepus::ir::FuncOp* v) {
  ::lynx::lepus::ir::Value::Destroy(v);
}
