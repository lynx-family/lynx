// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_EVENT_REPORT_ANDROID_EVENT_TRACKER_ANDROID_H_
#define CORE_SERVICES_EVENT_REPORT_ANDROID_EVENT_TRACKER_ANDROID_H_

#include <jni.h>

namespace lynx {
namespace tasm {
namespace report {

void RegisterJni(JNIEnv* env);

}  // namespace report
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_EVENT_REPORT_ANDROID_EVENT_TRACKER_ANDROID_H_
