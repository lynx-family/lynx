// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_ANDROID_LYNX_ENV_ANDROID_H_
#define CORE_RENDERER_UTILS_ANDROID_LYNX_ENV_ANDROID_H_

#include <jni.h>

#include <optional>
#include <string>

namespace lynx {
namespace tasm {

class LynxEnvAndroid {
 public:
  LynxEnvAndroid() = delete;
  ~LynxEnvAndroid() = delete;

  static void RegisterJNI(JNIEnv* env);
  static void onPiperInvoked(const std::string& module_name,
                             const std::string& method_name,
                             const std::string& param_str,
                             const std::string& url);
  static std::optional<std::string> GetStringFromExternalEnv(
      const std::string& key);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_ANDROID_LYNX_ENV_ANDROID_H_
