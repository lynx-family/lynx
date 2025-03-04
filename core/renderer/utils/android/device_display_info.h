// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_ANDROID_DEVICE_DISPLAY_INFO_H_
#define CORE_RENDERER_UTILS_ANDROID_DEVICE_DISPLAY_INFO_H_

#include <jni.h>

namespace lynx {
namespace tasm {
namespace android {
class DeviceDisplayInfo {
 public:
  static bool RegisterJNI(JNIEnv* env);
};
}  // namespace android
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_ANDROID_DEVICE_DISPLAY_INFO_H_
