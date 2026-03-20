// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PASS_H_
#define CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PASS_H_

#include <string>

#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

class FuncOp;
class ModuleOp;
class Instruction;
class IRContext;

class Pass {
 public:
  enum class PassKind : uint8_t {
    Function,
    ModuleOp,
  };

  void SetMode(StageMode mode) { mode_ = mode; }

  StageMode GetMode() const { return mode_; }

 private:
 public:
  /// Constructor. \p K indicates the kind of pass this is.
  explicit Pass(Pass::PassKind k, IRContext* ir_ctx, const std::string& name)
      : kind_(k), name_(name), ir_ctx_(ir_ctx) {}

  virtual ~Pass() = default;

  /// Returns the kind of the pass.
  PassKind GetKind() const { return kind_; }

  /// Returns the textual name of the pass.
  std::string GetName() const { return name_; }

 protected:
  const PassKind kind_;
  std::string name_;
  IRContext* ir_ctx_ = nullptr;
  StageMode mode_ = StageMode::SM_HIR;
};

class FunctionPass : public Pass {
 public:
  explicit FunctionPass(IRContext* ir_ctx, const std::string& name)
      : Pass(Pass::PassKind::Function, ir_ctx, name) {}
  ~FunctionPass() override = default;

  /// Runs the current pass on the function \p F.
  /// \returns true if the function was modified.
  virtual bool RunOnFunction(FuncOp* f) = 0;

  static bool classof(const Pass* s) {
    return s->GetKind() == PassKind::Function;
  }
};

class ModulePass : public Pass {
 public:
  explicit ModulePass(IRContext* ir_ctx, const std::string name)
      : Pass(Pass::PassKind::ModuleOp, ir_ctx, name) {}
  ~ModulePass() override = default;

  /// Runs the current pass on the module \p M.
  /// \returns true if module was modified.
  virtual bool RunOnModule(ModuleOp* mod) = 0;

  static bool classof(const Pass* pass) {
    return pass->GetKind() == PassKind::ModuleOp;
  }
};

/// Pass header declaration.
#define PASS(ID, NAME, DESCRIPTION) Pass* Create##ID(IRContext* ir_ctx);
#include "core/runtime/lepus/ir/pass_manager/passes.def"

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PASS_H_
