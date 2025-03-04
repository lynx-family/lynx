// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_NATIVE_FACADE_ANDROID_H_
#define CORE_SHELL_ANDROID_NATIVE_FACADE_ANDROID_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/native_facade.h"

namespace lynx {
namespace shell {

class NativeFacadeAndroid : public NativeFacade {
 public:
  NativeFacadeAndroid(JNIEnv* env, jobject jni_object)
      : jni_object_(env, jni_object) {}
  ~NativeFacadeAndroid() override = default;

  NativeFacadeAndroid(const NativeFacadeAndroid& facade) = delete;
  NativeFacadeAndroid& operator=(const NativeFacadeAndroid&) = delete;

  NativeFacadeAndroid(NativeFacadeAndroid&& facade) = default;
  NativeFacadeAndroid& operator=(NativeFacadeAndroid&&) = default;

  static void RegisterJni(JNIEnv* env);

  void OnDataUpdated() override;

  void OnPageChanged(bool is_first_screen) override;

  void OnTemplateLoaded(const std::string& url) override;

  void OnSSRHydrateFinished(const std::string& url) override;

  void OnRuntimeReady() override;

  void OnTasmFinishByNative() override;

  void ReportError(const base::LynxError& error) override;

  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override;

  void OnTimingSetup(const lepus::Value& timing_info) override;

  void OnTimingUpdate(const lepus::Value& timing_info,
                      const lepus::Value& update_timing,
                      const std::string& update_flag) override;

  void OnDynamicComponentPerfReady(const lepus::Value& perf_info) override;

  void OnConfigUpdated(const lepus::Value& data) override;

  void OnUpdateDataWithoutChange() override;

  void TriggerLepusMethodAsync(const std::string& method_name,
                               const lepus::Value& argus) override;

  void FlushJSBTiming(piper::NativeModuleInfo timing) override;

  void InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                      const std::string& method,
                      std::unique_ptr<tasm::PropBundle> params,
                      piper::ApiCallBack callback) override;

  void OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) override;

  void OnReceiveMessageEvent(runtime::MessageEvent event) override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_NATIVE_FACADE_ANDROID_H_
