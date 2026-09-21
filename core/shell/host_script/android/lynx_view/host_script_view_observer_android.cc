// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/android/lynx_view/host_script_view_observer_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/android_jni.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/shell/host_script/runtime/process_runtime.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptViewObserver_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptViewObserver_register_jni.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxHostScriptViewObserver(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
namespace shell {
void InstallHostScriptViewObserver() {
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled()) return;
  Java_LynxHostScriptViewObserver_install(base::android::AttachCurrentThread());
}
void UninstallHostScriptViewObserver() {
  Java_LynxHostScriptViewObserver_uninstall(
      base::android::AttachCurrentThread());
}
}  // namespace shell
}  // namespace lynx

static void NotifyCreated(JNIEnv* env, jclass, jstring view_id, jstring url) {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) return;
  using lynx::base::android::JNIConvertHelper;
  rapidjson::StringBuffer payload;
  rapidjson::Writer<rapidjson::StringBuffer> writer(payload);
  auto write_string = [env, &writer](jstring value) {
    auto text = JNIConvertHelper::ConvertToString(env, value);
    writer.String(text.data(), static_cast<rapidjson::SizeType>(text.size()));
  };
  writer.StartObject();
  writer.Key("viewId");
  write_string(view_id);
  writer.Key("url");
  write_string(url);
  writer.EndObject();
  lynx::shell::ProcessRuntime::GetInstance().NotifyEvent(
      {"lynxViewCreated", payload.GetString()});
}
