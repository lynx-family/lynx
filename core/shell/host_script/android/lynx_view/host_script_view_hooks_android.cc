// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/android/lynx_view/host_script_view_hooks_android.h"

#include <string>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/android_jni.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/shell/host_script/runtime/process_runtime.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptViewHooks_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptViewHooks_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxHostScriptViewHooks(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
namespace shell {
void InstallHostScriptViewHooks() {
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled()) return;
  Java_LynxHostScriptViewHooks_setEnabled(base::android::AttachCurrentThread(),
                                          true);
}
void UninstallHostScriptViewHooks() {
  Java_LynxHostScriptViewHooks_setEnabled(base::android::AttachCurrentThread(),
                                          false);
}
}  // namespace shell
}  // namespace lynx

static void DispatchCreated(JNIEnv* env, jclass, jlong view_id, jstring url) {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) return;
  lynx::shell::ProcessRuntime::GetInstance().NotifyViewCreated(
      {std::to_string(view_id),
       lynx::base::android::JNIConvertHelper::ConvertToString(env, url)});
}
