// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UTILS_ANDROID_EVENT_CONVERTER_ANDROID_H_
#define CORE_RENDERER_UTILS_ANDROID_EVENT_CONVERTER_ANDROID_H_

#include <jni.h>

#include "core/base/android/java_only_map.h"
#include "core/runtime/bindings/common/event/message_event.h"

namespace lynx {
namespace tasm {
namespace android {
class EventConverterAndroid {
 public:
  // Convert MessageEvent to JavaOnlyMap
  static base::android::JavaOnlyMap ConvertMessageEventToJavaOnlyMap(
      const runtime::MessageEvent& event);
  // Convert JavaOnlyMap to MessageEvent
  static runtime::MessageEvent ConvertJavaOnlyMapToMessageEvent(JNIEnv* env,
                                                                jobject map);
};

}  // namespace android
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_ANDROID_EVENT_CONVERTER_ANDROID_H_
