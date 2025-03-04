// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_LOGGING_ANDROID_H_
#define CORE_BASE_ANDROID_LOGGING_ANDROID_H_

#include <android/log.h>
#include <jni.h>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
namespace logging {

BASE_EXPORT_FOR_DEVTOOL bool RegisterJNI(JNIEnv* env);

BASE_EXPORT_FOR_DEVTOOL long GetALogPtr(JNIEnv* env);

BASE_EXPORT_FOR_DEVTOOL void InitLynxLog();

}  // namespace logging
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_LOGGING_ANDROID_H_
