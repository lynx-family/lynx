// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ANDROID_LYNX_TEMPLATE_BUNDLE_ANDROID_H_
#define CORE_RENDERER_DOM_ANDROID_LYNX_TEMPLATE_BUNDLE_ANDROID_H_

#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace tasm {
class LynxTemplateBundleAndroid {
 public:
  static bool RegisterJNI(JNIEnv* env);

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
  LynxTemplateBundleAndroid(const LynxTemplateBundleAndroid&) = delete;
  LynxTemplateBundleAndroid& operator=(const LynxTemplateBundleAndroid&) =
      delete;
};

lynx::base::android::ScopedLocalJavaRef<jobject>
ConstructJTemplateBundleFromNative(LynxTemplateBundle bundle);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ANDROID_LYNX_TEMPLATE_BUNDLE_ANDROID_H_
