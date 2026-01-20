// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bytecode/android/bytecode_callback.h"

#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxBytecodeCallback_jni.h"

namespace lynx {
namespace jni {

bool RegisterJNIForLynxBytecodeCallback(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace runtime {
namespace js {
namespace cache {

void OnBytecodeResponse(JNIEnv* env,
                        base::android::ScopedGlobalJavaRef<jobject> obj,
                        base::android::ScopedLocalJavaRef<jstring> error_msg,
                        base::android::JavaOnlyMap& java_map) {
  Java_LynxBytecodeCallback_onResponse(
      env, obj.Get(), error_msg.IsNull() ? nullptr : error_msg.Get(),
      java_map.jni_object());
}

std::unique_ptr<BytecodeGenerateCallback> CreateBytecodeCallback(
    JNIEnv* env, jobject callback) {
  std::unique_ptr<cache::BytecodeGenerateCallback> bytecode_callback = nullptr;
  if (nullptr != callback) {
    lynx::base::android::ScopedGlobalJavaRef<jobject> jni_object(env, callback);
    bytecode_callback = std::make_unique<cache::BytecodeGenerateCallback>(
        [jni_object = std::move(jni_object)](
            std::string error_msg,
            std::unordered_map<std::string, std::shared_ptr<Buffer>> buffers) {
          JNIEnv* env = lynx::base::android::AttachCurrentThread();
          lynx::base::android::ScopedLocalJavaRef<jstring> jni_error_msg;
          if (!error_msg.empty()) {
            jni_error_msg =
                lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
                    env, error_msg);
          }
          auto java_map = lynx::base::android::JavaOnlyMap();
          for (const auto& iter : buffers) {
            if (nullptr != iter.second) {
              jobject byte_buffer = env->NewDirectByteBuffer(
                  const_cast<void*>(
                      static_cast<const void*>(iter.second->data())),
                  iter.second->size());
              java_map.PushByteBuffer(iter.first, byte_buffer);
            }
          }
          cache::OnBytecodeResponse(env, std::move(jni_object),
                                    std::move(jni_error_msg), java_map);
        });
  }
  return bytecode_callback;
}

}  // namespace cache
}  // namespace js
}  // namespace runtime
}  // namespace lynx
