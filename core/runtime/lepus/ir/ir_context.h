// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_IR_CONTEXT_H_
#define CORE_RUNTIME_LEPUS_IR_IR_CONTEXT_H_

#include <map>
#include <memory>
#include <unordered_map>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_string.h"
#include "core/runtime/lepus/ir/attributes.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/primitive_typeop.h"
#include "core/runtime/lepus/ir/target_context.h"
#include "core/runtime/lepus/ir/type_op.h"

namespace lynx {
namespace lepus {

class Function;
class VMContext;

namespace ir {

class ModuleOp;
class ClassOp;
class OpBuilder;
class FuncOp;

class IRContext {
 public:
  explicit IRContext(VMContext* vm_ctx);
  ~IRContext();
  OpBuilder* GetOpBuilder() { return builder_.get(); }
  ModuleOp* GetMainMod();
  FuncOp* GetFuncOp(const fml::RefPtr<::lynx::lepus::Function>& func) const;

  // Register a mapping between a lepus::Function and its corresponding FuncOp.
  //
  // IRContext::Init()/CollectChildFuncs() will populate this mapping for the
  // normal pipeline, but unit tests or other ad-hoc builders may construct
  // FuncOp manually. RegisterFuncOp makes passes that rely on GetFuncOp more
  // robust in those scenarios.
  void RegisterFuncOp(const fml::RefPtr<::lynx::lepus::Function>& func,
                      FuncOp* op);
  void Init(fml::RefPtr<::lynx::lepus::Function>& root_function,
            lynx::lepus::VMContext* context);
  void CollectChildFuncs(const fml::RefPtr<Function>& function);

  llvh::DenseMap<uint32_t, PrimitiveTypeOp*>& GetPrimitiveTypeMap() {
    return primitive_types_;
  }

  VMContext* GetVMContext() { return context_; }

  void SetTargetContext(std::unique_ptr<TargetContext>& target_ctx);
  TargetContext* GetTargetContext();

  void InsertToplevelValue(Instruction* val, unsigned reg);
  std::map<unsigned, Instruction*>& GetToplevelVariables();
  void UpdateToplevelVar(Instruction* origin, Instruction* new_val);

  void UpdateSpecialAttribute(Instruction* old_val, Value* new_val);

 private:
  VMContext* context_ = nullptr;
  std::unique_ptr<OpBuilder> builder_;
  std::unique_ptr<ModuleOp> main_mod_;
  llvh::DenseMap<uint32_t, PrimitiveTypeOp*> primitive_types_;
  std::unordered_map<fml::RefPtr<Function>, FuncOp*> funcs_ = {};
  std::unique_ptr<TargetContext> target_context_;
  // key: toplevel reg idx, value: toplevel vars in this reg
  std::map<unsigned, Instruction*> top_level_variables_{};
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_IR_CONTEXT_H_
