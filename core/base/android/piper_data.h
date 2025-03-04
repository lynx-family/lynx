// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_PIPER_DATA_H_
#define CORE_BASE_ANDROID_PIPER_DATA_H_
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/runtime/jsi/jsi.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace base {
namespace android {

class PiperData {
 public:
  static bool RegisterJni(JNIEnv* env);

  static std::optional<piper::Value> jsObjectFromPiperData(
      JNIEnv* env, piper::Runtime* rt,
      android::ScopedLocalJavaRef<jobject> json);
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_PIPER_DATA_H_
