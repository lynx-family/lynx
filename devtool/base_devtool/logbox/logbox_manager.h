// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_MANAGER_H_
#define DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_MANAGER_H_
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/logbox/logbox_base.h"
#include "devtool/base_devtool/logbox/logbox_dialog_base.h"

namespace lynx {
namespace devtool {

// Forward declarations
class LogBoxNotification;

class LogBoxManager : public std::enable_shared_from_this<LogBoxManager> {
 public:
  LogBoxManager(void* window) : native_window_(window) {}
  void OnNewLog(LogBoxLogMsg&& msg, const std::shared_ptr<LogBoxProxy>& proxy);

  // Notification related methods
  void OnNotificationClick(int32_t level);
  void OnNotificationClose(int32_t level);
  void OnRemoveBtnClick(int32_t level);
  std::string ExtractBriefMessage(std::string message);
  void OnNativeWindowChange();

 private:
  static constexpr int32_t LEVEL_COUNT =
      static_cast<int32_t>(LogBoxLogLevel::Butt);
  std::shared_ptr<LogBoxDialogBase> dialog_;
  std::shared_ptr<LogBoxNotification> notification_;

  std::list<std::shared_ptr<LogBoxProxy>> proxys_;
  int32_t current_index_ = -1;
  std::weak_ptr<LogBoxProxy> current_proxy_[LEVEL_COUNT];
  int32_t log_counts_[LEVEL_COUNT] = {0};
  std::string last_messages_[LEVEL_COUNT];
  void* native_window_ = nullptr;

  void InitNotification(const std::shared_ptr<LogBoxProxy>& proxy);
  void RequestLogsOfCurrentView(int32_t level);
  int32_t GetIndexOfProxy(const std::shared_ptr<LogBoxProxy>& proxy);
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_MANAGER_H_
