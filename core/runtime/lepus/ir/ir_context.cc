// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/ir_context.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

IRContext::IRContext(VMContext* vm_ctx) : context_(vm_ctx) {
  builder_ = std::make_unique<OpBuilder>();

  main_mod_ =
      std::make_unique<ModuleOp>(nullptr, builder_.get(), 0, this, "mod");
  builder_->SetModuleOp(main_mod_.get());
  main_mod_->Init();
  top_level_variables_.clear();
}

IRContext::~IRContext() {}

void IRContext::InsertToplevelValue(Instruction* val, unsigned reg) {
  val->SetToplevelVarReg(reg);
  if (top_level_variables_.find(reg) != top_level_variables_.end()) return;
  top_level_variables_.insert({reg, val});
}

std::map<unsigned, Instruction*>& IRContext::GetToplevelVariables() {
  return top_level_variables_;
}

void IRContext::UpdateToplevelVar(Instruction* origin, Instruction* new_val) {
  if (LEPUS_UNLIKELY(!new_val)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRContext::UpdateToplevelVar called with nullptr "
        "new_val");
  }
  auto reg = origin->GetToplevelVarReg();
  if (reg == -1) return;
  const unsigned unsigned_reg = static_cast<unsigned>(reg);
  if (LEPUS_UNLIKELY(top_level_variables_.find(unsigned_reg) ==
                     top_level_variables_.end())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRContext::UpdateToplevelVar cannot find existing "
        "toplevel var mapping");
  }
  new_val->SetToplevelVarReg(reg);
  auto it = top_level_variables_.find(unsigned_reg);
  if (it != top_level_variables_.end() && it->second == origin) {
    it->second = new_val;
  }
}

ModuleOp* IRContext::GetMainMod() {
  if (LEPUS_UNLIKELY(!main_mod_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRContext::GetMainMod expects non-null module");
  }
  return main_mod_.get();
}

FuncOp* IRContext::GetFuncOp(
    const fml::RefPtr<::lynx::lepus::Function>& func) const {
  auto it = funcs_.find(func);
  if (it == funcs_.end()) return nullptr;
  return it->second;
}

void IRContext::RegisterFuncOp(const fml::RefPtr<::lynx::lepus::Function>& func,
                               FuncOp* op) {
  if (LEPUS_UNLIKELY(!func)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRContext::RegisterFuncOp called with nullptr "
        "lepus::Function");
  }
  if (LEPUS_UNLIKELY(!op)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRContext::RegisterFuncOp called with nullptr "
        "FuncOp");
  }

  auto it = funcs_.find(func);
  if (it != funcs_.end() && it->second != op) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRContext::RegisterFuncOp detected conflicting "
        "FuncOp mapping for the same lepus::Function");
  }
  funcs_[func] = op;
}

void IRContext::CollectChildFuncs(const fml::RefPtr<Function>& function) {
  for (auto child : function->GetChildFunction()) {
    auto func_name = child->GetFunctionName();
    if (func_name == "") {
      auto mod = GetMainMod();
      func_name = "closure_" + std::to_string(mod->GenerateClosureFunctionId());
    }

    auto* func = builder_->Create<FuncOp>(child->CurrentLineCol(), func_name);
    func->Init(child);

    CollectChildFuncs(child);
  }
}

void IRContext::SetTargetContext(std::unique_ptr<TargetContext>& target_ctx) {
  target_context_ = std::move(target_ctx);
}

TargetContext* IRContext::GetTargetContext() { return target_context_.get(); }

void IRContext::Init(fml::RefPtr<Function>& root_function, VMContext* context) {
  if (LEPUS_UNLIKELY(!root_function->IsToplevelFunction())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRContext::Init expects root_function to be a "
        "toplevel function");
  }
  context_ = context;
  auto* builder = GetOpBuilder();
  auto* mod = GetMainMod();
  builder->SetInsertionPointToEnd(mod->GetFunctionBlock());

  auto func_name = root_function->GetFunctionName();
  if (func_name == "") {
    func_name = "closure_" + std::to_string(mod->GenerateClosureFunctionId());
  }

  auto* func =
      builder->Create<FuncOp>(root_function->CurrentLineCol(), func_name);
  func->SetTopLevelFunction();
  mod->SetRootFunction(func);
  func->Init(root_function);
  CollectChildFuncs(root_function);
}

void IRContext::UpdateSpecialAttribute(Instruction* old_val, Value* new_val) {
  unsigned reg = old_val->GetClosureVarReg();
  if (reg != -1) {
    auto* root_func = GetMainMod()->GetRootFunction();
    root_func->UpdateClosureVar(old_val, new_val);
  }

  reg = old_val->GetToplevelVarReg();
  if (reg != constants::kInvalidSignedValue &&
      llvh::isa<Instruction>(new_val)) {
    UpdateToplevelVar(old_val, llvh::cast<Instruction>(new_val));
  }
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
