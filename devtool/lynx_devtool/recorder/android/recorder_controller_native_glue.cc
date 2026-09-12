// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/jni_helper.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/recorder/recorder_types.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_register_jni.h"

void StartRecord(JNIEnv* env, jclass jcaller) {
  lynx::tasm::recorder::RecorderController::StartRecord();
}

void StartRecordWithFixtureArtifact(JNIEnv* env, jclass jcaller,
                                    jboolean fixture_artifact_enabled) {
  const auto format = fixture_artifact_enabled
                          ? lynx::tasm::recorder::ArtifactFormat::kBoth
                          : lynx::tasm::recorder::ArtifactFormat::kJson;
  lynx::tasm::recorder::RecorderController::StartRecord(format);
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
