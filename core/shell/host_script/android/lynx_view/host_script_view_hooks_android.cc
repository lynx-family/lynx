// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/android/lynx_view/host_script_view_hooks_android.h"

#include <iterator>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/value/base_string.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/base/android/android_jni.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/runtime/lepus/json_parser.h"
#include "core/shell/host_script/runtime/interceptor.h"
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
std::string GetHostScriptResourceRoot() {
  auto* env = base::android::AttachCurrentThread();
  auto root = Java_LynxHostScriptViewHooks_getHostScriptResourceRoot(env);
  return root.Get()
             ? base::android::JNIConvertHelper::ConvertToString(env, root.Get())
             : std::string{};
}

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

namespace {
using lynx::base::android::JNIConvertHelper;
using lynx::shell::InterceptKind;
using lynx::shell::Interceptor;

BASE_STATIC_STRING_DECL(kUpdateData, "updateData");
BASE_STATIC_STRING_DECL(kGlobalProps, "globalProps");
BASE_STATIC_STRING_DECL(kViewId, "viewId");
BASE_STATIC_STRING_DECL(kRequest, "request");
BASE_STATIC_STRING_DECL(kKind, "kind");
BASE_STATIC_STRING_DECL(kSize, "size");
BASE_STATIC_STRING_DECL(kUrl, "url");
BASE_STATIC_STRING_DECL(kSource, "source");
BASE_STATIC_STRING_DECL(kInitialData, "initialData");
BASE_STATIC_STRING_DECL(kBytes, "bytes");

// Source kinds of LynxHostScriptViewHooks.
constexpr const char* kSourceKinds[] = {"url", "bytes", "buffer", "bundle"};

lynx::base::android::ScopedLocalJavaRef<jbyteArray> ToJavaBytes(
    JNIEnv* env, const std::string& bytes) {
  return JNIConvertHelper::ConvertToJNIByteArray(env, bytes);
}

// Returns the patch of a UI view event, or nil when nothing changed.
lynx::lepus::Value DispatchView(InterceptKind kind, lynx::lepus::Value event) {
  auto provider = Interceptor::Current({kind}, Interceptor::Domain::kUI);
  if (!provider) return lynx::lepus::Value();
  auto result = provider->Dispatch(kind, event);
  return result.failed ? lynx::lepus::Value() : std::move(result.patch);
}
}  // namespace

static void DispatchCreated(JNIEnv* env, jclass, jlong view_id, jstring url) {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) return;
  lynx::shell::ProcessRuntime::GetInstance().NotifyViewCreated(
      {std::to_string(view_id),
       lynx::base::android::JNIConvertHelper::ConvertToString(env, url)});
}

static jbyteArray DispatchUpdateMetaData(JNIEnv* env, jclass, jlong view_id,
                                         jlong update_data,
                                         jlong global_props) {
  using lynx::lepus::Value;
  if (!Interceptor::IsEnabled()) return nullptr;
  Value request(lynx::lepus::Dictionary::Create());
  if (update_data)
    request.SetProperty(kUpdateData, *reinterpret_cast<Value*>(update_data));
  if (global_props)
    request.SetProperty(kGlobalProps, *reinterpret_cast<Value*>(global_props));
  Value event(lynx::lepus::Dictionary::Create());
  event.SetProperty(kViewId, Value(std::to_string(view_id)));
  event.SetProperty(kRequest, request);
  auto patch = DispatchView(InterceptKind::kUpdateMetaData, std::move(event));
  if (!patch.IsTable() || !patch.GetLength()) return nullptr;
  auto result = ToJavaBytes(env, lynx::lepus::lepusValueToJSONString(patch));
  // Transfer a separate local reference to Java before the scoped wrapper is
  // destroyed.
  return static_cast<jbyteArray>(
      env->NewLocalRef(result.Get()));  // NOLINT(lynx_custom/new_java_ref)
}

static jobjectArray DispatchLoadTemplate(JNIEnv* env, jclass, jlong view_id,
                                         jbyteArray url, jint source, jint size,
                                         jlong initial_data,
                                         jlong global_props) {
  using lynx::lepus::Value;
  if (!Interceptor::IsEnabled() || source < 0 ||
      source >= static_cast<jint>(std::size(kSourceKinds)))
    return nullptr;
  Value original_source(lynx::lepus::Dictionary::Create());
  original_source.SetProperty(kKind, Value(kSourceKinds[source]));
  if (size > 0) original_source.SetProperty(kSize, Value(size));
  Value request(lynx::lepus::Dictionary::Create());
  request.SetProperty(kUrl, Value(JNIConvertHelper::ConvertToString(env, url)));
  request.SetProperty(kSource, original_source);
  // Borrowed from TemplateData; event delivery copies them into JavaScript.
  if (initial_data)
    request.SetProperty(kInitialData, *reinterpret_cast<Value*>(initial_data));
  if (global_props)
    request.SetProperty(kGlobalProps, *reinterpret_cast<Value*>(global_props));
  Value event(lynx::lepus::Dictionary::Create());
  event.SetProperty(kViewId, Value(std::to_string(view_id)));
  event.SetProperty(kRequest, request);
  auto patch = DispatchView(InterceptKind::kLoadTemplate, std::move(event));
  if (!patch.IsTable()) return nullptr;

  // Report only a replaced source, and move its bytes out of the JSON.
  Value changes(lynx::lepus::Dictionary::Create());
  Value bytes;
  for (const auto& [key, value] : *patch.Table()) {
    if (key != kSource) {
      changes.SetProperty(key, value);
      continue;
    }
    if (value == original_source) continue;
    Value kind(lynx::lepus::Dictionary::Create());
    kind.SetProperty(kKind, value.GetProperty(kKind));
    changes.SetProperty(key, kind);
    bytes = value.GetProperty(kBytes);
  }
  if (!changes.GetLength()) return nullptr;
  // Local references are released when the native call returns.
  jobjectArray result = env->NewObjectArray(2, env->FindClass("[B"), nullptr);
  env->SetObjectArrayElement(
      result, 0,
      ToJavaBytes(env, lynx::lepus::lepusValueToJSONString(changes)).Get());
  if (bytes.IsByteArray()) {
    auto array = bytes.ByteArray();
    env->SetObjectArrayElement(
        result, 1,
        ToJavaBytes(env,
                    std::string(reinterpret_cast<const char*>(array->GetPtr()),
                                array->GetLength()))
            .Get());
  }
  return result;
}
