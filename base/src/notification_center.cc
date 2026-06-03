// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/notification_center.h"

#include <algorithm>
#include <mutex>
#include <utility>

#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"

namespace lynx {
namespace base {

namespace {
class NotificationCenter {
 public:
  void AddObserver(NotificationCallback* listener) {
    std::scoped_lock<std::mutex> lock(mutex_);
    observers_.push_back(listener);
  }

  void RemoveObserver(NotificationCallback* listener) {
    std::scoped_lock<std::mutex> lock(mutex_);
    auto it = std::find(observers_.begin(), observers_.end(), listener);
    if (it != observers_.end()) {
      observers_.erase(it);
    }
  }

  void Notify(const std::string& tag, intptr_t data) {
    std::scoped_lock<std::mutex> lock(mutex_);
    for (auto* listener : observers_) {
      listener->OnNotification(tag, data);
    }
  }

 private:
  std::mutex mutex_;
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
      TRACE_EVENT("NotificationCallback", tag,
                  [data](lynx::perfetto::EventContext ctx) {
                    ctx.event()->add_debug_annotations("data",
                                                       std::to_string(data));
                  });
      callback.second(tag, data);
      return;
    }
  }
}

}  // namespace base
}  // namespace lynx
