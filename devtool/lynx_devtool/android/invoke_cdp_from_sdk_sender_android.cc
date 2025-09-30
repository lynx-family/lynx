// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/lynx_devtool/android/invoke_cdp_from_sdk_sender_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/CDPResultCallbackWrapper_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/CDPResultCallbackWrapper_register_jni.h"

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForCDPResultCallbackWrapper(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace devtool
}  // namespace lynx

namespace lynx {
namespace devtool {

InvokeCDPFromSDKSenderAndroid::InvokeCDPFromSDKSenderAndroid(JNIEnv* env,
                                                             jobject callback)
    : jobj_ptr_(
          std::make_unique<lynx::base::android::ScopedGlobalJavaRef<jobject>>(
              env, callback)) {}

void InvokeCDPFromSDKSenderAndroid::SendMessage(const std::string& type,
                                                const Json::Value& msg) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> msgJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, msg.toStyledString());
  Java_CDPResultCallbackWrapper_onResult(env, jobj_ptr_->Get(), msgJStr.Get());
}
void InvokeCDPFromSDKSenderAndroid::SendMessage(const std::string& type,
                                                const std::string& msg) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> msgJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, msg);
  Java_CDPResultCallbackWrapper_onResult(env, jobj_ptr_->Get(), msgJStr.Get());
}

}  // namespace devtool
}  // namespace lynx
