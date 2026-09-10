// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "base/include/string/string_utils.h"
#include "core/base/android/android_jni.h"
#include "core/shell/host_script/android/runtime/host_script_android_session.h"
#include "core/shell/host_script/lynx_view/lynx_view_ref_proxy.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptLynxViewBinding_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptLynxViewBinding_register_jni.h"

namespace {

using lynx::base::android::JNIConvertHelper;
using lynx::base::android::ScopedLocalJavaRef;
using lynx::base::android::ScopedWeakGlobalJavaRef;
using lynx::shell::HostScriptJavaString;
using lynx::shell::HostScriptSessionFromHandle;
using lynx::shell::LynxViewRefGlobalEventRequest;
using lynx::shell::LynxViewRefLoadTemplateRequest;
using lynx::shell::LynxViewRefProxy;
using lynx::shell::LynxViewRefReloadTemplateRequest;
using lynx::shell::LynxViewRefSsrRequest;
using lynx::shell::LynxViewRefUpdateMetaDataRequest;

class AndroidLynxViewRefProxy final : public LynxViewRefProxy {
 public:
  AndroidLynxViewRefProxy(JNIEnv* env, jobject binding, jobject view)
      : binding_(env, binding), view_(env, view) {}

  bool LoadTemplate(LynxViewRefLoadTemplateRequest request) override {
    return Dispatch([&](JNIEnv* env, jobject binding, jobject view) {
      ScopedLocalJavaRef<jbyteArray> bytes;
      if (request.has_template) {
        bytes = JNIConvertHelper::ConvertToJNIByteArray(
            env, request.template_data.data(),
            static_cast<int32_t>(request.template_data.size()));
      }
      auto url = HostScriptJavaString(env, request.url);
      auto initial_data = HostScriptJavaString(env, request.initial_data_json);
      auto global_props = HostScriptJavaString(env, request.global_props_json);
      auto processor = HostScriptJavaString(env, request.processor_name);
      return Java_LynxHostScriptLynxViewBinding_enqueueLoadTemplate(
          env, binding, view, bytes.Get(), url.Get(), initial_data.Get(),
          global_props.Get(), processor.Get(), request.read_only);
    });
  }

  bool LoadSSR(LynxViewRefSsrRequest request) override {
    return DispatchSsr(std::move(request), false);
  }

  bool HydrateSSR(LynxViewRefSsrRequest request) override {
    return DispatchSsr(std::move(request), true);
  }

  bool UpdateMetaData(LynxViewRefUpdateMetaDataRequest request) override {
    return Dispatch([&](JNIEnv* env, jobject binding, jobject view) {
      auto data = HostScriptJavaString(env, request.data_json);
      auto props = HostScriptJavaString(env, request.global_props_json);
      return Java_LynxHostScriptLynxViewBinding_enqueueUpdateMetaData(
          env, binding, view, request.has_data ? data.Get() : nullptr,
          request.has_global_props ? props.Get() : nullptr);
    });
  }

  bool SetGlobalProps(std::string global_props_json) override {
    return Dispatch([&](JNIEnv* env, jobject binding, jobject view) {
      auto props = HostScriptJavaString(env, global_props_json);
      return Java_LynxHostScriptLynxViewBinding_enqueueSetGlobalProps(
          env, binding, view, props.Get());
    });
  }

  bool ReloadTemplate(LynxViewRefReloadTemplateRequest request) override {
    return Dispatch([&](JNIEnv* env, jobject binding, jobject view) {
      auto data = HostScriptJavaString(env, request.data_json);
      auto global_props = HostScriptJavaString(env, request.global_props_json);
      return Java_LynxHostScriptLynxViewBinding_enqueueReloadTemplate(
          env, binding, view, data.Get(), global_props.Get());
    });
  }

  bool SendGlobalEvent(LynxViewRefGlobalEventRequest request) override {
    return Dispatch([&](JNIEnv* env, jobject binding, jobject view) {
      auto name = HostScriptJavaString(env, request.name);
      auto params = HostScriptJavaString(env, request.params_json);
      return Java_LynxHostScriptLynxViewBinding_enqueueSendGlobalEvent(
          env, binding, view, name.Get(), params.Get());
    });
  }

  void Invalidate() override { invalidated_.store(true); }

 private:
  template <typename Invoke>
  bool Dispatch(Invoke invoke) {
    JNIEnv* env = lynx::base::android::AttachCurrentThread();
    ScopedLocalJavaRef<jobject> binding(binding_);
    ScopedLocalJavaRef<jobject> view(view_);
    if (invalidated_.load() || binding.IsNull() || view.IsNull()) {
      return false;
    }
    return invoke(env, binding.Get(), view.Get());
  }

  bool DispatchSsr(LynxViewRefSsrRequest request, bool hydrate) {
    return Dispatch([&](JNIEnv* env, jobject binding, jobject view) {
      auto bytes = JNIConvertHelper::ConvertToJNIByteArray(
          env, request.data.data(), static_cast<int32_t>(request.data.size()));
      auto url = HostScriptJavaString(env, request.url);
      auto initial_data = HostScriptJavaString(env, request.initial_data_json);
      if (hydrate) {
        return Java_LynxHostScriptLynxViewBinding_enqueueHydrateSSR(
            env, binding, view, bytes.Get(), url.Get(), initial_data.Get());
      } else {
        return Java_LynxHostScriptLynxViewBinding_enqueueLoadSSR(
            env, binding, view, bytes.Get(), url.Get(), initial_data.Get());
      }
    });
  }

  ScopedWeakGlobalJavaRef<jobject> binding_;
  ScopedWeakGlobalJavaRef<jobject> view_;
  std::atomic<bool> invalidated_{false};
};

}  // namespace

namespace lynx {
namespace jni {
bool RegisterJNIForLynxHostScriptLynxViewBinding(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

static jboolean BindView(JNIEnv* env, jclass, jlong native_ptr, jobject binding,
                         jobject view) {
  if (!native_ptr || !binding || !view) {
    return false;
  }
  auto proxy = std::make_unique<AndroidLynxViewRefProxy>(env, binding, view);
  return HostScriptSessionFromHandle(native_ptr).BindView(std::move(proxy));
}

static void Invalidate(JNIEnv*, jclass, jlong native_ptr) {
  if (native_ptr) {
    HostScriptSessionFromHandle(native_ptr).InvalidateView();
  }
}

static void Notify(JNIEnv* env, jclass, jlong native_ptr, jstring event,
                   jint code, jstring message) {
  if (native_ptr) {
    HostScriptSessionFromHandle(native_ptr)
        .Notify(JNIConvertHelper::ConvertToString(env, event), code,
                message ? JNIConvertHelper::ConvertToString(env, message) : "");
  }
}
