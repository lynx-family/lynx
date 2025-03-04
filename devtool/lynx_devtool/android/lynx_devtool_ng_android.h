// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_ANDROID_LYNX_DEVTOOL_NG_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_ANDROID_LYNX_DEVTOOL_NG_ANDROID_H_

#include "devtool/lynx_devtool/lynx_devtool_ng.h"

namespace lynx {
namespace devtool {

class LynxDevToolNGAndroid : public LynxDevToolNG {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_LYNX_DEVTOOL_ANDROID_LYNX_DEVTOOL_NG_ANDROID_H_
