// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/android/environment_android.h"

#include "core/build/gen/EnvUtils_jni.h"

namespace lynx {
namespace base {
namespace android {
std::string EnvironmentAndroid::s_cache_dir_;

bool EnvironmentAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

std::string EnvironmentAndroid::GetCacheDir() {
  if (s_cache_dir_.empty()) {
    JNIEnv* env = base::android::AttachCurrentThread();
    lynx::base::android::ScopedLocalJavaRef<jstring> dir =
        Java_EnvUtils_getCacheDir(env);
    const char* str = env->GetStringUTFChars(dir.Get(), JNI_FALSE);
    s_cache_dir_ = std::string(str);
    env->ReleaseStringUTFChars(dir.Get(), str);
  }
  return s_cache_dir_;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
