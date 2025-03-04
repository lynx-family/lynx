// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_VSYNC_OBSERVER_INTERFACE_H_
#define CORE_PUBLIC_VSYNC_OBSERVER_INTERFACE_H_

#include <stdio.h>

#include <functional>

#include "base/include/base_export.h"
#include "base/include/closure.h"

namespace lynx {
namespace runtime {

// C++ VsyncObserver interface, exported from lynx.so.
class BASE_EXPORT IVSyncObserver {
 public:
  IVSyncObserver() = default;
  virtual ~IVSyncObserver() = default;

  virtual void RequestAnimationFrame(
      uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) = 0;

  virtual void RequestBeforeAnimationFrame(
      uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) = 0;

  virtual void RegisterAfterAnimationFrameListener(
      base::MoveOnlyClosure<void, int64_t, int64_t> callback) = 0;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_PUBLIC_VSYNC_OBSERVER_INTERFACE_H_
