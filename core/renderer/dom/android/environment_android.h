// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_ANDROID_ENVIRONMENT_ANDROID_H_
#define CORE_RENDERER_DOM_ANDROID_ENVIRONMENT_ANDROID_H_

#include <jni.h>

#include <string>

namespace lynx {
namespace base {
namespace android {
class EnvironmentAndroid {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
  static std::string GetCacheDir();

 private:
  static std::string s_cache_dir_;
};

}  // namespace android
}  // namespace base
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_ANDROID_ENVIRONMENT_ANDROID_H_
