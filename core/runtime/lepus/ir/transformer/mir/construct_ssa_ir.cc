// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/construct_ssa_ir.h"

namespace lynx {
namespace lepus {
namespace ir {
bool ConstructSSAIRPass::RunOnFunction(FuncOp* func) {
  SSABuilder builder(ir_ctx_, func);
  builder.Build();

  return true;
}

Pass* CreateConstructSSAIRPass(IRContext* ir_ctx) {
  return new ConstructSSAIRPass(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
