// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/android/module_factory_android.h"

#include <unordered_map>
#include <utility>

#include "core/base/android/jni_helper.h"
#include "core/value_wrapper/android/value_impl_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxModuleFactory_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxModuleFactory_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxModuleFactory(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

jboolean RetainJniObject(JNIEnv* env, jobject jcaller, jlong nativePtr) {
  lynx::runtime::js::ModuleFactoryAndroid* module_manager =
      reinterpret_cast<lynx::runtime::js::ModuleFactoryAndroid*>(nativePtr);
  if (!module_manager) {
    return JNI_FALSE;
  }
  return static_cast<jboolean>(module_manager->RetainJniObject());
}

namespace lynx {
namespace runtime {
namespace js {
ModuleFactoryAndroid::ModuleFactoryAndroid(JNIEnv* env, jobject moduleFactory)
    : jni_object_(env, moduleFactory) {
  Java_LynxModuleFactory_setNativePtr(env, moduleFactory,
                                      reinterpret_cast<jlong>(this));
}

ModuleFactoryAndroid::~ModuleFactoryAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (strong_jni_object_.IsNull()) {
    LOGE(
        "NativeModule: lynx module_factory_android destroy error: "
        "strong_jni_object_ is "
        "empty.");
    return;
  }
  LOGI("NativeModule: lynx module_factory_android destroy.");
  Java_LynxModuleFactory_destroy(env, strong_jni_object_.Get());
  strong_jni_object_.Reset(env, nullptr);
}

bool ModuleFactoryAndroid::RetainJniObject() {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (env->IsSameObject(jni_object_.Get(), nullptr)) {
    return false;
  }
  strong_jni_object_.Reset(env, jni_object_.Get());
  return !strong_jni_object_.IsNull();
}

std::shared_ptr<LynxNativeModule> ModuleFactoryAndroid::CreateModule(
    const std::string& name) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(jni_object_);
  if (local_ref.IsNull()) {
    LOGW("NativeModule: LynxModule, "
         << "ModuleFactoryAndroid::ModuleFactoryAndroid, "
         << "bindingPtr, failed to find module: " << name
         << ", local_ref is IsNull()");
    return std::shared_ptr<LynxNativeModule>(nullptr);
  }
  JNIEnv* localEnv = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jstring> moduleName =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(localEnv, name);
  auto wrapper = Java_LynxModuleFactory_moduleWrapperForName(
      localEnv, local_ref.Get(), moduleName.Get());
  if (wrapper.Get() != nullptr) {
    auto android_value_factory =
        std::make_shared<pub::ValueImplAndroidFactory>();
    return std::make_shared<LynxModuleAndroid>(
        localEnv, wrapper.Get(), std::move(android_value_factory));
  }
  return std::shared_ptr<LynxNativeModule>(nullptr);
}

std::shared_ptr<ModuleFactoryAndroid> ModuleFactoryAndroid::CreateOrReuse(
    JNIEnv* env, jobject moduleFactory) {
  if (env->IsSameObject(moduleFactory, nullptr)) {
    return std::shared_ptr<ModuleFactoryAndroid>(nullptr);
  }
  int64_t reuse_ptr = Java_LynxModuleFactory_getNativePtr(env, moduleFactory);
  if (reuse_ptr != 0) {
    LOGI("NativeModule: module factory reuse: " << reuse_ptr);
    ModuleFactoryAndroid* module_factory_android =
        reinterpret_cast<ModuleFactoryAndroid*>(reuse_ptr);
    return module_factory_android->shared_from_this();
  }
  return std::make_shared<ModuleFactoryAndroid>(env, moduleFactory);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
