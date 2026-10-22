// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/platform/android/host_script_runtime_callbacks.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptRuntimeBridge_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptRuntimeBridge_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxHostScriptRuntimeBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

static jboolean SetRuntimeCallbacks(JNIEnv*, jclass, jlong callbacks,
                                    jboolean commit) {
  return lynx::shell::SetHostScriptRuntimeProvider(
      reinterpret_cast<const lynx::shell::HostScriptRuntimeCallbacks*>(
          callbacks),
      commit);
}
