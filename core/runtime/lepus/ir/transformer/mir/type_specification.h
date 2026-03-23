// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_TYPE_SPECIFICATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_TYPE_SPECIFICATION_H_

#include <string>

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

class LoadConstInst;

class TypeSpecification : public FunctionPass {
 public:
  explicit TypeSpecification(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "type-specification") {}
  ~TypeSpecification() override = default;
  bool RunOnFunction(FuncOp* func) override;
  const std::string& GetConstString(LoadConstInst* load_const_inst);
  void Reset(FuncOp* func);
  void SpecifyGetTableForStringProtoType();
  void PropagateTypes();

 private:
  FuncOp* func_;
  llvh::SmallVector<Operation*, 16> to_removed_;
};

Pass* CreateTypeSpecificationPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_TYPE_SPECIFICATION_H_
