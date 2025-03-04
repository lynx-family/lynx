// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_ANDROID_INVOKE_CDP_FROM_SDK_SENDER_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_ANDROID_INVOKE_CDP_FROM_SDK_SENDER_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {

class InvokeCDPFromSDKSenderAndroid : public lynx::devtool::MessageSender {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
  InvokeCDPFromSDKSenderAndroid(JNIEnv* env, jobject callback);
  void SendMessage(const std::string& type, const Json::Value& msg) override;
  void SendMessage(const std::string& type, const std::string& msg) override;

 private:
  std::unique_ptr<lynx::base::android::ScopedGlobalJavaRef<jobject>> jobj_ptr_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_ANDROID_INVOKE_CDP_FROM_SDK_SENDER_ANDROID_H_
