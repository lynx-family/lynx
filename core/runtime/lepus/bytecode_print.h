// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_BYTECODE_PRINT_H_
#define CORE_RUNTIME_LEPUS_BYTECODE_PRINT_H_

#include <chrono>
#include <iostream>
#include <ostream>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "core/runtime/lepus/vm_context.h"

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
  explicit Dumper(Function* function, std::ostream& os = std::cout)
      : root_(function), os_(os) {}
  void Dump();
  void DumpFunction();
  void DumpFunction(fml::RefPtr<Function>& func_ptr);

 private:
  void ProcessSpecialInstructions(Instruction* ins, size_t& i);
  void PrintOpCode(Instruction ins, Function* func_ptr, int i);
  void PrintDetail(const char* oper, int nums, long offsets[],
                   OffsetScope scope_id[]);
  void DumpScope(Function* func_ptr);
  void DumpBlockScope(Function* func_ptr, const Value& scopes, int32_t intend);
  Function* root_;
  std::vector<Function*> functions_;
  std::ostream& os_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BYTECODE_PRINT_H_
