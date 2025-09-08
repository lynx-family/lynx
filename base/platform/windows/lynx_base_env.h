// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_WINDOWS_LYNX_BASE_ENV_H_
#define BASE_PLATFORM_WINDOWS_LYNX_BASE_ENV_H_

namespace lynx {
namespace base {
class LynxBaseEnv {
 public:
  LynxBaseEnv() = default;
  ~LynxBaseEnv() = default;
  static LynxBaseEnv* Instance();

  void Init(bool is_print_log_to_all_channel);
  void OnlyInitBaseTrace();
};
}  // namespace base
}  // namespace lynx
#endif  // BASE_PLATFORM_WINDOWS_LYNX_BASE_ENV_H_
