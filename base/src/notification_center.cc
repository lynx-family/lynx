// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/notification_center.h"

#include <algorithm>
#include <mutex>
#include <utility>

#include "base/include/no_destructor.h"

namespace lynx {
namespace base {

namespace {
class NotificationCenter {
 public:
  void AddObserver(NotificationCallback* listener) {
    std::scoped_lock<std::recursive_mutex> lock(mutex_);
    observers_.push_back(listener);
  }

  void RemoveObserver(NotificationCallback* listener) {
    std::scoped_lock<std::recursive_mutex> lock(mutex_);
    auto it = std::find(observers_.begin(), observers_.end(), listener);
    if (it != observers_.end()) {
      observers_.erase(it);
    }
  }

  void Notify(const std::string& tag, intptr_t data) {
    std::scoped_lock<std::recursive_mutex> lock(mutex_);
    auto copied_observers = observers_;
    for (auto* listener : copied_observers) {
      // listener may have been removed/destroyed reentrantly during a previous
      // callback (recursive_mutex allows RemoveObserver in the same thread).
      if (std::find(observers_.begin(), observers_.end(), listener) ==
          observers_.end()) {
        continue;
      }
      listener->OnNotification(tag, data);
    }
  }

 private:
  std::recursive_mutex mutex_;
  Vector<NotificationCallback*> observers_;
};

NotificationCenter& GetNotificationCenter() {
  static NoDestructor<NotificationCenter> observer{};
  return *observer;
}
}  // namespace

void NotificationCallback::Notify(const std::string& tag, intptr_t data) {
  GetNotificationCenter().Notify(tag, data);
}

NotificationCallback::NotificationCallback(const std::string& tag,
                                           Callback callback) {
  callbacks_.emplace_back(tag, std::move(callback));
  GetNotificationCenter().AddObserver(this);
}

NotificationCallback::NotificationCallback(CallbackList callbacks) {
  for (const auto& callback : callbacks) {
    callbacks_.emplace_back(callback.first, callback.second);
  }
  GetNotificationCenter().AddObserver(this);
}

NotificationCallback::~NotificationCallback() {
  GetNotificationCenter().RemoveObserver(this);
}

void NotificationCallback::OnNotification(const std::string& tag,
                                          intptr_t data) {
  for (const auto& callback : callbacks_) {
    if (callback.first == tag) {
      callback.second(tag, data);
      return;
    }
  }
}

}  // namespace base
}  // namespace lynx
