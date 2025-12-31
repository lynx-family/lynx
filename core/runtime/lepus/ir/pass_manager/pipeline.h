// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PIPELINE_H_
#define CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PIPELINE_H_

#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

class ModuleOp;
class PassManager;

void addMIRPasses(PassManager& pm);
void addTargetPasses(PassManager& pm);
void RunO1OptimizationPasses(ModuleOp& m, const char* ir_dump_path = nullptr);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PIPELINE_H_
