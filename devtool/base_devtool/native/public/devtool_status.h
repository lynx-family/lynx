// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_STATUS_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_STATUS_H_

#include <mutex>
#include <string>

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
  static const char* NO_DEBUG_INFO_FOUND_BY_URL;

 public:
  static DevToolStatus& GetInstance();
  DevToolStatus(const DevToolStatus&) = delete;
  DevToolStatus& operator=(const DevToolStatus&) = delete;
  // Screenshot mode always has a value, defaulting to fullscreen.
  // default_value only applies to unset connection status or unknown keys.
  std::string GetStatus(const DevToolStatusKey& key,
                        const std::string& default_value = "");
  void SetStatus(const DevToolStatusKey& key, const std::string& value);

 private:
  DevToolStatus() = default;
  std::string connection_status_;
  std::string screenshot_mode_ = SCREENSHOT_MODE_FULLSCREEN;
  std::mutex mutex_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_STATUS_H_
