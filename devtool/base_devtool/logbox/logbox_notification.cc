// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/logbox/logbox_notification.h"

#include "devtool/base_devtool/logbox/logbox_manager.h"

namespace lynx {
namespace devtool {

LogBoxNotification::LogBoxNotification(std::weak_ptr<LogBoxManager> manager)
    : manager_(manager) {}

LogBoxNotification::~LogBoxNotification() { Hide(); }

void LogBoxNotification::HandleClick(int32_t level) {
  auto sp = manager_.lock();
  if (sp) {
    sp->OnNotificationClick(level);
  }
}

void LogBoxNotification::HandleClose(int32_t level) {
  auto sp = manager_.lock();
  if (sp) {
    sp->OnNotificationClose(level);
  }
}

__attribute__((weak)) std::shared_ptr<LogBoxNotification>
LogBoxNotification::Create(std::weak_ptr<LogBoxManager> manager) {
  return nullptr;
}
}  // namespace devtool
}  // namespace lynx
