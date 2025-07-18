// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/lynx_devtool/android/cdp_event_listener_sender_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/CDPEventListenerWrapper_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/CDPEventListenerWrapper_register_jni.h"

namespace lynx {
namespace jni {

bool RegisterJNIForCDPEventListenerWrapper(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace devtool {

CDPEventListenerSenderAndroid::CDPEventListenerSenderAndroid(JNIEnv* env,
                                                             jobject listener)
    : jobj_ptr_(
          std::make_unique<lynx::base::android::ScopedGlobalJavaRef<jobject>>(
              env, listener)) {}

void CDPEventListenerSenderAndroid::SendMessage(const std::string& type,
                                                const Json::Value& msg) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> msgJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, msg.toStyledString());
  Java_CDPEventListenerWrapper_onEvent(env, jobj_ptr_->Get(), msgJStr.Get());
}
void CDPEventListenerSenderAndroid::SendMessage(const std::string& type,
                                                const std::string& msg) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> msgJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, msg);
  Java_CDPEventListenerWrapper_onEvent(env, jobj_ptr_->Get(), msgJStr.Get());
}

}  // namespace devtool
}  // namespace lynx
