// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_BUILDER_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_BUILDER_H_

#include "core/runtime/lepus/ir/op_builder.h"

namespace lynx {
namespace lepus {
namespace ir {

class OpBuilder;
class IRContext;
class FuncOp;

class SSABuilder {
 public:
  SSABuilder(IRContext* ir_ctx, FuncOp* func);
  void Build();

 private:
  IRContext* ir_ctx_;
  FuncOp* func_ = nullptr;
  int32_t param_count_;
  uint32_t register_count_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_BYTECODE_BUILDER_H_
