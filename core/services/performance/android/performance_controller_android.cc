// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/android/performance_controller_android.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/jni_helper.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/services/performance/memory_monitor/memory_record.h"
#include "core/services/timing_handler/timing_handler.h"
#include "platform/android/lynx_android/src/main/jni/gen/PerformanceController_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/PerformanceController_register_jni.h"

static void AllocateMemory(JNIEnv* env, jobject jcaller, jlong j_native_ptr,
                           jstring j_category, jlong j_size_bytes,
                           jstring j_detail_key, jstring j_detail_value) {
  if (j_native_ptr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          j_native_ptr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }
  std::string category =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, j_category);
  std::unique_ptr<std::unordered_map<std::string, std::string>> detail_ptr =
      nullptr;
  if (j_detail_key && j_detail_value) {
    std::string detail_key =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_key);
    std::string detail_value =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_value);
    detail_ptr =
        std::make_unique<std::unordered_map<std::string, std::string>>();
    detail_ptr->emplace(detail_key, detail_value);
  }
  nativeActorPtr->Act(
      [category = std::move(category), size_bytes = j_size_bytes,
       captured_detail = std::move(detail_ptr)](auto& performance) mutable {
        performance->GetMemoryMonitor().AllocateMemory(
            lynx::tasm::performance::MemoryRecord(category, size_bytes,
                                                  std::move(captured_detail)));
      });
}

static void DeallocateMemory(JNIEnv* env, jobject jcaller, jlong j_native_ptr,
                             jstring j_category, jlong j_size_bytes,
                             jstring j_detail_key, jstring j_detail_value) {
  if (j_native_ptr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          j_native_ptr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }
  std::string category =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, j_category);
  std::unique_ptr<std::unordered_map<std::string, std::string>> detail_ptr =
      nullptr;
  if (j_detail_key && j_detail_value) {
    std::string detail_key =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_key);
    std::string detail_value =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_value);
    detail_ptr =
        std::make_unique<std::unordered_map<std::string, std::string>>();
    detail_ptr->emplace(detail_key, detail_value);
  }
  nativeActorPtr->Act(
      [category = std::move(category), size_bytes = j_size_bytes,
       captured_detail = std::move(detail_ptr)](auto& performance) mutable {
        performance->GetMemoryMonitor().DeallocateMemory(
            lynx::tasm::performance::MemoryRecord(category, size_bytes,
                                                  std::move(captured_detail)));
      });
}

static void UpdateMemoryUsage(JNIEnv* env, jobject jcaller, jlong j_native_ptr,
                              jstring j_category, jlong j_size_bytes,
                              jint j_instance_count, jobject j_detail_map) {
  if (j_native_ptr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          j_native_ptr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }
  std::string category =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, j_category);
  std::unique_ptr<std::unordered_map<std::string, std::string>> detail_ptr =
      lynx::base::android::JNIConvertHelper::
          ConvertJavaStringHashMapToSTLStringMap(env, j_detail_map);
  nativeActorPtr->Act(
      [category = std::move(category), size_bytes = j_size_bytes,
       instance_count = j_instance_count,
       captured_detail = std::move(detail_ptr)](auto& performance) mutable {
        performance->GetMemoryMonitor().UpdateMemoryUsage(
            lynx::tasm::performance::MemoryRecord(category, size_bytes,
                                                  instance_count,
                                                  std::move(captured_detail)));
      });
}

static void SetTiming(JNIEnv* env, jobject jcaller, jlong j_native_ptr,
                      jstring key, jlong us_timestamp, jstring j_pipeline_id) {
  if (j_native_ptr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          j_native_ptr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }
  auto timing_key = static_cast<lynx::tasm::timing::TimestampKey>(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key));
  auto pipeline_id = static_cast<lynx::tasm::PipelineID>(
      lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                             j_pipeline_id));
  nativeActorPtr->Act([timing_key = std::move(timing_key), us_timestamp,
                       pipeline_id =
                           std::move(pipeline_id)](auto& controller) mutable {
    controller->GetTimingHandler().SetTiming(
        timing_key, static_cast<lynx::tasm::timing::TimestampUs>(us_timestamp),
        pipeline_id);
  });
}

static void SetPaintEndTimingAndHostPlatformTiming(
    JNIEnv* env, jobject jcaller, jlong j_native_ptr, jlong j_us_timestamp,
    jobject j_host_platform_timing_map, jobject j_pipeline_ids) {
  if (j_native_ptr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          j_native_ptr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }
  lynx::lepus::Value host_platform_timing_map_value;
  if (j_host_platform_timing_map) {
    host_platform_timing_map_value =
        lynx::tasm::android::ValueConverterAndroid::ConvertJavaOnlyMapToLepus(
            env, j_host_platform_timing_map);
  }

  lynx::lepus::Value pipeline_id_array_value;
  if (j_pipeline_ids) {
    pipeline_id_array_value =
        lynx::tasm::android::ValueConverterAndroid::ConvertJavaOnlyArrayToLepus(
            env, j_pipeline_ids);
  }

  nativeActorPtr->Act([j_us_timestamp,
                       host_platform_timing_map_value =
                           std::move(host_platform_timing_map_value),
                       pipeline_id_array_value = std::move(
                           pipeline_id_array_value)](auto& controller) mutable {
    lynx::tasm::ForEachLepusValue(
        pipeline_id_array_value,
        [&host_platform_timing_map_value, &controller, j_us_timestamp](
            const lynx::lepus::Value& key,
            const lynx::lepus::Value& pipeline_id_value) {
          auto pipeline_id = pipeline_id_value.StdString();
          // set host platform timing
          lynx::tasm::ForEachLepusValue(
              host_platform_timing_map_value,
              [&pipeline_id, &controller](
                  const lynx::lepus::Value& timing_key_value,
                  const lynx::lepus::Value& timestamp_value) {
                auto timing_key = timing_key_value.StdString();
                controller->GetTimingHandler().SetHostPlatformTiming(
                    timing_key, static_cast<uint64_t>(timestamp_value.Int64()),
                    pipeline_id);
              });
          // set paint end
          lynx::tasm::timing::TimestampKey paint_end_key(
              lynx::tasm::timing::kPaintEnd);
          controller->GetTimingHandler().SetTiming(
              paint_end_key,
              static_cast<lynx::tasm::timing::TimestampUs>(j_us_timestamp),
              pipeline_id);

          // The rendering pipeline is considerd to be complete after the
          // kPaintEnd phase, so we mark pipelineEnd here
          lynx::tasm::timing::TimestampKey pipeline_end_key(
              lynx::tasm::timing::kPipelineEnd);
          controller->GetTimingHandler().SetTiming(
              pipeline_end_key,
              static_cast<lynx::tasm::timing::TimestampUs>(j_us_timestamp),
              pipeline_id);
        });
  });
}

static void SetFSPTimingInfo(JNIEnv* env, jobject jcaller, jlong j_native_ptr,
                             jlong j_us_timestamp, jobject j_detail) {
  if (j_native_ptr == 0 || env == nullptr) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          j_native_ptr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }
  auto us_timestamp =
      static_cast<lynx::tasm::timing::TimestampUs>(j_us_timestamp);
  std::unique_ptr<std::unordered_map<std::string, std::string>> detail_ptr =
      lynx::base::android::JNIConvertHelper::
          ConvertJavaStringHashMapToSTLStringMap(env, j_detail);
  nativeActorPtr->Act([us_timestamp,
                       captured_detail_ptr =
                           std::move(detail_ptr)](auto& performance) mutable {
    // set fsp info
    if (captured_detail_ptr != nullptr) {
      auto& captured_detail = *captured_detail_ptr;
      for (auto const& [k, v] : captured_detail) {
        performance->GetTimingHandler().SetFSPInfo(k, v);
      }
    }
    // set fsp
    lynx::tasm::timing::TimestampKey timing_key(lynx::tasm::timing::kFSPEnd);
    lynx::tasm::PipelineID pipeline_id;
    performance->GetTimingHandler().SetTiming(timing_key, us_timestamp,
                                              pipeline_id);
  });
}

static jboolean IsMemoryMonitorEnabled(JNIEnv* env, jclass jcaller) {
  return lynx::tasm::performance::MemoryMonitor::Enable();
}

static jlong CurrentSystemTimeMicroseconds(JNIEnv* env, jclass jcaller) {
  return lynx::base::CurrentSystemTimeMicroseconds();
}

namespace lynx {
namespace jni {
bool RegisterJNIForPerformanceController(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace tasm {
namespace performance {

void PerformanceControllerAndroid::OnPerformanceEvent(
    const std::unique_ptr<pub::Value>& entry_map) {
  JNIEnv* jni_env = base::android::AttachCurrentThread();
  lepus::Value lepus_entry =
      pub::ValueUtils::ConvertValueToLepusValue(*entry_map);
  auto j_entry_map = lynx::tasm::android::ValueConverterAndroid::
      ConvertLepusToJavaOnlyMapForTiming(lepus_entry);
  Java_PerformanceController_onPerformanceEvent(
      jni_env, jni_platform_performance_.Get(), j_entry_map.jni_object());
}

void PerformanceControllerAndroid::SetActor(
    const std::shared_ptr<shell::LynxActor<PerformanceController>>& actor) {
  actor_ = actor;
  jlong ptr_num = reinterpret_cast<jlong>(this);
  PerformanceController::GetTaskRunner()->PostTask(
      [jni_platform_performance = jni_platform_performance_, ptr_num]() {
        JNIEnv* env = base::android::AttachCurrentThread();
        Java_PerformanceController_setNativePtr(
            env, jni_platform_performance.Get(), ptr_num);
      });
}

// Run on report thread.
PerformanceControllerAndroid::~PerformanceControllerAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PerformanceController_setNativePtr(env, jni_platform_performance_.Get(),
                                          0);
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
