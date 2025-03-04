// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/android/event_converter_android.h"

#include "core/base/android/jni_helper.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/bindings/common/event/context_proxy.h"

namespace lynx {
namespace tasm {
namespace android {

base::android::JavaOnlyMap
EventConverterAndroid::ConvertMessageEventToJavaOnlyMap(
    const runtime::MessageEvent& event) {
  base::android::JavaOnlyMap jni_map;
  jni_map.PushString(kType, event.type());
  jni_map.PushInt64(kTimestamp, event.time_stamp());
  jni_map.PushString(kTarget, event.GetTargetString());
  jni_map.PushString(kOrigin, event.GetOriginString());
  ValueConverterAndroid::PushKeyAndValueToJavaOnlyMap(jni_map, kData,
                                                      event.message());
  return jni_map;
}

runtime::MessageEvent EventConverterAndroid::ConvertJavaOnlyMapToMessageEvent(
    JNIEnv* env, jobject map) {
  auto event = ValueConverterAndroid::ConvertJavaOnlyMapToLepus(env, map);
  const auto& type = event.GetProperty(BASE_STATIC_STRING(kType)).StdString();
  auto time_stamp = static_cast<int64_t>(
      event.GetProperty(BASE_STATIC_STRING(kTimestamp)).Number());
  auto target = runtime::ContextProxy::ConvertStringToContextType(
      event.GetProperty(BASE_STATIC_STRING(kTarget)).StdString());
  auto origin = runtime::ContextProxy::ConvertStringToContextType(
      event.GetProperty(BASE_STATIC_STRING(kOrigin)).StdString());
  auto data = event.GetProperty(BASE_STATIC_STRING(kData));
  return runtime::MessageEvent(type, time_stamp, origin, target, data);
}

}  // namespace android
}  // namespace tasm
}  // namespace lynx
