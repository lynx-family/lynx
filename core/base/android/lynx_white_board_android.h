// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_LYNX_WHITE_BOARD_ANDROID_H_
#define CORE_BASE_ANDROID_LYNX_WHITE_BOARD_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace base {

class LynxWhiteBoardAndroid {
 public:
  static void RegisterJni(JNIEnv* env);
};
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_LYNX_WHITE_BOARD_ANDROID_H_
