// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/android/devtool_global_slot_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "devtool/base_devtool/build/gen/DevToolGlobalSlot_jni.h"

static void OnGlobalSlotMessage(JNIEnv* env, jobject jcaller,
                                jlong nativeHandler, jstring type,
                                jstring message) {
  lynx::devtool::DevToolGlobalSlotDelegate* devtool_global_slot =
      reinterpret_cast<lynx::devtool::DevToolGlobalSlotDelegate*>(
          nativeHandler);
  if (devtool_global_slot) {
    devtool_global_slot->OnMessage(
        lynx::base::android::JNIConvertHelper::ConvertToString(env, type),
        lynx::base::android::JNIConvertHelper::ConvertToString(env, message));
  }
}

namespace lynx {
namespace devtool {

std::shared_ptr<DevToolGlobalSlot> DevToolGlobalSlot::Create(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate) {
  return std::make_shared<DevToolGlobalSlotDelegate>(delegate);
}

DevToolGlobalSlotDelegate::DevToolGlobalSlotDelegate(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolGlobalSlot(delegate) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> jobj =
      Java_DevToolGlobalSlot_createInstance(env, reinterpret_cast<jlong>(this));
  jobj_ptr_ =
      std::make_unique<lynx::base::android::ScopedGlobalJavaRef<jobject>>(
          env, jobj.Get());
}

void DevToolGlobalSlotDelegate::SendMessage(const std::string& type,
                                            const std::string& msg) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> typeJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, type);
  lynx::base::android::ScopedLocalJavaRef<jstring> msgJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, msg);
  Java_DevToolGlobalSlot_sendMessage(env, jobj_ptr_->Get(), typeJStr.Get(),
                                     msgJStr.Get());
}

bool DevToolGlobalSlotDelegate::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace devtool
}  // namespace lynx
