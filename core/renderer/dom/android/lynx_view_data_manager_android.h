// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ANDROID_LYNX_VIEW_DATA_MANAGER_ANDROID_H_
#define CORE_RENDERER_DOM_ANDROID_LYNX_VIEW_DATA_MANAGER_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/renderer/data/lynx_view_data_manager.h"

namespace lynx {
namespace lepus {
class JsonData;
}

namespace tasm {
class LynxViewDataManagerAndroid : public LynxViewDataManager {
 public:
  static bool RegisterJNI(JNIEnv* env);
  static lepus::Value GetJsThreadDataFromTemplateData(JNIEnv* env,
                                                      jobject jni_object);
  static void ConsumeTemplateDataActions(JNIEnv* env, jobject jni_object);

  LynxViewDataManagerAndroid(JNIEnv* env, jobject jni_object);

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
  LynxViewDataManagerAndroid(const LynxViewDataManagerAndroid&) = delete;
  LynxViewDataManagerAndroid& operator=(const LynxViewDataManagerAndroid&) =
      delete;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ANDROID_LYNX_VIEW_DATA_MANAGER_ANDROID_H_
