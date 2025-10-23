// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_JNI_REGISTER_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_JNI_REGISTER_H_

#include <jni.h>

namespace clay {

bool RegisterSharedImageJNI(JNIEnv *env);

}

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_JNI_REGISTER_H_
