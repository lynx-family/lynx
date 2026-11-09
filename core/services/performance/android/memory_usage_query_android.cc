// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "base/include/string/string_utils.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxMemoryUsageQuery_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxMemoryUsageQuery_register_jni.h"

namespace {

constexpr size_t kGlobalValueCount = 6;
constexpr size_t kInstanceValueCount = 7;
constexpr size_t kInstanceStringCount = 3;

lynx::base::android::ScopedLocalJavaRef<jlongArray> ToJavaLongArray(
    JNIEnv* env, const std::vector<jlong>& values) {
  lynx::base::android::ScopedLocalJavaRef<jlongArray> result(
      env, env->NewLongArray(static_cast<jsize>(values.size())));
  if (!values.empty()) {
    env->SetLongArrayRegion(result.Get(), 0, static_cast<jsize>(values.size()),
                            values.data());
  }
  return result;
}

lynx::base::android::ScopedLocalJavaRef<jobjectArray> ToJavaStringArray(
    JNIEnv* env, const std::vector<std::string>& values) {
  lynx::base::android::ScopedLocalJavaRef<jclass> string_class(
      env, env->FindClass("java/lang/String"));
  lynx::base::android::ScopedLocalJavaRef<jobjectArray> result(
      env, env->NewObjectArray(static_cast<jsize>(values.size()),
                               string_class.Get(), nullptr));
  for (size_t i = 0; i < values.size(); ++i) {
    auto value = lynx::base::android::JNIConvertHelper::U16StringToJNIString(
        env, lynx::base::U8StringToU16(values[i]));
    env->SetObjectArrayElement(result.Get(), static_cast<jsize>(i),
                               value.Get());
  }
  return result;
}

}  // namespace

static void QueryGlobalMemoryUsageAsync(JNIEnv* env, jclass jcaller,
                                        jobject callback,
                                        jlong collection_start_ms,
                                        jlong collection_timeout_ms) {
  lynx::base::android::ScopedGlobalJavaRef<jobject> global_callback(env,
                                                                    callback);
  if (global_callback.IsNull()) return;
  lynx::tasm::performance::GlobalMemoryMonitor::GetInstance()
      .GetGlobalMemoryUsage(
          [callback = std::move(global_callback), collection_start_ms,
           collection_timeout_ms](
              lynx::tasm::performance::GlobalMemoryUsage usage) mutable {
            JNIEnv* env = lynx::base::android::AttachCurrentThread();
            const auto& total = usage.total;
            std::vector<jlong> global_values;
            global_values.reserve(kGlobalValueCount);
            global_values.insert(global_values.end(),
                                 {static_cast<jlong>(total.total_bytes),
                                  static_cast<jlong>(total.element_bytes),
                                  static_cast<jlong>(total.element_count),
                                  static_cast<jlong>(total.ui_bytes),
                                  static_cast<jlong>(total.mts_bytes),
                                  static_cast<jlong>(total.bts_bytes)});

            std::vector<jlong> instance_values;
            std::vector<std::string> instance_strings;
            instance_values.reserve(usage.instances.size() *
                                    kInstanceValueCount);
            instance_strings.reserve(usage.instances.size() *
                                     kInstanceStringCount);
            for (const auto& instance : usage.instances) {
              const auto& page = instance.page;
              instance_values.insert(instance_values.end(),
                                     {static_cast<jlong>(instance.instance_id),
                                      static_cast<jlong>(page.total_bytes),
                                      static_cast<jlong>(page.element_bytes),
                                      static_cast<jlong>(page.element_count),
                                      static_cast<jlong>(page.ui_bytes),
                                      static_cast<jlong>(page.mts_bytes),
                                      static_cast<jlong>(page.bts_bytes)});
              instance_strings.insert(instance_strings.end(),
                                      {instance.page_id, instance.url,
                                       instance.bts_runtime_group_id});
            }

            auto j_global_values = ToJavaLongArray(env, global_values);
            auto j_instance_values = ToJavaLongArray(env, instance_values);
            auto j_instance_strings = ToJavaStringArray(env, instance_strings);
            Java_LynxMemoryUsageQuery_onNativeMemoryUsageResult(
                env, callback.Get(), collection_start_ms, collection_timeout_ms,
                j_global_values.Get(), j_instance_values.Get(),
                j_instance_strings.Get());
          });
}

namespace lynx {
namespace jni {

bool RegisterJNIForLynxMemoryUsageQuery(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx
