// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_BASE_ANDROID_LYNX_ERROR_ANDROID_H_
#define CORE_BASE_ANDROID_LYNX_ERROR_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace base {

struct LynxError;

namespace android {

class LynxErrorAndroid {
 public:
  explicit LynxErrorAndroid(const base::LynxError& error);
  ~LynxErrorAndroid() = default;

  jobject jni_object() { return jni_object_.Get(); }

 private:
  ScopedGlobalJavaRef<jobject> jni_object_;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_LYNX_ERROR_ANDROID_H_
