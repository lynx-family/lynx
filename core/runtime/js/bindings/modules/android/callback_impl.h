// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_CALLBACK_IMPL_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_CALLBACK_IMPL_H_

#include <jni.h>

#include <functional>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"

namespace lynx {
namespace runtime {
namespace js {
class LynxModuleAndroid;
class LynxPromiseImpl;

class ModuleCallbackAndroid {
 public:
  using CallbackPair = std::pair<std::shared_ptr<ModuleCallbackAndroid>,
                                 base::android::ScopedGlobalJavaRef<jobject>>;
  using CallbackArgsConverter = std::function<std::unique_ptr<pub::Value>(
      Runtime* rt, ModuleCallback* callback,
      lynx::base::android::ScopedGlobalJavaRef<jobject> args)>;

  ModuleCallbackAndroid(std::weak_ptr<LynxModuleAndroid> callback_invoker,
                        std::shared_ptr<LynxModuleCallback> callback)
      : callback_invoker_(std::move(callback_invoker)),
        callback_(std::move(callback)) {}
  ~ModuleCallbackAndroid() = default;
  void Invoke(lynx::base::android::ScopedGlobalJavaRef<jobject> args);

  void SetCustomArgsConverter(CallbackArgsConverter converter) {
    custom_args_converter_ = std::move(converter);
  }

  std::shared_ptr<LynxModuleCallback> GetCallback() { return callback_; }

  static CallbackPair CreateCallbackImpl(
      std::shared_ptr<LynxModuleCallback> callback,
      std::shared_ptr<LynxModuleAndroid> invoker);

  std::weak_ptr<LynxPromiseImpl> promise;

 private:
  std::weak_ptr<LynxModuleAndroid> callback_invoker_;
  std::shared_ptr<LynxModuleCallback> callback_;
  CallbackArgsConverter custom_args_converter_;
};
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_ANDROID_CALLBACK_IMPL_H_
