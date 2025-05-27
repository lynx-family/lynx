// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_ANDROID_H_
#define CORE_SHELL_ANDROID_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_ANDROID_H_

#include <jni.h>

#include <memory>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/public/vsync_observer_interface.h"
#include "core/runtime/piper/js/runtime_lifecycle_listener_delegate.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace shell {
class RuntimeLifecycleListenerDelegateAndroid
    : public runtime::RuntimeLifecycleListenerDelegate {
 public:
  RuntimeLifecycleListenerDelegateAndroid(JNIEnv* env, jobject delegate);
  ~RuntimeLifecycleListenerDelegateAndroid() override = default;

  void OnRuntimeAttach(Napi::Env env) override;
  void OnRuntimeDetach() override;

 private:
  void OnRuntimeCreate(
      std::shared_ptr<runtime::IVSyncObserver> observer) override {}
  void OnRuntimeInit(int64_t runtime_id) override {}
  void OnAppEnterForeground() override {}
  void OnAppEnterBackground() override {}

  base::android::ScopedGlobalJavaRef<jobject> impl_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_ANDROID_H_
