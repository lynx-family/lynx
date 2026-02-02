// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/memory/memory_pressure_callback.h"

#include <algorithm>
#include <list>
#include <mutex>
#include <utility>

#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"

namespace lynx {
namespace base {
namespace {
class MemoryPressureObserver {
 public:
  MemoryPressureObserver() = default;
  ~MemoryPressureObserver() = default;

  void AddObserver(MemoryPressureCallback* listener) {
    std::scoped_lock<std::mutex> lock(observers_mutex_);
    observers_.push_back(listener);
  }

  void RemoveObserver(MemoryPressureCallback* listener) {
    std::scoped_lock<std::mutex> lock(observers_mutex_);
    auto it = std::find(observers_.begin(), observers_.end(), listener);
    if (it != observers_.end()) {
      observers_.erase(it);
    }
  }

  void Notify(MemoryPressureLevel memory_pressure_level) {
    std::scoped_lock<std::mutex> lock(observers_mutex_);
    for (auto* listener : observers_) {
      listener->OnLowMemory(memory_pressure_level);
    }
  }

 private:
  std::mutex observers_mutex_;
  std::list<MemoryPressureCallback*> observers_;
};

MemoryPressureObserver& GetMemoryPressureObserver() {
  static NoDestructor<MemoryPressureObserver> observer{};
  return *observer;
}
}  // namespace

void MemoryPressureCallback::NotifyMemoryPressure(
    MemoryPressureLevel memory_pressure_level) {
  if (memory_pressure_level ==
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE) {
    return;
  }
  GetMemoryPressureObserver().Notify(memory_pressure_level);
}

MemoryPressureCallback::MemoryPressureCallback(Callback callback)
    : callback_(std::move(callback)) {
  GetMemoryPressureObserver().AddObserver(this);
}

MemoryPressureCallback::~MemoryPressureCallback() {
  GetMemoryPressureObserver().RemoveObserver(this);
}

void MemoryPressureCallback::OnLowMemory(
    MemoryPressureLevel memory_pressure_level) {
  TRACE_EVENT("MemoryPressureCallback", "OnLowMemory");
  callback_(memory_pressure_level);
}

}  // namespace base
}  // namespace lynx
