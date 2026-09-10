// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include "core/base/android/android_jni.h"
#include "core/shell/host_script/android/runtime/host_script_android_session.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptRuntimeBinding_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxHostScriptRuntimeBinding_register_jni.h"

using lynx::shell::HostScriptJavaString;
using lynx::shell::HostScriptSession;
using lynx::shell::HostScriptSessionFromHandle;
using lynx::shell::HostScriptSessionHandle;

namespace lynx {
namespace jni {
bool RegisterJNIForLynxHostScriptRuntimeBinding(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

static jlong Create(JNIEnv* env, jclass, jobject runtime_binding) {
  if (!runtime_binding) {
    return 0;
  }
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> weak(env,
                                                             runtime_binding);
  auto session = HostScriptSession::Create(
      [weak](const std::string& status, const std::string& message) {
        auto* env = lynx::base::android::AttachCurrentThread();
        lynx::base::android::ScopedLocalJavaRef<jobject> binding(weak);
        if (binding.IsNull()) {
          return;
        }
        auto java_status = HostScriptJavaString(env, status);
        auto java_message = HostScriptJavaString(env, message);
        Java_LynxHostScriptRuntimeBinding_onHostScriptEntryResult(
            env, binding.Get(), java_status.Get(), java_message.Get());
      });
  return reinterpret_cast<jlong>(
      new HostScriptSessionHandle(std::move(session)));
}

static void OnRuntimeAttach(JNIEnv*, jclass, jlong ptr, jlong napi_env_ptr) {
  if (ptr && napi_env_ptr) {
    HostScriptSessionFromHandle(ptr).Attach(
        reinterpret_cast<napi_env>(napi_env_ptr));
  }
}

static void OnRuntimeDetach(JNIEnv*, jclass, jlong ptr) {
  if (ptr) {
    HostScriptSessionFromHandle(ptr).Detach();
  }
}

static void Destroy(JNIEnv*, jclass, jlong ptr) {
  delete reinterpret_cast<HostScriptSessionHandle*>(ptr);
}
