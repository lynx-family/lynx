// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/android/devtool_slot_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/base_devtool/build/gen/DevToolSlot_jni.h"

static void OnSlotMessage(JNIEnv* env, jobject jcaller, jlong nativeHandler,
                          jstring type, jstring message) {
  lynx::devtool::DevToolSlotDelegate* devtool_slot =
      reinterpret_cast<lynx::devtool::DevToolSlotDelegate*>(nativeHandler);
  if (devtool_slot) {
    devtool_slot->OnMessage(
        lynx::base::android::JNIConvertHelper::ConvertToString(env, type),
        lynx::base::android::JNIConvertHelper::ConvertToString(env, message));
  }
}
namespace lynx {
namespace devtool {

std::shared_ptr<DevToolSlot> DevToolSlot::Create(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate) {
  return std::make_shared<DevToolSlotDelegate>(delegate);
}

DevToolSlotDelegate::DevToolSlotDelegate(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolSlot(delegate) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> jobj =
      Java_DevToolSlot_createInstance(env, reinterpret_cast<jlong>(this));
  jobj_ptr_ =
      std::make_unique<lynx::base::android::ScopedGlobalJavaRef<jobject>>(
          env, jobj.Get());
}
int32_t DevToolSlotDelegate::Plug(const std::string& url) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> urlJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  return Java_DevToolSlot_plug(env, jobj_ptr_->Get(), urlJStr.Get());
}
void DevToolSlotDelegate::Pull() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_DevToolSlot_pull(env, jobj_ptr_->Get());
}

void DevToolSlotDelegate::SendMessage(const std::string& type,
                                      const std::string& msg) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> typeJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, type);
  lynx::base::android::ScopedLocalJavaRef<jstring> msgJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, msg);
  Java_DevToolSlot_sendMessage(env, jobj_ptr_->Get(), typeJStr.Get(),
                               msgJStr.Get());
}

bool DevToolSlotDelegate::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace devtool
}  // namespace lynx
