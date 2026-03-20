// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/register_allocation_pass.h"

#include <memory>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/transformer/vm/hvm_register_allocator.h"

namespace lynx {
namespace lepus {
namespace ir {
bool RegisterAllocationPass::RunOnFunction(FuncOp* func) {
  std::unique_ptr<RegisterAllocator> ra(new HVMRegisterAllocator(func));
  PostOrderAnalysis po(func);
  llvh::SmallVector<Block*, 16> order(po.rbegin(), po.rend());
  ra->Preallocate();
  ra->Allocate(order);

  if (LEPUS_UNLIKELY(!ir_ctx_->GetTargetContext())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterAllocationPass requires non-null "
        "TargetContext");
  }
  ir_ctx_->GetTargetContext()->SetRegisterAllocAnalysis(func, ra);
  return true;
}

Pass* CreateRegisterAllocationPass(IRContext* ir_ctx) {
  return new RegisterAllocationPass(ir_ctx);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
