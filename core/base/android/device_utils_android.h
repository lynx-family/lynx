// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_DEVICE_UTILS_ANDROID_H_
#define CORE_BASE_ANDROID_DEVICE_UTILS_ANDROID_H_

#include <jni.h>

namespace lynx {
namespace base {
namespace android {

class DeviceUtilsAndroid {
 public:
  DeviceUtilsAndroid() = delete;
  ~DeviceUtilsAndroid() = delete;

  static void RegisterJNI(JNIEnv* env);

  static bool Is64BitDevice();
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_DEVICE_UTILS_ANDROID_H_
