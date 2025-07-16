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

static void AllocateMemory(JNIEnv* env, jobject jcaller, jlong nativePtr,
                           jstring j_category, jfloat j_sizeKb,
                           jstring j_detail_key, jstring j_detail_value) {
  if (nativePtr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          nativePtr);
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
      [category = std::move(category), sizeKb = j_sizeKb,
       captured_detail = std::move(detail_ptr)](auto& performance) mutable {
        performance->GetMemoryMonitor().AllocateMemory(
            lynx::tasm::performance::MemoryRecord(category, sizeKb,
                                                  std::move(captured_detail)));
      });
}

static void DeallocateMemory(JNIEnv* env, jobject jcaller, jlong nativePtr,
                             jstring j_category, jfloat j_sizeKb,
                             jstring j_detail_key, jstring j_detail_value) {
  if (nativePtr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          nativePtr);
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
      [category = std::move(category), sizeKb = j_sizeKb,
       captured_detail = std::move(detail_ptr)](auto& performance) mutable {
        performance->GetMemoryMonitor().DeallocateMemory(
            lynx::tasm::performance::MemoryRecord(category, sizeKb,
                                                  std::move(captured_detail)));
      });
}

static void UpdateMemoryUsage(JNIEnv* env, jobject jcaller, jlong nativePtr,
                              jstring j_category, jfloat j_sizeKb,
                              jint j_instanceCount, jobject j_detail_map) {
  if (nativePtr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          nativePtr);
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
      [category = std::move(category), size_kb = j_sizeKb,
       instance_count = j_instanceCount,
       captured_detail = std::move(detail_ptr)](auto& performance) mutable {
        performance->GetMemoryMonitor().UpdateMemoryUsage(
            lynx::tasm::performance::MemoryRecord(
                category, size_kb, instance_count, std::move(captured_detail)));
      });
}

static void SetTiming(JNIEnv* env, jobject jcaller, jlong nativePtr,
                      jstring key, jlong usTimestamp, jstring pipelineID) {
  if (nativePtr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          nativePtr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }
  auto timing_key = static_cast<lynx::tasm::timing::TimestampKey>(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, key));
  auto pipeline_id = static_cast<lynx::tasm::PipelineID>(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, pipelineID));
  nativeActorPtr->Act([timing_key = std::move(timing_key), usTimestamp,
                       pipeline_id =
                           std::move(pipeline_id)](auto& controller) mutable {
    controller->GetTimingHandler().SetTiming(
        timing_key, static_cast<lynx::tasm::timing::TimestampUs>(usTimestamp),
        pipeline_id);
  });
}

static void SetPaintEndTiming(JNIEnv* env, jobject jcaller, jlong nativePtr,
                              jlong j_us_timestamp, jobject j_pipeline_ids) {
  if (nativePtr == 0) {
    return;
  }
  auto* wrapper =
      reinterpret_cast<lynx::tasm::performance::PerformanceControllerAndroid*>(
          nativePtr);
  auto& nativeActorPtr = wrapper->GetActor();
  if (!nativeActorPtr) {
    return;
  }

  lynx::lepus::Value pipeline_id_array_value;
  if (j_pipeline_ids) {
    pipeline_id_array_value =
        lynx::tasm::android::ValueConverterAndroid::ConvertJavaOnlyArrayToLepus(
            env, j_pipeline_ids);
  }

  nativeActorPtr->Act([j_us_timestamp, pipeline_id_array_value =
                                           std::move(pipeline_id_array_value)](
                          auto& controller) mutable {
    lynx::tasm::ForEachLepusValue(
        pipeline_id_array_value,
        [&controller, j_us_timestamp](
            const lynx::lepus::Value& key,
            const lynx::lepus::Value& pipeline_id_value) {
          auto pipeline_id = pipeline_id_value.StdString();
          // set paint end
          lynx::tasm::timing::TimestampKey paint_end_key(
              lynx::tasm::timing::kPaintEnd);
          controller->GetTimingHandler().SetTiming(
              paint_end_key,
              static_cast<lynx::tasm::timing::TimestampUs>(j_us_timestamp),
              pipeline_id);
        });
  });
}

static jboolean IsMemoryMonitorEnabled(JNIEnv* env, jclass jcaller) {
  return lynx::tasm::performance::MemoryMonitor::Enable();
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
