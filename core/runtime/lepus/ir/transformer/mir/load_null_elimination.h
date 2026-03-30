// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_NULL_ELIMINATION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_NULL_ELIMINATION_H_

#include "core/runtime/lepus/ir/pass_manager/pass.h"

namespace lynx {
namespace lepus {
namespace ir {

// Canonicalize redundant LoadNullOrUndefinedInst.
//
// Instead of always hoisting to the entry block, this pass places a single
// canonical load at the nearest common dominator block of all occurrences for
// the same load type.
class LoadNullEliminationPass : public FunctionPass {
 public:
  explicit LoadNullEliminationPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "load-null-elimination") {}
  ~LoadNullEliminationPass() override = default;

  bool RunOnFunction(FuncOp* f) override;
};

Pass* CreateLoadNullEliminationPass(IRContext* ir_ctx);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_MIR_LOAD_NULL_ELIMINATION_H_
