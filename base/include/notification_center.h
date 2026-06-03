// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_NOTIFICATION_CENTER_H_
#define BASE_INCLUDE_NOTIFICATION_CENTER_H_

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>

#include "base/include/base_export.h"
#include "base/include/vector.h"

namespace lynx {
namespace base {

class BASE_EXPORT NotificationCallback {
 public:
  using Callback = std::function<void(const std::string& tag, intptr_t data)>;
  using CallbackList = std::initializer_list<std::pair<std::string, Callback>>;

  NotificationCallback(const std::string& tag, Callback callback);
  NotificationCallback(CallbackList callbacks);  // Multiple notifications.
  NotificationCallback(const NotificationCallback& other) = delete;
  NotificationCallback(NotificationCallback&& other) = delete;
  NotificationCallback& operator=(const NotificationCallback& other) = delete;
  NotificationCallback& operator=(NotificationCallback&& other) = delete;
  ~NotificationCallback();

  void OnNotification(const std::string& tag, intptr_t data);

  static void Notify(const std::string& tag, intptr_t data);

 private:
  InlineVector<std::pair<std::string, Callback>, 2> callbacks_;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_NOTIFICATION_CENTER_H_
