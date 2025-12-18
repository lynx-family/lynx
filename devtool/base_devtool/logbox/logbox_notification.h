// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_NOTIFICATION_H_
#define DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_NOTIFICATION_H_

#include <cstdint>
#include <memory>
#include <string>

#include "devtool/base_devtool/logbox/logbox_base.h"

namespace lynx {
namespace devtool {

class LogBoxManager;

class LogBoxNotification {
 public:
  explicit LogBoxNotification(std::weak_ptr<LogBoxManager> manager);
  virtual ~LogBoxNotification();

  virtual void UpdateInfo(int32_t level, const std::string& msg,
                          int log_count) {}
  virtual void SetNativeWindow(void* native_window) {}
  virtual void Show() {}
  virtual void Hide() {}
  virtual void HideNotification(int32_t level) {}
  virtual void Destroy() {}
  virtual void OnNativeWindowChange() {}
  void HandleClick(int32_t level);
  void HandleClose(int32_t level);

  // Factory method to create platform-specific notification
  static std::shared_ptr<LogBoxNotification> Create(
      std::weak_ptr<LogBoxManager> manager);

  std::weak_ptr<LogBoxManager> manager_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_NOTIFICATION_H_
