// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_ANDROID_TEXT_UTILS_ANDROID_H_
#define CORE_RENDERER_UTILS_ANDROID_TEXT_UTILS_ANDROID_H_

#include <jni.h>

#include <memory>
#include <string>

#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {

class TextUtilsAndroidHelper {
 public:
  static void RegisterJNI(JNIEnv* env);
  static std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                                 const pub::Value& info);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_ANDROID_TEXT_UTILS_ANDROID_H_
