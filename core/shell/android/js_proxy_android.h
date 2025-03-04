// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_JS_PROXY_ANDROID_H_
#define CORE_SHELL_ANDROID_JS_PROXY_ANDROID_H_

#include <memory>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/lynx_shell.h"

namespace lynx {
namespace shell {

// call by platform android
class JSProxyAndroid {
 public:
  JSProxyAndroid(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& actor,
                 int64_t id, JNIEnv* env, jobject jni_object,
                 const std::string& js_group_thread_name,
                 bool runtime_standalone_mode)
      : actor_(actor),
        id_(id),
        jni_object_(
            std::make_shared<base::android::ScopedWeakGlobalJavaRef<jobject>>(
                env, jni_object)),
        js_group_thread_name_(js_group_thread_name),
        runtime_standalone_mode_(runtime_standalone_mode) {}
  ~JSProxyAndroid() = default;

  JSProxyAndroid(const JSProxyAndroid&) = delete;
  JSProxyAndroid& operator=(const JSProxyAndroid&) = delete;
  JSProxyAndroid(JSProxyAndroid&&) = delete;
  JSProxyAndroid& operator=(JSProxyAndroid&&) = delete;

  static bool RegisterJNIUtils(JNIEnv* env);

  int64_t GetId() const { return id_; }

  jobject GetJniObject() const {
    return jni_object_ ? jni_object_->Get() : nullptr;
  }

  void CallJSFunction(std::string module_id, std::string method_id,
                      long args_id);

  void CallJSIntersectionObserver(int32_t observer_id, int32_t callback_id,
                                  long args_id);

  void CallJSApiCallbackWithValue(int32_t callback_id, long args_id);

  void EvaluateScript(const std::string& url, std::string script,
                      int32_t callback_id);

  void RejectDynamicComponentLoad(const std::string& url, int32_t callback_id,
                                  int32_t err_code, const std::string& err_msg);

  void RunOnJSThread(base::MoveOnlyClosure<> task);

 private:
  std::shared_ptr<LynxActor<runtime::LynxRuntime>> actor_;

  const int64_t id_;

  std::shared_ptr<base::android::ScopedWeakGlobalJavaRef<jobject>> jni_object_;
  std::string js_group_thread_name_;
  bool runtime_standalone_mode_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_JS_PROXY_ANDROID_H_
