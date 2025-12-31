// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/type_op.h"

namespace lynx {
namespace lepus {
namespace ir {

class DumpOpcodeBeforeOptPass : public FunctionPass {
 public:
  explicit DumpOpcodeBeforeOptPass(IRContext* ir_ctx)
      : FunctionPass(ir_ctx, "dump-opcode-before-opt") {}
  ~DumpOpcodeBeforeOptPass() override = default;
  bool RunOnFunction(FuncOp* func) override;
};

bool DumpOpcodeBeforeOptPass::RunOnFunction(FuncOp* func) { return true; }

Pass* CreateDumpOpcodeBeforeOptPass(IRContext* ir_ctx) {
  return new DumpOpcodeBeforeOptPass(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
