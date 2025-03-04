// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_LONG_TASK_TIMING_ANDROID_LONG_TASK_MONITOR_ANDROID_H_
#define CORE_SERVICES_LONG_TASK_TIMING_ANDROID_LONG_TASK_MONITOR_ANDROID_H_

#include <jni.h>

namespace lynx {
namespace tasm {
namespace timing {

void RegisterJniLongTaskMonitor(JNIEnv* env);

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_LONG_TASK_TIMING_ANDROID_LONG_TASK_MONITOR_ANDROID_H_
