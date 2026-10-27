// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/common/sys_info.h"

#include "build/build_config.h"
#include "clay/fml/logging.h"

namespace clay {

std::optional<bool> SysInfo::g_custom_is_low_end_device;

int64_t SysInfo::AmountOfPhysicalMemory() {
  FML_LOG(WARNING) << "AmountOfPhysicalMemory not implemented.";
  return 0;
}
bool SysInfo::IsLowEndDevice() {
  bool custom_is_low_end = false;
  if (g_custom_is_low_end_device.has_value()) {
    custom_is_low_end = g_custom_is_low_end_device.value();
  }
  return custom_is_low_end;
}
void SysInfo::SetCustomIsLowEndDevice(bool value) {
  g_custom_is_low_end_device = value;
}

}  // namespace clay
