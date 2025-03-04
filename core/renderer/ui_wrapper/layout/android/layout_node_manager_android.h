// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_NODE_MANAGER_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_NODE_MANAGER_ANDROID_H_

#include <jni.h>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/public/layout_node_value.h"

namespace lynx {
namespace tasm {

class LayoutNodeManagerAndroid {
 public:
  static bool RegisterJNI(JNIEnv* env);
};

class MeasureFuncAndroid : public MeasureFunc {
 public:
  MeasureFuncAndroid(JNIEnv* env, jobject obj);
  ~MeasureFuncAndroid() override = default;
  LayoutResult Measure(float width, int32_t width_mode, float height,
                       int32_t height_mode, bool final_measure) override;
  void Alignment() override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_LAYOUT_NODE_MANAGER_ANDROID_H_
