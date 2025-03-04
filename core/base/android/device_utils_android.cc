// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/device_utils_android.h"

#include "core/build/gen/DeviceUtils_jni.h"

namespace lynx {
namespace base {
namespace android {

void DeviceUtilsAndroid::RegisterJNI(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

bool DeviceUtilsAndroid::Is64BitDevice() {
  return Java_DeviceUtils_is64BitDevice(android::AttachCurrentThread());
}

}  // namespace android
}  // namespace base
}  // namespace lynx
