// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/frame_child_runtime_android.h"

#include <jni.h>

#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "clay/fml/logging.h"
#include "clay/lynx_adaptor/frame_child_runtime.h"
#include "clay/ui/component/frame_child_data_android.h"

namespace lynx {
namespace tasm {
namespace {

bool CheckJavaCall(JNIEnv* env, const char* operation) {
  std::string exception;
  if (base::android::CheckException(env, exception)) {
    return true;
  }
  FML_LOG(ERROR) << "FrameChildRuntimeAndroid " << operation
                 << " failed: " << exception;
  return false;
}

jmethodID GetMethod(JNIEnv* env, jclass clazz, const char* name,
                    const char* signature) {
  jmethodID method = env->GetMethodID(clazz, name, signature);
  return CheckJavaCall(env, name) ? method : nullptr;
}

base::android::ScopedLocalJavaRef<jobject> ToJavaObject(
    JNIEnv* env, const clay::Value& value) {
  if (value.IsNone() || value.IsNull()) {
    return {};
  }
  if (value.IsString()) {
    auto string = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, value.GetString());
    return base::android::ScopedLocalJavaRef<jobject>(
        env, env->NewLocalRef(string.Get()));
  }
  if (value.IsArrayBuffer()) {
    const auto& bytes = value.GetArrayBuffer();
    base::android::ScopedLocalJavaRef<jbyteArray> result(
        env, env->NewByteArray(static_cast<jsize>(bytes.size())));
    if (!result.IsNull() && !bytes.empty()) {
      env->SetByteArrayRegion(
          result.Get(), 0, static_cast<jsize>(bytes.size()),
          reinterpret_cast<const jbyte*>(bytes.data()));
    }
    return base::android::ScopedLocalJavaRef<jobject>(
        env, env->NewLocalRef(result.Get()));
  }

  const char* class_name = nullptr;
  const char* value_of_signature = nullptr;
  jvalue argument{};
  if (value.IsBool()) {
    class_name = "java/lang/Boolean";
    value_of_signature = "(Z)Ljava/lang/Boolean;";
    argument.z = value.GetBool() ? JNI_TRUE : JNI_FALSE;
  } else if (value.IsInt()) {
    class_name = "java/lang/Integer";
    value_of_signature = "(I)Ljava/lang/Integer;";
    argument.i = value.GetInt();
  } else if (value.IsUint()) {
    class_name = "java/lang/Long";
    value_of_signature = "(J)Ljava/lang/Long;";
    argument.j = static_cast<jlong>(value.GetUint());
  } else if (value.IsFloat()) {
    class_name = "java/lang/Double";
    value_of_signature = "(D)Ljava/lang/Double;";
    argument.d = value.GetFloat();
  } else if (value.IsDouble()) {
    class_name = "java/lang/Double";
    value_of_signature = "(D)Ljava/lang/Double;";
    argument.d = value.GetDouble();
  }
  if (class_name) {
    base::android::ScopedLocalJavaRef<jclass> clazz(
        env, env->FindClass(class_name));
    if (clazz.IsNull()) {
      return {};
    }
    jmethodID value_of =
        env->GetStaticMethodID(clazz.Get(), "valueOf", value_of_signature);
    if (!value_of) {
      return {};
    }
    return base::android::ScopedLocalJavaRef<jobject>(
        env, env->CallStaticObjectMethodA(clazz.Get(), value_of, &argument));
  }

  if (value.IsArray()) {
    base::android::ScopedLocalJavaRef<jclass> clazz(
        env, env->FindClass("java/util/ArrayList"));
    if (clazz.IsNull()) {
      return {};
    }
    jmethodID constructor = env->GetMethodID(clazz.Get(), "<init>", "()V");
    jmethodID add = env->GetMethodID(clazz.Get(), "add", "(Ljava/lang/Object;)Z");
    if (!constructor || !add) {
      return {};
    }
    base::android::ScopedLocalJavaRef<jobject> result(
        env, env->NewObject(clazz.Get(), constructor));
    for (const auto& item : value.GetArray()) {
      auto java_item = ToJavaObject(env, item);
      env->CallBooleanMethod(result.Get(), add, java_item.Get());
    }
    return result;
  }

  if (value.IsMap()) {
    base::android::ScopedLocalJavaRef<jclass> clazz(
        env, env->FindClass("java/util/HashMap"));
    if (clazz.IsNull()) {
      return {};
    }
    jmethodID constructor = env->GetMethodID(clazz.Get(), "<init>", "()V");
    jmethodID put = env->GetMethodID(
        clazz.Get(), "put",
        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!constructor || !put) {
      return {};
    }
    base::android::ScopedLocalJavaRef<jobject> result(
        env, env->NewObject(clazz.Get(), constructor));
    for (const auto& [key, item] : value.GetMap()) {
      auto java_key =
          base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, key);
      auto java_item = ToJavaObject(env, item);
      base::android::ScopedLocalJavaRef<jobject> previous(
          env, env->CallObjectMethod(result.Get(), put, java_key.Get(),
                                     java_item.Get()));
    }
    return result;
  }

  return {};
}

class FrameChildRuntimeAndroid final : public FrameChildRuntime {
 public:
  FrameChildRuntimeAndroid(JNIEnv* env, jobject runtime)
      : runtime_(env, runtime) {
    base::android::ScopedLocalJavaRef<jclass> clazz(
        env, env->GetObjectClass(runtime));
    if (!CheckJavaCall(env, "GetObjectClass") || clazz.IsNull()) {
      return;
    }
    load_bundle_method_ = GetMethod(
        env, clazz.Get(), "loadBundle",
        "(Ljava/lang/String;JJLjava/lang/Object;JLjava/lang/Object;)Z");
    update_metadata_method_ =
        GetMethod(env, clazz.Get(), "updateMetaData",
                  "(JLjava/lang/Object;JLjava/lang/Object;)Z");
    update_viewport_method_ =
        GetMethod(env, clazz.Get(), "updateViewport", "(FIFIZ)V");
    destroy_method_ = GetMethod(env, clazz.Get(), "destroy", "()V");
  }

  ~FrameChildRuntimeAndroid() override {
    if (runtime_.IsNull() || !destroy_method_) {
      return;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    env->CallVoidMethod(runtime_.Get(), destroy_method_);
    CheckJavaCall(env, "destroy");
  }

  bool IsValid() const {
    return !runtime_.IsNull() && load_bundle_method_ && update_metadata_method_ &&
           update_viewport_method_ && destroy_method_;
  }

  bool LoadBundle(const std::string& url, const LynxTemplateBundle& bundle,
                  const std::shared_ptr<FrameChildData>& data,
                  const std::shared_ptr<FrameChildData>& global_props) override {
    if (!IsValid()) {
      return false;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    auto java_url =
        base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
    if (java_url.IsNull()) {
      return false;
    }
    auto java_data = data ? ToJavaObject(env, data->value())
                          : base::android::ScopedLocalJavaRef<jobject>();
    auto java_global_props =
        global_props
            ? ToJavaObject(env, global_props->value())
            : base::android::ScopedLocalJavaRef<jobject>();
    if (!CheckJavaCall(env, "convert frame data")) {
      return false;
    }
    const jboolean result = env->CallBooleanMethod(
        runtime_.Get(), load_bundle_method_, java_url.Get(),
        reinterpret_cast<jlong>(&bundle),
        data ? data->TakeNativeDataPointer() : 0, java_data.Get(),
        global_props ? global_props->TakeNativeDataPointer() : 0,
        java_global_props.Get());
    return CheckJavaCall(env, "loadBundle") && result == JNI_TRUE;
  }

  bool UpdateMetaData(
      const std::shared_ptr<FrameChildData>& data,
      const std::shared_ptr<FrameChildData>& global_props) override {
    if (!IsValid() || (!data && !global_props)) {
      return false;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    auto java_data = data ? ToJavaObject(env, data->value())
                          : base::android::ScopedLocalJavaRef<jobject>();
    auto java_global_props =
        global_props
            ? ToJavaObject(env, global_props->value())
            : base::android::ScopedLocalJavaRef<jobject>();
    if (!CheckJavaCall(env, "convert frame metadata")) {
      return false;
    }
    const jboolean result = env->CallBooleanMethod(
        runtime_.Get(), update_metadata_method_,
        data ? data->TakeNativeDataPointer() : 0, java_data.Get(),
        global_props ? global_props->TakeNativeDataPointer() : 0,
        java_global_props.Get());
    return CheckJavaCall(env, "updateMetaData") && result == JNI_TRUE;
  }

  void UpdateViewport(float width, int width_mode, float height,
                      int height_mode, bool need_layout) override {
    if (!IsValid()) {
      return;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    env->CallVoidMethod(runtime_.Get(), update_viewport_method_, width,
                        width_mode, height, height_mode,
                        need_layout ? JNI_TRUE : JNI_FALSE);
    CheckJavaCall(env, "updateViewport");
  }

 private:
  base::android::ScopedGlobalJavaRef<jobject> runtime_;
  jmethodID load_bundle_method_ = nullptr;
  jmethodID update_metadata_method_ = nullptr;
  jmethodID update_viewport_method_ = nullptr;
  jmethodID destroy_method_ = nullptr;
};

class FrameChildRuntimeFactoryAndroid final
    : public FrameChildRuntimeFactory {
 public:
  FrameChildRuntimeFactoryAndroid(JNIEnv* env, jobject factory)
      : factory_(env, factory) {
    base::android::ScopedLocalJavaRef<jclass> clazz(
        env, env->GetObjectClass(factory));
    if (!CheckJavaCall(env, "factory GetObjectClass") || clazz.IsNull()) {
      return;
    }
    create_runtime_method_ = GetMethod(
        env, clazz.Get(), "create",
        "(JFII)Lcom/lynx/tasm/FrameChildRuntimeAndroid;");

    base::android::ScopedLocalJavaRef<jclass> runtime_class(
        env, env->FindClass("com/lynx/tasm/FrameChildRuntimeAndroid"));
    if (!CheckJavaCall(env, "find FrameChildRuntimeAndroid") ||
        runtime_class.IsNull()) {
      return;
    }
    jmethodID release_native_data = env->GetStaticMethodID(
        runtime_class.Get(), "releaseNativeData", "(J)V");
    if (!CheckJavaCall(env, "releaseNativeData") || !release_native_data) {
      return;
    }
    auto runtime_class_ref =
        std::make_shared<base::android::ScopedGlobalJavaRef<jclass>>(
            env, runtime_class.Get());
    clay::FrameChildDataAndroid::SetNativeDataReleaser(
        [runtime_class_ref, release_native_data](int64_t pointer) {
          JNIEnv* attached_env = base::android::AttachCurrentThread();
          attached_env->CallStaticVoidMethod(runtime_class_ref->Get(),
                                             release_native_data, pointer);
          CheckJavaCall(attached_env, "release frame native data");
        });

  }

  std::unique_ptr<FrameChildRuntime> CreateRuntime(
      UIDelegateClay* ui_delegate,
      const FrameChildRuntimeOptions& options) override {
    if (factory_.IsNull() || !create_runtime_method_ || !ui_delegate) {
      return nullptr;
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    const jint multi_async_thread = options.enable_multi_async_thread
                                        ? (*options.enable_multi_async_thread ? 1
                                                                              : 0)
                                        : -1;
    base::android::ScopedLocalJavaRef<jobject> runtime(
        env, env->CallObjectMethod(
                 factory_.Get(), create_runtime_method_,
                 reinterpret_cast<jlong>(ui_delegate),
                 options.device_pixel_ratio, options.embedded_mode,
                 multi_async_thread));
    if (!CheckJavaCall(env, "create") || runtime.IsNull()) {
      return nullptr;
    }
    auto child_runtime =
        std::make_unique<FrameChildRuntimeAndroid>(env, runtime.Get());
    if (!child_runtime->IsValid()) {
      return nullptr;
    }
    return child_runtime;
  }

 private:
  base::android::ScopedGlobalJavaRef<jobject> factory_;
  jmethodID create_runtime_method_ = nullptr;
};

}  // namespace

std::shared_ptr<FrameChildRuntimeFactory>
CreateFrameChildRuntimeFactoryAndroid(JNIEnv* env, jobject factory) {
  if (!env || !factory) {
    return nullptr;
  }
  return std::make_shared<FrameChildRuntimeFactoryAndroid>(env, factory);
}

}  // namespace tasm
}  // namespace lynx
