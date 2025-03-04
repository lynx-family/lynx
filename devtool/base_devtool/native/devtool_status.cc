// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/public/devtool_status.h"

namespace lynx {
namespace devtool {

const char* DevToolStatus::SCREENSHOT_MODE_FULLSCREEN = "fullscreen";
const char* DevToolStatus::SCREENSHOT_MODE_LYNXVIEW = "lynxview";

DevToolStatus& DevToolStatus::GetInstance() {
  static DevToolStatus instance;
  return instance;
}

std::string DevToolStatus::GetStatus(const DevToolStatusKey& key,
                                     std::string default_value) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string value = config_[key];
  if (value.empty()) {
    value = default_value;
  }
  return value;
}

void DevToolStatus::SetStatus(const DevToolStatusKey& key,
                              const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  config_[key] = value;
}

}  // namespace devtool
}  // namespace lynx
