// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_ANDROID_JNI_ANDROID_H_
#define CLAY_FML_PLATFORM_ANDROID_JNI_ANDROID_H_

#include <jni.h>

#include <cstdint>

#include "base/include/platform/android/jni_utils.h"

namespace clay {
namespace android {

bool CheckException(JNIEnv* env);

}  // namespace android
}  // namespace clay

#endif  // CLAY_FML_PLATFORM_ANDROID_JNI_ANDROID_H_
