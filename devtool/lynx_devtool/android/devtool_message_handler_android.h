// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_ANDROID_DEVTOOL_MESSAGE_HANDLER_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_ANDROID_DEVTOOL_MESSAGE_HANDLER_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

namespace lynx {
namespace devtool {

class DevToolMessageHandlerAndroid : public DevToolMessageHandler {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
  DevToolMessageHandlerAndroid(JNIEnv* env, jobject handler);
  void handle(const std::shared_ptr<MessageSender>& sender,
              const std::string& type, const Json::Value& message) override;

 private:
  std::unique_ptr<lynx::base::android::ScopedGlobalJavaRef<jobject>> jobj_ptr_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_ANDROID_DEVTOOL_MESSAGE_HANDLER_ANDROID_H_
