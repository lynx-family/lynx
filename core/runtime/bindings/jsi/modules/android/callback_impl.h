// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_CALLBACK_IMPL_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_CALLBACK_IMPL_H_

#include <jni.h>

#include <functional>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

class MethodInvoker;
class LynxPromiseImpl;

class ModuleCallbackAndroid : public ModuleCallback {
 public:
  using CallbackPair = std::pair<std::shared_ptr<ModuleCallbackAndroid>,
                                 base::android::ScopedGlobalJavaRef<jobject>>;
  static bool RegisterJNI(JNIEnv* env);
  static CallbackPair createCallbackImpl(
      int64_t callback_id, std::shared_ptr<MethodInvoker> invoker,
      ModuleCallbackType type = ModuleCallbackType::Base);

  ModuleCallbackAndroid(int64_t callback_id,
                        std::shared_ptr<MethodInvoker> invoker);
  ~ModuleCallbackAndroid() = default;

  std::weak_ptr<MethodInvoker> methodInvoker;
  std::weak_ptr<LynxPromiseImpl> promise;

  inline void setArguments(base::android::ScopedGlobalJavaRef<jobject> obj) {
    arguments = obj;
  };
  base::android::ScopedGlobalJavaRef<jobject> GetArgumentsRef() const {
    return arguments;
  }

  void Invoke(Runtime* runtime, ModuleCallbackFunctionHolder* holder) override;

 private:
  base::android::ScopedGlobalJavaRef<jobject> arguments;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_CALLBACK_IMPL_H_
