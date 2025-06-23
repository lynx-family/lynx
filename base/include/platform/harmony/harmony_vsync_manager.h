// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PLATFORM_HARMONY_HARMONY_VSYNC_MANAGER_H_
#define BASE_INCLUDE_PLATFORM_HARMONY_HARMONY_VSYNC_MANAGER_H_

#include <functional>
#include <mutex>
#include <vector>

#include "base/include/fml/macros.h"

struct OH_NativeVSync;
namespace lynx {
namespace base {

// This class is used to request vsync signal from Harmony, and dispatch it to
// all vsync monitors.
class HarmonyVsyncManager {
 public:
  using VSyncCallback = std::function<void(long long)>;

  static HarmonyVsyncManager& GetInstance();

  // request the next vsync signal, then callback when it arrives.
  // OH_NativeVSync_Create takes very long time (≈1ms), may slow down
  // initization, so we use a global OH_NativeVSync here, and dispatch to each
  // vsync monitor.
  void RequestVSync(const VSyncCallback& callback);

 private:
  HarmonyVsyncManager();
  ~HarmonyVsyncManager();

  // it's the callback for a vsync request, dispatch all callbacks in the
  // callbacks vector, and make sure it's thread-safe.
  static void OnVsyncFromHarmony(long long timestamp, void* data);

  OH_NativeVSync* vsync_handle_ = nullptr;
  bool requested_ = false;
  std::mutex mutex_;
  std::vector<VSyncCallback> callbacks_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(HarmonyVsyncManager);
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_PLATFORM_HARMONY_HARMONY_VSYNC_MANAGER_H_
