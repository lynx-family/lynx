// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_HVM_REGISTER_ALLOCATOR_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_HVM_REGISTER_ALLOCATOR_H_

#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {

namespace lepus {
namespace ir {

class HVMRegisterAllocator : public RegisterAllocator {
 private:
  unsigned max_parameter_count_ = 0;
  unsigned spill_count_ = 0;

 protected:
  void HandleInstruction(Instruction* i) override;
  bool HasTargetSpecificLowering(Instruction* i) override;

 public:
  explicit HVMRegisterAllocator(FuncOp* func) : RegisterAllocator(func) {}
  virtual ~HVMRegisterAllocator() = default;

  Register GetLastRegister() { return Register(GetMaxRegisterUsage() - 1); }

  /// Get the maximum number of registers used.
  unsigned GetMaxRegisterUsage() override {
    return GetMaxInstructionRegister() + spill_count_ + max_parameter_count_;
  }

  /// Get the maximum register allocated to regular instructions
  /// (i.e. not including parameter lists or spilling).
  unsigned GetMaxInstructionRegister() {
    return RegisterAllocator::GetMaxRegisterUsage();
  }

  void AllocateParameterCount(unsigned count) {
    if (max_parameter_count_ < count) {
      max_parameter_count_ = count;
    }
  }
  void AllocateSpillTempCount(unsigned count) { spill_count_ = count; }
  unsigned GetSpillOffset() { return RegisterAllocator::GetMaxRegisterUsage(); }
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_HVM_REGISTER_ALLOCATOR_H_
