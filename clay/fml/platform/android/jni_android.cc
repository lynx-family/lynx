// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/platform/android/jni_android.h"

#include <string>

#include "clay/fml/logging.h"

namespace clay {
namespace android {

bool CheckException(JNIEnv *env) {
  std::string exception_msg;
  bool ret = lynx::base::android::CheckException(env, exception_msg);
  if (!exception_msg.empty()) {
    FML_LOG(ERROR) << exception_msg;
  }
  return ret;
}

}  // namespace android
}  // namespace clay
