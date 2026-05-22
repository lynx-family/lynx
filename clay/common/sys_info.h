// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_SYS_INFO_H_
#define CLAY_COMMON_SYS_INFO_H_

#include <cstdint>
#include <optional>

namespace clay {

class SysInfo {
 public:
  static int64_t AmountOfPhysicalMemory();
  static bool IsLowEndDevice();
  static void SetCustomIsLowEndDevice(bool value);

 private:
  // Custom set by online settings or front end page.
  static std::optional<bool> g_custom_is_low_end_device;
};
}  // namespace clay
#endif  // CLAY_COMMON_SYS_INFO_H_
