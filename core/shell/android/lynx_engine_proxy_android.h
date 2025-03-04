// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_LYNX_ENGINE_PROXY_ANDROID_H_
#define CORE_SHELL_ANDROID_LYNX_ENGINE_PROXY_ANDROID_H_

#include <jni.h>

#include <memory>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/lynx_engine_proxy_impl.h"

namespace lynx {
namespace shell {

class LynxEngineProxyAndroid : public LynxEngineProxyImpl {
 public:
  static bool RegisterJNI(JNIEnv *env);
  LynxEngineProxyAndroid(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> actor, JNIEnv *env,
      jobject impl)
      : LynxEngineProxyImpl(actor), impl_(env, impl) {}
  virtual ~LynxEngineProxyAndroid() override = default;
  void InvokeLepusApiCallback(JNIEnv *env, jobject jcaller, jlong nativePtr,
                              jint callbackID, jstring entryName, jobject data);

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> impl_;
};
}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_LYNX_ENGINE_PROXY_ANDROID_H_
