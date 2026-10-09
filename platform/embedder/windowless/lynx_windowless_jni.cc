// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/windowless/lynx_windowless_jni.h"

#include "base/include/platform/android/jni_utils.h"
#include "platform/android/lynx_android/src/main/jni/gen/DeviceUtils_register_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/ICURegister_register_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/JavaOnlyArray_register_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/JavaOnlyMap_register_jni.h"

namespace lynx {
namespace jni {
bool InitializeJNIForLynxEventReporter(JNIEnv* env);
}  // namespace jni

namespace embedder {

bool InitializeWindowlessAndroidJNI() {
  static const bool initialized = [] {
    auto* env = base::android::AttachCurrentThread();
    // These helpers only cache Java classes; they register no native methods.
    // Keep the Java callbacks for ordinary Lynx views owned by the Lynx
    // library.
    bool success = jni::RegisterJNIForJavaOnlyMap(env) &&
                   jni::RegisterJNIForJavaOnlyArray(env) &&
                   jni::RegisterJNIForDeviceUtils(env) &&
                   jni::InitializeJNIForLynxEventReporter(env);
#if ENABLE_NAPI_BINDING
    success = success && jni::RegisterJNIForICURegister(env);
#endif
    return success;
  }();
  return initialized;
}

}  // namespace embedder
}  // namespace lynx
