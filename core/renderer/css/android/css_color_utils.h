// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_ANDROID_CSS_COLOR_UTILS_H_
#define CORE_RENDERER_CSS_ANDROID_CSS_COLOR_UTILS_H_

#include <jni.h>

namespace lynx {
namespace tasm {
class CSSColorUtils {
 public:
  static bool RegisterJNI(JNIEnv* env);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_ANDROID_CSS_COLOR_UTILS_H_
