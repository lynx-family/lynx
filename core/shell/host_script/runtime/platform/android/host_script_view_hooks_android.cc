// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/platform/android/host_script_view_hooks_android.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/base/android/android_jni.h"
#include "core/runtime/lepus/json_parser.h"
#include "core/shell/host_script/lynx_view/lynx_view_registry.h"
#include "core/shell/host_script/runtime/interceptor.h"
#include "core/shell/host_script/runtime/platform/android/process_runtime_android.h"
#include "platform/android/lynx_host_script/src/main/jni/gen/LynxHostScriptViewHooks_jni.h"
#include "platform/android/lynx_host_script/src/main/jni/gen/LynxHostScriptViewHooks_register_jni.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

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

void EnableHostScriptViewRegistry(fml::RefPtr<fml::TaskRunner> ui_runner) {
  LynxViewRegistry::GetInstance().SetUIRunner(std::move(ui_runner));
  Java_LynxHostScriptViewHooks_setRegistryAvailable(
      base::android::AttachCurrentThread());
}

void InstallHostScriptViewHooks() {
  if (!lynx::shell::IsHostScriptDebugEnabled()) return;
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
using lynx::base::android::ScopedLocalJavaRef;
using lynx::base::android::ScopedWeakGlobalJavaRef;
using lynx::shell::InterceptKind;
using lynx::shell::Interceptor;
using lynx::shell::LynxViewRegistry;

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
BASE_STATIC_STRING_DECL(kOrigin, "origin");
BASE_STATIC_STRING_DECL(kWidth, "width");
BASE_STATIC_STRING_DECL(kHeight, "height");

// Source kinds of LynxHostScriptViewHooks.
constexpr const char* kSourceKinds[] = {"url", "bytes", "buffer", "bundle"};

lynx::base::android::ScopedLocalJavaRef<jbyteArray> ToJavaBytes(
    JNIEnv* env, const std::string& bytes) {
  return JNIConvertHelper::ConvertToJNIByteArray(env, bytes);
}

// Converts through UTF-16: scripts may send text Modified UTF-8 cannot carry.
ScopedLocalJavaRef<jstring> ToJavaString(JNIEnv* env,
                                         const std::string& value) {
  auto utf16 = lynx::base::U8StringToU16(value);
  return JNIConvertHelper::ConvertToJNIString(
      env, reinterpret_cast<const jchar*>(utf16.data()),
      static_cast<jsize>(utf16.size()));
}

lynx::lepus::Value Origin(jboolean from_script) {
  return lynx::lepus::Value(from_script ? "host-script" : "host");
}

// Runs registry commands on the UI thread against a weakly referenced View.
// Each command executes synchronously; the registry already posted it.
class AndroidLynxView final : public lynx::shell::LynxViewRefProxy {
 public:
  AndroidLynxView(JNIEnv* env, jobject view, jlong view_id)
      : view_(env, view), view_id_(view_id) {}

  bool LoadTemplate(
      lynx::shell::LynxViewRefLoadTemplateRequest request) override {
    return Run([&](JNIEnv* env, jobject view) {
      ScopedLocalJavaRef<jbyteArray> bytes;
      if (request.has_template) {
        bytes = JNIConvertHelper::ConvertToJNIByteArray(
            env, request.template_data.data(),
            static_cast<int32_t>(request.template_data.size()));
      }
      Java_LynxHostScriptViewHooks_loadTemplate(
          env, view, view_id_, bytes.Get(),
          ToJavaString(env, request.url).Get(),
          Optional(env, request.initial_data_json).Get(),
          Optional(env, request.global_props_json).Get(),
          Optional(env, request.processor_name).Get(), request.read_only);
    });
  }

  bool LoadSSR(lynx::shell::LynxViewRefSsrRequest request) override {
    return Ssr(std::move(request), false);
  }

  bool HydrateSSR(lynx::shell::LynxViewRefSsrRequest request) override {
    return Ssr(std::move(request), true);
  }

  bool UpdateMetaData(
      lynx::shell::LynxViewRefUpdateMetaDataRequest request) override {
    return Run([&](JNIEnv* env, jobject view) {
      ScopedLocalJavaRef<jstring> data;
      ScopedLocalJavaRef<jstring> props;
      if (request.has_data) data = ToJavaString(env, request.data_json);
      if (request.has_global_props)
        props = ToJavaString(env, request.global_props_json);
      Java_LynxHostScriptViewHooks_updateMetaData(env, view, view_id_,
                                                  data.Get(), props.Get());
    });
  }

  bool ReloadTemplate(
      lynx::shell::LynxViewRefReloadTemplateRequest request) override {
    return Run([&](JNIEnv* env, jobject view) {
      Java_LynxHostScriptViewHooks_reloadTemplate(
          env, view, view_id_, ToJavaString(env, request.data_json).Get(),
          Optional(env, request.global_props_json).Get());
    });
  }

  bool SendGlobalEvent(
      lynx::shell::LynxViewRefGlobalEventRequest request) override {
    return Run([&](JNIEnv* env, jobject view) {
      Java_LynxHostScriptViewHooks_sendGlobalEvent(
          env, view, view_id_, ToJavaString(env, request.name).Get(),
          ToJavaString(env, request.params_json).Get());
    });
  }

  void Invalidate() override { invalidated_.store(true); }

 private:
  static ScopedLocalJavaRef<jstring> Optional(JNIEnv* env,
                                              const std::string& value) {
    return value.empty() ? ScopedLocalJavaRef<jstring>()
                         : ToJavaString(env, value);
  }

  template <typename Invoke>
  bool Run(Invoke invoke) {
    JNIEnv* env = lynx::base::android::AttachCurrentThread();
    ScopedLocalJavaRef<jobject> view(view_);
    if (invalidated_.load() || view.IsNull()) return false;
    invoke(env, view.Get());
    return true;
  }

  bool Ssr(lynx::shell::LynxViewRefSsrRequest request, bool hydrate) {
    return Run([&](JNIEnv* env, jobject view) {
      auto bytes = JNIConvertHelper::ConvertToJNIByteArray(
          env, request.data.data(), static_cast<int32_t>(request.data.size()));
      Java_LynxHostScriptViewHooks_loadSSR(
          env, view, view_id_, bytes.Get(),
          ToJavaString(env, request.url).Get(),
          Optional(env, request.initial_data_json).Get(), hydrate);
    });
  }

  ScopedWeakGlobalJavaRef<jobject> view_;
  const jlong view_id_;
  std::atomic<bool> invalidated_{false};
};

// Returns the patch of a UI view event, or nil when nothing changed.
lynx::lepus::Value DispatchView(InterceptKind kind, lynx::lepus::Value event) {
  auto provider = Interceptor::Current({kind}, Interceptor::Domain::kUI);
  if (!provider) return lynx::lepus::Value();
  auto result = provider->Dispatch(kind, event);
  return result.failed ? lynx::lepus::Value() : std::move(result.patch);
}

void WriteJson(rapidjson::Writer<rapidjson::StringBuffer>& writer,
               const lynx::lepus::Value& value) {
  if (value.IsNil() || value.IsUndefined()) {
    writer.Null();
  } else if (value.IsBool()) {
    writer.Bool(value.Bool());
  } else if (value.IsInt64()) {
    writer.Int64(value.Int64());
  } else if (value.IsUInt64()) {
    writer.Uint64(value.UInt64());
  } else if (value.IsInt32()) {
    writer.Int(value.Int32());
  } else if (value.IsUInt32()) {
    writer.Uint(value.UInt32());
  } else if (value.IsNumber()) {
    double number = value.Number();
    if (std::isfinite(number) && std::floor(number) == number &&
        number >= -9007199254740991.0 && number <= 9007199254740991.0 &&
        !(number == 0.0 && std::signbit(number))) {
      writer.Int64(static_cast<int64_t>(number));
    } else if (std::isfinite(number)) {
      writer.Double(number);
    } else {
      writer.Null();
    }
  } else if (value.IsString()) {
    auto text = value.StdString();
    writer.String(text.data(), static_cast<rapidjson::SizeType>(text.size()));
  } else if (value.IsArray()) {
    writer.StartArray();
    for (size_t i = 0; i < value.Array()->size(); ++i) {
      WriteJson(writer, value.Array()->get(i));
    }
    writer.EndArray();
  } else if (value.IsTable()) {
    writer.StartObject();
    for (const auto& [key, item] : *value.Table()) {
      writer.Key(key.c_str(), static_cast<rapidjson::SizeType>(key.length()));
      WriteJson(writer, item);
    }
    writer.EndObject();
  } else {
    writer.Null();
  }
}
jlong PackMeasureSpecs(jint width, jint height) {
  return static_cast<jlong>(
      (static_cast<uint64_t>(static_cast<uint32_t>(width)) << 32) |
      static_cast<uint64_t>(static_cast<uint32_t>(height)));
}
}  // namespace

static void RegisterView(JNIEnv* env, jclass, jlong view_id, jobject view,
                         jstring url) {
  if (!view) return;
  LynxViewRegistry::GetInstance().Register(
      static_cast<lynx::base::LynxEntityId>(view_id),
      std::make_shared<AndroidLynxView>(env, view, view_id),
      JNIConvertHelper::ConvertToString(env, url));
}

static void UnregisterView(JNIEnv*, jclass, jlong view_id) {
  LynxViewRegistry::GetInstance().Unregister(
      static_cast<lynx::base::LynxEntityId>(view_id));
}

static void ClearViewUrl(JNIEnv*, jclass, jlong view_id) {
  Interceptor::ClearViewUrl(static_cast<lynx::base::LynxEntityId>(view_id));
}

static void UpdateUrl(JNIEnv* env, jclass, jlong view_id, jstring url) {
  auto view = static_cast<lynx::base::LynxEntityId>(view_id);
  auto value = JNIConvertHelper::ConvertToString(env, url);
  Interceptor::UpdateViewUrl(view, value);
  LynxViewRegistry::GetInstance().UpdateUrl(view, std::move(value));
}

static void NotifyView(JNIEnv* env, jclass, jlong view_id, jstring event,
                       jint code, jstring message) {
  LynxViewRegistry::GetInstance().NotifyLifecycle(
      static_cast<lynx::base::LynxEntityId>(view_id),
      JNIConvertHelper::ConvertToString(env, event), code,
      message ? JNIConvertHelper::ConvertToString(env, message) : "");
}

// Returns a nonempty patch as JSON bytes, or null.
static jbyteArray PatchBytes(JNIEnv* env, const lynx::lepus::Value& patch) {
  if (!patch.IsTable() || !patch.GetLength()) return nullptr;
  // Keep MeasureSpec bits and floating-point configuration intact.
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  WriteJson(writer, patch);
  auto result =
      ToJavaBytes(env, std::string(buffer.GetString(), buffer.GetSize()));
  // Transfer a separate local reference to Java before the scoped wrapper is
  // destroyed.
  return static_cast<jbyteArray>(
      env->NewLocalRef(result.Get()));  // NOLINT(lynx_custom/new_java_ref)
}

static jbyteArray DispatchUpdateMetaData(JNIEnv* env, jclass, jlong view_id,
                                         jlong update_data, jlong global_props,
                                         jboolean from_script) {
  using lynx::lepus::Value;
  if (!Interceptor::IsEnabled()) return nullptr;
  Value request(lynx::lepus::Dictionary::Create());
  if (update_data)
    request.SetProperty(kUpdateData, *reinterpret_cast<Value*>(update_data));
  if (global_props)
    request.SetProperty(kGlobalProps, *reinterpret_cast<Value*>(global_props));
  Value event(lynx::lepus::Dictionary::Create());
  event.SetProperty(kViewId, Value(std::to_string(view_id)));
  event.SetProperty(kOrigin, Origin(from_script));
  event.SetProperty(kRequest, request);
  return PatchBytes(
      env, DispatchView(InterceptKind::kUpdateMetaData, std::move(event)));
}

static jboolean HasCreateHandler(JNIEnv*, jclass) {
  return Interceptor::Current({InterceptKind::kCreate},
                              Interceptor::Domain::kUI) != nullptr;
}

// The request is the JSON of the effective builder configuration. A View that
// is not yet constructed has no identity, proxy or page URL.
static jbyteArray DispatchCreate(JNIEnv* env, jclass, jbyteArray request_json) {
  using lynx::lepus::Value;
  if (!Interceptor::IsEnabled()) return nullptr;
  auto json = JNIConvertHelper::ConvertToString(env, request_json);
  auto request = lynx::lepus::jsonValueTolepusValue(json.c_str());
  if (!request.IsTable()) return nullptr;
  Value event(lynx::lepus::Dictionary::Create());
  event.SetProperty(kRequest, request);
  return PatchBytes(env,
                    DispatchView(InterceptKind::kCreate, std::move(event)));
}

static jlong DispatchUpdateViewport(JNIEnv*, jclass, jlong view_id, jint width,
                                    jint height) {
  using lynx::lepus::Value;
  if (!Interceptor::IsEnabled() ||
      !Interceptor::Current({InterceptKind::kUpdateViewport},
                            Interceptor::Domain::kUI)) {
    return PackMeasureSpecs(width, height);
  }
  Value request(lynx::lepus::Dictionary::Create());
  request.SetProperty(kWidth, Value(width));
  request.SetProperty(kHeight, Value(height));
  Value event(lynx::lepus::Dictionary::Create());
  event.SetProperty(kViewId, Value(std::to_string(view_id)));
  event.SetProperty(kRequest, request);
  auto patch = DispatchView(InterceptKind::kUpdateViewport, std::move(event));
  if (!patch.IsTable()) return PackMeasureSpecs(width, height);
  auto next_width = patch.GetProperty(kWidth);
  auto next_height = patch.GetProperty(kHeight);
  return PackMeasureSpecs(
      next_width.IsNumber() ? static_cast<jint>(next_width.Number()) : width,
      next_height.IsNumber() ? static_cast<jint>(next_height.Number())
                             : height);
}

static jobjectArray DispatchLoadTemplate(JNIEnv* env, jclass, jlong view_id,
                                         jbyteArray url, jint source, jint size,
                                         jlong initial_data, jlong global_props,
                                         jboolean from_script) {
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
  event.SetProperty(kOrigin, Origin(from_script));
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
      ToJavaBytes(env, lynx::lepus::lepusValueToString(changes)).Get());
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
