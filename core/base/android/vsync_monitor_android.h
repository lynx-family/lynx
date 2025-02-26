// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_VSYNC_MONITOR_ANDROID_H_
#define CORE_BASE_ANDROID_VSYNC_MONITOR_ANDROID_H_

#include <jni.h>

#include "core/base/threading/vsync_monitor.h"

namespace lynx {
namespace shell {

class VSyncMonitorAndroid : public VSyncMonitor {
 public:
  VSyncMonitorAndroid();
  ~VSyncMonitorAndroid() override = default;
  static bool RegisterJNI(JNIEnv* env);

  void RequestVSyncOnUIThread(Callback callback);

 protected:
  void RequestVSync() override;

 private:
  bool is_vsync_triggered_in_ui_thread_{false};
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_VSYNC_MONITOR_ANDROID_H_
