// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// MemoryPressure provides static APIs for handling memory pressure on
// platforms that have such signals, such as Android and ChromeOS.
// The app will try to discard buffers that aren't deemed essential (individual
// modules will implement their own policy).

#ifndef CLAY_MEMORY_MEMORY_PRESSURE_LISTENER_H_
#define CLAY_MEMORY_MEMORY_PRESSURE_LISTENER_H_

#include <functional>

#include "base/include/fml/task_runner.h"

namespace clay {

// To start listening, create a new instance, passing a callback to a
// function that takes a MemoryPressureLevel parameter. To stop listening,
// simply delete the listener object. The implementation guarantees
// that the callback will always be called on the thread that created
// the listener.
// Note that even on the same thread, the callback is not guaranteed to be
// called synchronously within the system memory pressure broadcast.
// Please see notes in MemoryPressureLevel enum below: some levels are
// absolutely critical, and if not enough memory is returned to the system,
// it'll potentially kill the app, and then later the app will have to be
// cold-started.
//
// Example:
//
//    void OnMemoryPressure(MemoryPressureLevel memory_pressure_level) {
//       ...
//    }
//
//    // Start listening.
//    auto listener = std::make_unique<MemoryPressureListener>(
//        base::BindRepeating(&OnMemoryPressure));
//
//    ...
//
//    // Stop listening.
//    listener.reset();
//
class MemoryPressureListener {
 public:
  // A Java counterpart is in
  // io/flutter/embedding/engine/memory/MemoryPressureLevel.java
  enum MemoryPressureLevel {
    // No problems, there is enough memory to use. This event is not sent via
    // callback.
    MEMORY_PRESSURE_LEVEL_NONE = 0,

    // Modules are advised to free buffers that are cheap to re-allocate and not
    // immediately needed.
    MEMORY_PRESSURE_LEVEL_MODERATE = 1,

    // At this level, modules are advised to free all possible memory.  The
    // alternative is to be killed by the system, which means all memory will
    // have to be re-created, plus the cost of a cold start.
    MEMORY_PRESSURE_LEVEL_CRITICAL = 2,

    // This must be the last value in the enum. The casing is different from the
    // other values to make this enum work well with the
    // UMA_HISTOGRAM_ENUMERATION macro.
    kMaxValue = MEMORY_PRESSURE_LEVEL_CRITICAL,
  };

  using MemoryPressureCallback = std::function<void(MemoryPressureLevel)>;
  using SyncMemoryPressureCallback = std::function<void(MemoryPressureLevel)>;

  MemoryPressureListener(const MemoryPressureCallback& memory_pressure_callback,
                         fml::RefPtr<fml::TaskRunner> task_runner);
  MemoryPressureListener(
      const MemoryPressureCallback& memory_pressure_callback,
      const SyncMemoryPressureCallback& sync_memory_pressure_callback,
      fml::RefPtr<fml::TaskRunner> task_runner);

  MemoryPressureListener(const MemoryPressureListener&) = delete;
  MemoryPressureListener& operator=(const MemoryPressureListener&) = delete;

  virtual ~MemoryPressureListener();

  // Intended for use by the platform specific implementation.
  static void NotifyMemoryPressure(MemoryPressureLevel memory_pressure_level);

  // Mark as virtual to be utilized in UT.
  virtual void Notify(MemoryPressureLevel memory_pressure_level);
  void SyncNotify(MemoryPressureLevel memory_pressure_level);

  // For unittests only.
  static bool TestIfListenerExistsAsync(MemoryPressureListener* listener);
  static bool TestIfListenerExistsSync(MemoryPressureListener* listener);

 private:
  static void DoNotifyMemoryPressure(MemoryPressureLevel memory_pressure_level);

  fml::RefPtr<fml::TaskRunner> task_runner_ = nullptr;

  MemoryPressureCallback callback_ = nullptr;
  SyncMemoryPressureCallback sync_memory_pressure_callback_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_MEMORY_MEMORY_PRESSURE_LISTENER_H_
