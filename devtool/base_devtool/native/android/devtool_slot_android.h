// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_ANDROID_DEVTOOL_SLOT_ANDROID_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_ANDROID_DEVTOOL_SLOT_ANDROID_H_
#include <memory>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/base_devtool/native/devtool_slot.h"

namespace lynx {
namespace devtool {
class DevToolSlotDelegate : public DevToolSlot {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
  explicit DevToolSlotDelegate(
      const std::shared_ptr<DebugRouterMessageSubscriber>& delegate);
  int32_t Plug(const std::string& url) override;
  void Pull() override;
  void SendMessage(const std::string& type, const std::string& msg) override;

 private:
  std::unique_ptr<lynx::base::android::ScopedGlobalJavaRef<jobject>> jobj_ptr_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_ANDROID_DEVTOOL_SLOT_ANDROID_H_
