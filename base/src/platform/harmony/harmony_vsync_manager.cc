// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/harmony/harmony_vsync_manager.h"

#include <native_vsync/native_vsync.h>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"

// temporarily workaround to compile without logging
#undef LOGE
#undef DCHECK
#define LOGE(...)
#define DCHECK(...)

namespace lynx {
namespace base {

namespace {

const char* kLynxSyncName = "lynx_vsync_connect";

}  // namespace

HarmonyVsyncManager& HarmonyVsyncManager::GetInstance() {
  static HarmonyVsyncManager manager;
  return manager;
}

HarmonyVsyncManager::HarmonyVsyncManager() {
  vsync_handle_ = OH_NativeVSync_Create(kLynxSyncName, strlen(kLynxSyncName));
  DCHECK(vsync_handle_);
}

HarmonyVsyncManager::~HarmonyVsyncManager() {
  OH_NativeVSync_Destroy(vsync_handle_);
}

void HarmonyVsyncManager::RequestVSync(const VSyncCallback& callback) {
  bool requested = false;
  {
    std::lock_guard<std::mutex> l(mutex_);
    callbacks_.push_back(callback);
    requested = requested_;
    requested_ = true;
  }
  if (!requested) {
    OH_NativeVSync* handle = vsync_handle_;
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "HarmonyVsyncManager::RequestVSync");
    int32_t ret = 0;
    if (0 != (ret = OH_NativeVSync_RequestFrame(handle, &OnVsyncFromHarmony,
                                                this))) {
      // request vsync failed.
      LOGE("RequestVSync...failed:" << ret);
      std::lock_guard<std::mutex> l(mutex_);
      requested_ = false;
    }
  }
}

void HarmonyVsyncManager::OnVsyncFromHarmony(long long timestamp, void* data) {
  HarmonyVsyncManager* self = reinterpret_cast<HarmonyVsyncManager*>(data);
  std::vector<VSyncCallback> callbacks;
  // make callbacks to be thread-safed.
  {
    std::lock_guard<std::mutex> l(self->mutex_);
    self->requested_ = false;
    callbacks = self->callbacks_;
    self->callbacks_.clear();
  }
  for (auto& callback : callbacks) {
    callback(timestamp);
  }
}

}  // namespace base
}  // namespace lynx
