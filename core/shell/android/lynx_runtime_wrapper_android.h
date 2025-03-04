// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_LYNX_RUNTIME_WRAPPER_ANDROID_H_
#define CORE_SHELL_ANDROID_LYNX_RUNTIME_WRAPPER_ANDROID_H_

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/bindings/jsi/modules/android/module_factory_android.h"
#include "core/shell/native_facade.h"
#include "core/shell/native_facade_empty_implementation.h"
#include "core/shell/runtime_standalone_helper.h"

namespace lynx {
namespace shell {

class NativeRuntimeFacadeAndroid : public NativeFacadeEmptyImpl {
 public:
  NativeRuntimeFacadeAndroid(JNIEnv* env, jobject jni_object)
      : jni_object_(env, jni_object) {}
  ~NativeRuntimeFacadeAndroid() override = default;
  void ReportError(const base::LynxError& error) override;
  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};

class LynxRuntimeWrapperAndroid {
 public:
  LynxRuntimeWrapperAndroid(
      InitRuntimeStandaloneResult runtime_standalone_bundle,
      std::string group_name,
      std::shared_ptr<lynx::piper::LynxModuleManager> module_manager)
      : runtime_standalone_bundle_(std::move(runtime_standalone_bundle)),
        group_name_(std::move(group_name)),
        weak_module_manager_(module_manager) {}

  ~LynxRuntimeWrapperAndroid() = default;
  LynxRuntimeWrapperAndroid(const LynxRuntimeWrapperAndroid& facade) = delete;
  LynxRuntimeWrapperAndroid& operator=(const LynxRuntimeWrapperAndroid&) =
      delete;
  LynxRuntimeWrapperAndroid(LynxRuntimeWrapperAndroid&& facade) = delete;
  LynxRuntimeWrapperAndroid& operator=(LynxRuntimeWrapperAndroid&&) = delete;

  static void RegisterJNI(JNIEnv* env);

  void SetPresetData(lepus::Value data);

  void EvaluateScript(std::string url, std::string script);

  void DestroyRuntime();

  void SetSessionStorageItem(const std::string& key,
                             const std::shared_ptr<tasm::TemplateData>& data);
  void GetSessionStorageItem(const std::string& key,
                             std::unique_ptr<PlatformCallBack> callback);

  double SubscribeSessionStorage(const std::string& key,
                                 std::unique_ptr<PlatformCallBack> callback);

  void UnSubscribeSessionStorage(const std::string& key, double callback_id);

  const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& GetRuntimeActor() {
    return runtime_standalone_bundle_.runtime_actor_;
  }

  const std::shared_ptr<LynxActor<tasm::timing::TimingHandler>>&
  GetTimingActor() {
    return runtime_standalone_bundle_.timing_actor_;
  }

  std::weak_ptr<lynx::piper::LynxModuleManager> GetModuleManager() {
    return weak_module_manager_;
  }

 private:
  const InitRuntimeStandaloneResult
      runtime_standalone_bundle_;  // keep this read-only until destruction
  std::string group_name_;
  // Only ref module_manager;
  std::weak_ptr<lynx::piper::LynxModuleManager> weak_module_manager_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_LYNX_RUNTIME_WRAPPER_ANDROID_H_
