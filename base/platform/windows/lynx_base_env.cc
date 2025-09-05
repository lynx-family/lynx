// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/platform/windows/lynx_base_env.h"

#include "base/include/log/logging_base.h"

namespace lynx {
namespace base {

LynxBaseEnv* LynxBaseEnv::Instance() {
  static LynxBaseEnv instance;
  return &instance;
}

void LynxBaseEnv::Init(bool is_print_log_to_all_channel) {
  lynx::InitLynxBaseLog(is_print_log_to_all_channel);
}
}  // namespace base
}  // namespace lynx
