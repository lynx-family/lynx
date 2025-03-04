// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_BYTECODE_GENERATOR_H_
#define CORE_RUNTIME_VM_LEPUS_BYTECODE_GENERATOR_H_

#include <string>

#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

class BytecodeGenerator {
 public:
  static std::string GenerateBytecode(Context* context,
                                      const std::string& source,
                                      const std::string& sdk_version);

 private:
  static std::string GenerateBytecodeForVMContext(
      VMContext* context, const std::string& source,
      const std::string& sdk_version);
  static std::string GenerateBytecodeForQuickContext(
      QuickContext* context, const std::string& source,
      const std::string& sdk_version);
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_BYTECODE_GENERATOR_H_
