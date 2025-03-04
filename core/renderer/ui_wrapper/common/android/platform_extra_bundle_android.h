// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PLATFORM_EXTRA_BUNDLE_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PLATFORM_EXTRA_BUNDLE_ANDROID_H_

#include "core/base/android/jni_helper.h"
#include "core/public/platform_extra_bundle.h"

namespace lynx {
namespace tasm {

class PlatformBundleHolderAndroid : public PlatformExtraBundleHolder {
 public:
  PlatformBundleHolderAndroid(JNIEnv *env);
  ~PlatformBundleHolderAndroid() override;

  base::android::ScopedLocalJavaRef<jobject> QueryPlatformBundle(
      int32_t signature);

  void PutPlatformBundle(
      int32_t signature,
      const base::android::ScopedLocalJavaRef<jobject> &bundle);

  static bool RegisterJNI(JNIEnv *env);

 private:
  base::android::ScopedGlobalJavaRef<jobject> impl_ = {};
};

class PlatformExtraBundleAndroid : public PlatformExtraBundle {
 public:
  PlatformExtraBundleAndroid(
      int32_t signature, PlatformBundleHolderAndroid *holder,
      const base::android::ScopedLocalJavaRef<jobject> &bundle);
  ~PlatformExtraBundleAndroid() override = default;

  base::android::ScopedLocalJavaRef<jobject> GetPlatformBundle();
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PLATFORM_EXTRA_BUNDLE_ANDROID_H_
