// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_BYTECODE_PRINT_H_
#define CORE_RUNTIME_VM_LEPUS_BYTECODE_PRINT_H_

#include <chrono>
#include <vector>

#include "base/include/log/logging.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/vm_context.h"
namespace lynx {
namespace lepus {
enum OffsetScope {
  Normal = 0,
  Global = 1,
  Constant,
  Clo  // represents Closure type
};

class Dumper {
 public:
  Dumper(Function* r) : root(r) {}
  void Dump();
  void DumpFunction();

 private:
  Function* root;
  void PrintOpCode(Instruction ins, Function* func_ptr, int i);
  void PrintDetail(const char* oper, int nums, long offsets[],
                   OffsetScope scopeId[]);
  std::vector<Function*> functions_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_BYTECODE_PRINT_H_
