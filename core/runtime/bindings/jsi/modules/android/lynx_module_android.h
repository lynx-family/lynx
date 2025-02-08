// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_MODULE_ANDROID_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_MODULE_ANDROID_H_

#include <jni.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/expected.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "core/runtime/bindings/jsi/modules/android/java_attribute_descriptor.h"
#include "core/runtime/bindings/jsi/modules/android/method_invoker.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "lynx/core/runtime/bindings/jsi/modules/android/callback_impl.h"

namespace lynx {

namespace piper {

using CallbackHolders =
    std::unordered_map<uint64_t, ModuleCallbackAndroid::CallbackPair>;

class LynxModuleAndroid
    : public LynxNativeModule,
      public std::enable_shared_from_this<LynxModuleAndroid> {
 public:
  static bool RegisterJNI(JNIEnv *env);
  static std::string getName(jobject jni_object);
  LynxModuleAndroid(JNIEnv *env, jobject jni_object,
                    std::shared_ptr<pub::PubValueFactory> value_factory);
  void Destroy() override;
  // Build the Module Method Map and cache the metaData of the methods
  // registered on the Android Platform. MethodInvoker represents a ModuleMethod
  // registered by Android
  void buildMap(JNIEnv *env,
                lynx::base::android::ScopedLocalJavaRef<jobject> &descriptions,
                NativeModuleMethods &methods,
                std::unordered_map<std::string, std::shared_ptr<MethodInvoker>>
                    &method_invoker_maps);
  const base::android::ScopedGlobalJavaRef<jobject> &CreateLynxModuleCallback(
      const std::shared_ptr<LynxModuleCallback> &base_callback);
  // use delegate invoke Callback
  void InvokeCallback(const std::shared_ptr<ModuleCallback> &callback,
                      std::weak_ptr<LynxPromiseImpl> promise);
  // for timing api & native promise
  // TODO(zhangqun.29) We will remove this method after remove native promise
  void EnterInvokeScope(
      Runtime *rt, std::shared_ptr<ModuleDelegate> module_delegate) override;
  void ExitInvokeScope() override;
  std::optional<piper::Value> TryGetPromiseRet() override;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string &method_name, std::unique_ptr<pub::Value> args,
      size_t count, const CallbackMap &callbacks) override;

  ModuleCallbackAndroid *GetModuleCallbackById(uint64_t callback_id);

 private:
  std::string module_name_;
  // cache the metaData of the methods registered on the Android Platform
  base::android::ScopedWeakGlobalJavaRef<jobject> wrapper_;
  // A MethodInvoker represents a ModuleMethod registered by Android
  std::unordered_map<std::string, std::shared_ptr<MethodInvoker>>
      method_invokers_;

  // for timing api & native promise
  // TODO(zhangqun.29) We will remove this method after remove native promise
  std::vector<Runtime *> scope_rts_;
  std::vector<piper::NativeModuleInfoCollectorPtr> scope_timing_collectors_;
  std::vector<std::optional<piper::Value>> scope_native_promise_rets_;
  // TODO(zhangqun.29) LynxNativeModule::Delegate does not have OnErrorOccurred
  // & OnMethodInvoked methods, and the delegate will be removed later.
  std::shared_ptr<ModuleDelegate> legacy_module_delegate_;

  CallbackHolders callbackHolders_;
  std::unordered_set<std::shared_ptr<LynxPromiseImpl>> promises_;

  base::expected<piper::Value, std::string> CreateLynxNativePromise(
      const std::shared_ptr<MethodInvoker> &invoker, jobject module,
      const pub::Value *method_args, size_t args_count,
      const CallbackMap &callbacks);
  void InvokeCallbackInternal(const std::shared_ptr<ModuleCallback> &callback,
                              std::weak_ptr<LynxPromiseImpl> promise);

  inline Runtime *GetScopeRuntime() {
    if (!scope_rts_.empty()) {
      auto scope_rt = scope_rts_.back();
      return scope_rt;
    }
    return nullptr;
  }

  inline piper::NativeModuleInfoCollectorPtr GetScopeTimingCollector() {
    if (!scope_timing_collectors_.empty()) {
      auto scope_timing_collector = scope_timing_collectors_.back();
      return scope_timing_collector;
    }
    return nullptr;
  }
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_MODULE_ANDROID_H_
