// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <cstdint>
#include <string_view>
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
constexpr size_t kInstanceValueCount = 8;
constexpr size_t kInstanceStringCount = 4;
// Matches LynxGlobalMemoryUsageResult.fromNative:
// Live values: id, total, element, count, view, MTS, BTS, whole BTS heap.
// Destroyed values: id, whole BTS heap, residual.
// Both strings: page key, URL, group, type.
constexpr size_t kDestroyedValueCount = 3;

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
    JNIEnv* env,
    const std::vector<lynx::tasm::performance::InstanceMemoryUsage>&
        instances) {
  lynx::base::android::ScopedLocalJavaRef<jclass> string_class(
      env, env->FindClass("java/lang/String"));
  lynx::base::android::ScopedLocalJavaRef<jobjectArray> result(
      env, env->NewObjectArray(
               static_cast<jsize>(instances.size() * kInstanceStringCount),
               string_class.Get(), nullptr));
  size_t index = 0;
  for (const auto& instance : instances) {
    const std::string_view values[kInstanceStringCount] = {
        instance.page_id, instance.url, instance.bts_runtime_group_id,
        instance.bts_runtime_type};
    for (const auto value : values) {
      auto string = lynx::base::android::JNIConvertHelper::U16StringToJNIString(
          env, lynx::base::U8StringToU16(value));
      env->SetObjectArrayElement(result.Get(), static_cast<jsize>(index++),
                                 string.Get());
    }
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
            instance_values.reserve(usage.instances.size() *
                                    kInstanceValueCount);
            for (const auto& instance : usage.instances) {
              const auto& page = instance.page;
              instance_values.insert(instance_values.end(),
                                     {static_cast<jlong>(instance.instance_id),
                                      static_cast<jlong>(page.total_bytes),
                                      static_cast<jlong>(page.element_bytes),
                                      static_cast<jlong>(page.element_count),
                                      static_cast<jlong>(page.ui_bytes),
                                      static_cast<jlong>(page.mts_bytes),
                                      static_cast<jlong>(page.bts_bytes),
                                      instance.bts_heap_bytes});
            }

            std::vector<jlong> destroyed_values;
            destroyed_values.reserve(usage.destroyed_instances.size() *
                                     kDestroyedValueCount);
            for (const auto& instance : usage.destroyed_instances) {
              destroyed_values.insert(
                  destroyed_values.end(),
                  {instance.instance_id, instance.bts_heap_bytes,
                   instance.bts_residual_bytes});
            }

            auto j_global_values = ToJavaLongArray(env, global_values);
            auto j_instance_values = ToJavaLongArray(env, instance_values);
            auto j_instance_strings = ToJavaStringArray(env, usage.instances);
            auto j_destroyed_values = ToJavaLongArray(env, destroyed_values);
            auto j_destroyed_strings =
                ToJavaStringArray(env, usage.destroyed_instances);
            Java_LynxMemoryUsageQuery_onNativeMemoryUsageResult(
                env, callback.Get(), collection_start_ms, collection_timeout_ms,
                static_cast<jint>(usage.status), j_global_values.Get(),
                j_instance_values.Get(), j_instance_strings.Get(),
                j_destroyed_values.Get(), j_destroyed_strings.Get());
          });
}

namespace lynx {
namespace jni {

bool RegisterJNIForLynxMemoryUsageQuery(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx
