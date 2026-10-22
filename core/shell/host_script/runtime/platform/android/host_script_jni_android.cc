// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include "core/shell/host_script/runtime/platform/android/host_script_runtime_callbacks.h"
#include "core/shell/host_script/runtime/platform/android/process_runtime_android.h"
#include "platform/android/lynx_host_script/src/main/jni/gen/LynxHostScriptRuntime_jni.h"
#include "platform/android/lynx_host_script/src/main/jni/gen/LynxHostScriptRuntime_register_jni.h"

namespace hsr = lynx::shell;

namespace lynx {
namespace jni {
bool RegisterJNIForLynxHostScriptRuntime(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

static jlong Install(JNIEnv* env, jclass, jboolean commit) {
  static const hsr::HostScriptRuntimeCallbacks callbacks{
      hsr::kHostScriptRuntimeAbiVersion, hsr::kHostScriptExecutorFeatures,
      hsr::PrepareStandaloneHostScriptRuntime,
      hsr::UpdateStandaloneHostScriptDebugState};
  const auto* transport = hsr::GetStandaloneHostScriptTransport();
  if (!hsr::PublishHostScriptRuntimeCallbacks(env, &callbacks, commit))
    return 0;
  return reinterpret_cast<jlong>(transport);
}
