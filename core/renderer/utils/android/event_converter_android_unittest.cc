// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/android/event_converter_android.h"

#include <string>

#include "core/base/android/android_jni.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace android {

TEST(EventConverterAndroidTest, TestConvertFunctions) {
  runtime::MessageEvent event("xxx", 1, runtime::ContextProxy::Type::kJSContext,
                              runtime::ContextProxy::Type::kDevTool,
                              lepus::Value("zzz"));

  JNIEnv* env = base::android::AttachCurrentThread();
  auto map = EventConverterAndroid::ConvertMessageEventToJavaOnlyMap(event);
  auto result = EventConverterAndroid::ConvertJavaOnlyMapToMessageEvent(
      env, map.jni_object());

  EXPECT_EQ(result.type(), "xxx");
  EXPECT_EQ(event.type(), result.type());
  EXPECT_EQ(result.time_stamp(), 1);
  EXPECT_EQ(event.time_stamp(), result.time_stamp());
  EXPECT_EQ(result.GetOriginType(), runtime::ContextProxy::Type::kJSContext);
  EXPECT_EQ(event.GetOriginType(), result.GetOriginType());
  EXPECT_EQ(result.GetOriginString(), std::string(runtime::kJSContext));
  EXPECT_EQ(event.GetOriginString(), result.GetOriginString());
  EXPECT_EQ(result.GetTargetType(), runtime::ContextProxy::Type::kDevTool);
  EXPECT_EQ(event.GetTargetType(), result.GetTargetType());
  EXPECT_EQ(result.GetTargetString(), std::string(runtime::kDevTool));
  EXPECT_EQ(event.GetTargetString(), result.GetTargetString());
  EXPECT_EQ(result.message(), lepus::Value("zzz"));
  EXPECT_EQ(event.message(), result.message());
}

}  // namespace android
}  // namespace tasm
}  // namespace lynx
