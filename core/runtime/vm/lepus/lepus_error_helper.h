// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_ERROR_HELPER_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_ERROR_HELPER_H_

#include <iostream>
#include <string>

#include "base/include/log/logging.h"
#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace lepus {
class LepusErrorHelper {
 public:
  static std::string GetErrorStack(LEPUSContext* ctx, LEPUSValue& value);

  static std::string GetErrorMessage(LEPUSContext* ctx,
                                     LEPUSValue& exception_value);

  static int32_t GetErrorCode(LEPUSContext*, LEPUSValue);
  static constexpr const char* err_code_prop_ = "__error_code__";
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_LEPUS_ERROR_HELPER_H_
