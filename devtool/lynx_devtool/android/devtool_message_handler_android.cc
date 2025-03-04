// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/lynx_devtool/android/devtool_message_handler_android.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "devtool/lynx_devtool/build/gen/DevToolMessageHandlerDelegate_jni.h"

namespace lynx {
namespace devtool {

DevToolMessageHandlerAndroid::DevToolMessageHandlerAndroid(JNIEnv* env,
                                                           jobject handler)
    : jobj_ptr_(
          std::make_unique<lynx::base::android::ScopedGlobalJavaRef<jobject>>(
              env, handler)) {}
void DevToolMessageHandlerAndroid::handle(
    const std::shared_ptr<MessageSender>& sender, const std::string& type,
    const Json::Value& message) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jstring> msgJStr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, message.toStyledString());
  return Java_DevToolMessageHandlerDelegate_onMessage(env, jobj_ptr_->Get(),
                                                      msgJStr.Get());
}
bool DevToolMessageHandlerAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace devtool
}  // namespace lynx
