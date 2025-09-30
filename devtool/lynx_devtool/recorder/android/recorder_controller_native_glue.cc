// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/jni_helper.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/recorder/testbench_base_recorder.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_register_jni.h"

void StartRecord(JNIEnv* env, jclass jcaller) {
  lynx::tasm::recorder::RecorderController::StartRecord();
}

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForRecorderController(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace devtool
}  // namespace lynx
