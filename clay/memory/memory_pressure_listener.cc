// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/memory/memory_pressure_listener.h"

#include <list>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"

namespace clay {

namespace {
// This class is thread safe and internally synchronized.
class MemoryPressureObserver {
 public:
  // There is at most one MemoryPressureObserver and it is never deleted.
  ~MemoryPressureObserver() = delete;

  void AddObserver(MemoryPressureListener* listener, bool sync) {
    std::scoped_lock<std::mutex> lock(observers_mutex_);
    async_observers_.push_back(listener);
    if (sync) {
      sync_observers_.push_back(listener);
    }
  }

  void RemoveObserver(MemoryPressureListener* listener) {
    auto remove_func =
        [listener](std::list<MemoryPressureListener*>& observers) {
          auto it = std::find(observers.begin(), observers.end(), listener);
          if (it != observers.end()) {
            observers.erase(it);
          }
        };
    std::scoped_lock<std::mutex> lock(observers_mutex_);
    remove_func(async_observers_);
    remove_func(sync_observers_);
  }

  void Notify(
      MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
    std::scoped_lock<std::mutex> lock(observers_mutex_);
    for (auto* listener : async_observers_) {
      listener->Notify(memory_pressure_level);
    }
    for (auto* listener : sync_observers_) {
      listener->SyncNotify(memory_pressure_level);
    }
  }

 private:
  friend bool MemoryPressureListener::TestIfListenerExistsAsync(
      MemoryPressureListener*);
  friend bool MemoryPressureListener::TestIfListenerExistsSync(
      MemoryPressureListener*);
  std::mutex observers_mutex_;
  std::list<MemoryPressureListener*> async_observers_;
  std::list<MemoryPressureListener*> sync_observers_;
};

// Gets the shared MemoryPressureObserver singleton instance.
MemoryPressureObserver& GetMemoryPressureObserver() {
  static fml::NoDestructor<MemoryPressureObserver> observer{};
  return *observer;
}
}  // namespace

MemoryPressureListener::MemoryPressureListener(
    const MemoryPressureCallback& memory_pressure_callback,
    fml::RefPtr<fml::TaskRunner> task_runner)
    : callback_(memory_pressure_callback) {
  task_runner_ = task_runner;
  GetMemoryPressureObserver().AddObserver(this, false);
}

MemoryPressureListener::MemoryPressureListener(
    const MemoryPressureCallback& memory_pressure_callback,
    const SyncMemoryPressureCallback& sync_memory_pressure_callback,
    fml::RefPtr<fml::TaskRunner> task_runner)
    : callback_(memory_pressure_callback),
      sync_memory_pressure_callback_(sync_memory_pressure_callback) {
  task_runner_ = task_runner;
  GetMemoryPressureObserver().AddObserver(this, true);
}

// static
bool MemoryPressureListener::TestIfListenerExistsAsync(
    MemoryPressureListener* listener) {
  std::scoped_lock<std::mutex> lock(
      GetMemoryPressureObserver().observers_mutex_);
  auto it =
      std::find(GetMemoryPressureObserver().async_observers_.begin(),
                GetMemoryPressureObserver().async_observers_.end(), listener);
  return it != GetMemoryPressureObserver().async_observers_.end();
}

// static
bool MemoryPressureListener::TestIfListenerExistsSync(
    MemoryPressureListener* listener) {
  std::scoped_lock<std::mutex> lock(
      GetMemoryPressureObserver().observers_mutex_);
  auto it =
      std::find(GetMemoryPressureObserver().sync_observers_.begin(),
                GetMemoryPressureObserver().sync_observers_.end(), listener);
  return it != GetMemoryPressureObserver().sync_observers_.end();
}

// static
void MemoryPressureListener::NotifyMemoryPressure(
    MemoryPressureLevel memory_pressure_level) {
  if (memory_pressure_level ==
      MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE) {
    return;
  }
  TRACE_EVENT("flutter", __PRETTY_FUNCTION__);
  DoNotifyMemoryPressure(memory_pressure_level);
}

// static
void MemoryPressureListener::DoNotifyMemoryPressure(
    MemoryPressureLevel memory_pressure_level) {
  GetMemoryPressureObserver().Notify(memory_pressure_level);
}

void MemoryPressureListener::Notify(MemoryPressureLevel memory_pressure_level) {
  TRACE_EVENT("flutter", __PRETTY_FUNCTION__);
  FML_DCHECK(task_runner_);
  fml::TaskRunner::RunNowOrPostTask(
      task_runner_, [callback = callback_, memory_pressure_level]() {
        callback(memory_pressure_level);
      });
}

void MemoryPressureListener::SyncNotify(
    MemoryPressureLevel memory_pressure_level) {
  if (!sync_memory_pressure_callback_) {
    return;
  }
  TRACE_EVENT("flutter", __PRETTY_FUNCTION__);
  fml::AutoResetWaitableEvent latch;
  fml::TaskRunner::RunNowOrPostTask(task_runner_,
                                    [callback = sync_memory_pressure_callback_,
                                     memory_pressure_level, &latch]() {
                                      callback(memory_pressure_level);
                                      latch.Signal();
                                    });
  latch.Wait();
}

MemoryPressureListener::~MemoryPressureListener() {
  GetMemoryPressureObserver().RemoveObserver(this);
}

}  // namespace clay
