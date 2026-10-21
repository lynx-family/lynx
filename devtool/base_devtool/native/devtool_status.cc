// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/public/devtool_status.h"

namespace lynx {
namespace devtool {

const char* DevToolStatus::SCREENSHOT_MODE_FULLSCREEN = "fullscreen";
const char* DevToolStatus::SCREENSHOT_MODE_LYNXVIEW = "lynxview";
const char* DevToolStatus::NO_DEBUG_INFO_FOUND_BY_URL =
    "NO_DEBUG_INFO_FOUND_BY_URL";

DevToolStatus& DevToolStatus::GetInstance() {
  static DevToolStatus instance;
  return instance;
}

std::string DevToolStatus::GetStatus(const DevToolStatusKey& key,
                                     const std::string& default_value) {
  std::lock_guard<std::mutex> lock(mutex_);
  switch (key) {
    case kDevToolStatusKeyIsConnected:
      return connection_status_.empty() ? default_value : connection_status_;
    case kDevToolStatusKeyScreenShotMode:
      return screenshot_mode_;
  }
  return default_value;
}

void DevToolStatus::SetStatus(const DevToolStatusKey& key,
                              const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  switch (key) {
    case kDevToolStatusKeyIsConnected:
      connection_status_ = value;
      break;
    case kDevToolStatusKeyScreenShotMode:
      // Clearing the status restores the default. Ignore unsupported modes so
      // all coordinate consumers see the same valid mode.
      if (value.empty()) {
        screenshot_mode_ = SCREENSHOT_MODE_FULLSCREEN;
      } else if (value == SCREENSHOT_MODE_FULLSCREEN ||
                 value == SCREENSHOT_MODE_LYNXVIEW) {
        screenshot_mode_ = value;
      }
      break;
  }
}

}  // namespace devtool
}  // namespace lynx
