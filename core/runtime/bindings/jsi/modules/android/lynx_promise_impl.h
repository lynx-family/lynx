// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_PROMISE_IMPL_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_PROMISE_IMPL_H_

#include <memory>

#include "core/runtime/bindings/jsi/modules/android/callback_impl.h"

namespace lynx {
namespace piper {
class LynxPromiseImpl {
 public:
  static bool RegisterJNI(JNIEnv *env);
  LynxPromiseImpl(ModuleCallbackAndroid::CallbackPair resolve_pair,
                  ModuleCallbackAndroid::CallbackPair reject_pair);
  std::shared_ptr<ModuleCallbackAndroid> GetResolve();
  std::shared_ptr<ModuleCallbackAndroid> GetReject();
  jobject GetJniObject();

 private:
  std::shared_ptr<ModuleCallbackAndroid> resolveCallback_;
  std::shared_ptr<ModuleCallbackAndroid> rejectCallback_;
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_PROMISE_IMPL_H_
