// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/hvm_register_allocator.h"

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"

namespace lynx {

namespace lepus {
namespace ir {
void HVMRegisterAllocator::HandleInstruction(Instruction* i) {}

bool HVMRegisterAllocator::HasTargetSpecificLowering(Instruction* i) {
  return llvh::isa<CallInst>(i);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
