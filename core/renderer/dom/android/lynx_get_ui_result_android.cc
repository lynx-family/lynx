// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/android/lynx_get_ui_result_android.h"

#include "core/base/android/java_only_array.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/LynxGetUIResult_jni.h"
#include "core/renderer/dom/lynx_get_ui_result.h"

namespace lynx {
namespace tasm {

bool LynxGetUIResultAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

LynxGetUIResultAndroid::LynxGetUIResultAndroid(const LynxGetUIResult& result) {
  base::android::JavaOnlyArray array;
  for (int ui : result.UiImplIds()) {
    array.PushInt(ui);
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> err_msg =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env,
                                                             result.ErrMsg());
  jni_object_ = Java_LynxGetUIResult_create(env, array.jni_object(),
                                            result.ErrCode(), err_msg.Get());
}

}  // namespace tasm
}  // namespace lynx
