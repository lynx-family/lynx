/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <jni.h>
#ifndef CORE_BASE_ANDROID_FRESCO_BLUR_FILTER_H_
#define CORE_BASE_ANDROID_FRESCO_BLUR_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif
void fresco_iterativeBoxBlur(JNIEnv* env, jclass clazz, jobject bitmap,
                             jint iterations, jint radius);
#ifdef __cplusplus
}
#endif

#endif  // CORE_BASE_ANDROID_FRESCO_BLUR_FILTER_H_
