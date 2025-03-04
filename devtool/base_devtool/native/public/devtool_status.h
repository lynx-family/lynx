// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_STATUS_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_STATUS_H_
#include <mutex>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/native/public/base_devtool_export.h"

namespace lynx {
namespace devtool {

/**
 * Store some status of devtool
 */
class BASE_DEVTOOL_EXPORT DevToolStatus {
 public:
  enum DevToolStatusKey {
    kDevToolStatusKeyIsConnected =
        0,  // devtool is whether connect to debug platform or not
    kDevToolStatusKeyScreenShotMode
  };

 public:
  static const char* SCREENSHOT_MODE_FULLSCREEN;
  static const char* SCREENSHOT_MODE_LYNXVIEW;

 public:
  static DevToolStatus& GetInstance();
  DevToolStatus(const DevToolStatus&) = delete;
  DevToolStatus& operator=(const DevToolStatus&) = delete;
  std::string GetStatus(const DevToolStatusKey& key,
                        std::string default_value = "");
  void SetStatus(const DevToolStatusKey& key, const std::string& value);

 private:
  DevToolStatus() = default;
  std::unordered_map<DevToolStatusKey, std::string> config_;
  std::mutex mutex_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_STATUS_H_
