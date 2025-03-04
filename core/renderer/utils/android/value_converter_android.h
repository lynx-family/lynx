// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UTILS_ANDROID_VALUE_CONVERTER_ANDROID_H_
#define CORE_RENDERER_UTILS_ANDROID_VALUE_CONVERTER_ANDROID_H_

#include <jni.h>

#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
namespace android {
class ValueConverterAndroid {
 public:
  // Convert lepus::Value To JavaOnlyMap
  static base::android::JavaOnlyMap ConvertLepusToJavaOnlyMap(
      const lepus::Value& value);
  // Convert lepus::Value To JavaOnlyArray
  static base::android::JavaOnlyArray ConvertLepusToJavaOnlyArray(
      const lepus::Value& value);
  // Convert JavaOnlyArray jobject to lepus::Value
  static lepus::Value ConvertJavaOnlyArrayToLepus(JNIEnv* env, jobject array);
  // Convert JavaOnlyMap jobject to lepus::Value
  static lepus::Value ConvertJavaOnlyMapToLepus(JNIEnv* env, jobject map);

  // Push Key and Value to JavaOnlyMap
  static void PushKeyAndValueToJavaOnlyMap(base::android::JavaOnlyMap& map,
                                           const char* key,
                                           const lepus::Value& value);
  // Push Value to JavaOnlyArray
  static void PushValueToJavaOnlyArray(base::android::JavaOnlyArray& ary,
                                       const lepus::Value& value);

  // Convert lepus::Value To JavaOnlyMap for Timing
  // TODO(kechenglong): Temporary API, will be removed after impl Timing related
  // prop bundle.
  static base::android::JavaOnlyMap ConvertLepusToJavaOnlyMapForTiming(
      const lepus::Value& value);
  // Push Key and Value to JavaOnlyMap for Timing
  // TODO(kechenglong): Temporary API, will be removed after impl Timing related
  // prop bundle.
  static void PushKeyAndValueToJavaOnlyMapForTiming(
      base::android::JavaOnlyMap& map, const char* key,
      const lepus::Value& value);
};

}  // namespace android
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_ANDROID_VALUE_CONVERTER_ANDROID_H_
