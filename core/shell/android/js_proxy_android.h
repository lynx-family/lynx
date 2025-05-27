// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_JS_PROXY_ANDROID_H_
#define CORE_SHELL_ANDROID_JS_PROXY_ANDROID_H_

#include <memory>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/lynx_runtime_proxy_impl.h"
#include "core/shell/lynx_shell.h"

namespace lynx {
namespace shell {

// call by platform android
class JSProxyAndroid : public LynxRuntimeProxyImpl {
 public:
  JSProxyAndroid(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& actor,
                 int64_t id, JNIEnv* env, jobject jni_object,
                 const std::string& js_group_thread_name,
                 bool runtime_standalone_mode)
      : LynxRuntimeProxyImpl(actor, runtime_standalone_mode),
        id_(id),
        jni_object_(
            std::make_shared<base::android::ScopedWeakGlobalJavaRef<jobject>>(
                env, jni_object)),
        js_group_thread_name_(js_group_thread_name) {}
  ~JSProxyAndroid() = default;

  JSProxyAndroid(const JSProxyAndroid&) = delete;
  JSProxyAndroid& operator=(const JSProxyAndroid&) = delete;
  JSProxyAndroid(JSProxyAndroid&&) = delete;
  JSProxyAndroid& operator=(JSProxyAndroid&&) = delete;

  int64_t GetId() const { return id_; }

  jobject GetJniObject() const {
    return jni_object_ ? jni_object_->Get() : nullptr;
  }

  void CallJSFunctionByArgsId(std::string module_id, std::string method_id,
                              long args_id);

  void CallJSIntersectionObserverByArgsId(int32_t observer_id,
                                          int32_t callback_id, long args_id);

  void CallJSApiCallbackWithValueByArgsId(int32_t callback_id, long args_id);

  void RunOnJSThread(base::MoveOnlyClosure<> task);

 private:
  static std::unique_ptr<pub::Value> GetArgs(
      std::shared_ptr<base::android::ScopedWeakGlobalJavaRef<jobject>> jobject,
      long args_id, bool is_array);
  const int64_t id_;

  std::shared_ptr<base::android::ScopedWeakGlobalJavaRef<jobject>> jni_object_;
  std::string js_group_thread_name_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_JS_PROXY_ANDROID_H_
