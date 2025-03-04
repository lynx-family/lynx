// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/lynx_devtool/recorder/android/recorder_controller_native_glue.h"

#include "core/base/android/jni_helper.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/recorder/testbench_base_recorder.h"
#include "devtool/lynx_devtool/build/gen/RecorderController_jni.h"

void StartRecord(JNIEnv* env, jclass jcaller) {
  lynx::tasm::recorder::RecorderController::StartRecord();
}

namespace lynx {
namespace devtool {
bool RecorderControllerNativeGlue::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace devtool
}  // namespace lynx
