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
  void OnEvaluateJavaScriptEnd(const std::string& url) override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};

class LynxRuntimeWrapperAndroid {
 public:
  explicit LynxRuntimeWrapperAndroid(
      std::unique_ptr<RuntimeStandalone> runtime_standalone)
      : runtime_standalone_(std::move(runtime_standalone)) {}

  ~LynxRuntimeWrapperAndroid() = default;
  LynxRuntimeWrapperAndroid(const LynxRuntimeWrapperAndroid& facade) = delete;
  LynxRuntimeWrapperAndroid& operator=(const LynxRuntimeWrapperAndroid&) =
      delete;
  LynxRuntimeWrapperAndroid(LynxRuntimeWrapperAndroid&& facade) = delete;
  LynxRuntimeWrapperAndroid& operator=(LynxRuntimeWrapperAndroid&&) = delete;

  RuntimeStandalone& RuntimeStandalone() { return *runtime_standalone_; }

 private:
  std::unique_ptr<shell::RuntimeStandalone> runtime_standalone_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_LYNX_RUNTIME_WRAPPER_ANDROID_H_
