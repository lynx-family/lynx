// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_RECORDER_ANDROID_RECORDER_CONTROLLER_NATIVE_GLUE_H_
#define DEVTOOL_LYNX_DEVTOOL_RECORDER_ANDROID_RECORDER_CONTROLLER_NATIVE_GLUE_H_

#include <jni.h>

namespace lynx {
namespace devtool {
class RecorderControllerNativeGlue {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_RECORDER_ANDROID_RECORDER_CONTROLLER_NATIVE_GLUE_H_
