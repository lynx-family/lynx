// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/android/performance_controller_android.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

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
  // TODO: Migrate parameter conversion logic to report thread.
  lynx::tasm::performance::MemoryCategory category =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, j_category);
  if (j_detail_key && j_detail_value) {
    std::string detail_key =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_key);
    std::string detail_value =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_value);
    nativeActorPtr->Act(
        [category = std::move(category), sizeKb = j_sizeKb,
         detail_key = std::move(detail_key),
         detail_value = std::move(detail_key)](auto& performance) {
          auto detail =
              std::make_unique<std::unordered_map<std::string, std::string>>();
          detail->emplace(detail_key, detail_value);
          performance->GetMemoryMonitor().AllocateMemory(
              lynx::tasm::performance::MemoryRecord(category, sizeKb,
                                                    std::move(detail)));
        });
  } else {
    nativeActorPtr->Act(
        [category = std::move(category), sizeKb = j_sizeKb](auto& performance) {
          performance->GetMemoryMonitor().AllocateMemory(
              lynx::tasm::performance::MemoryRecord(category, sizeKb));
        });
  }
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
  // TODO: Migrate parameter conversion logic to report thread.
  std::string category =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, j_category);
  if (j_detail_key && j_detail_value) {
    std::string detail_key =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_key);
    std::string detail_value =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_value);
    nativeActorPtr->Act(
        [category = std::move(category), sizeKb = j_sizeKb,
         detail_key = std::move(detail_key),
         detail_value = std::move(detail_key)](auto& performance) {
          auto detail =
              std::make_unique<std::unordered_map<std::string, std::string>>();
          detail->emplace(detail_key, detail_value);
          performance->GetMemoryMonitor().DeallocateMemory(
              lynx::tasm::performance::MemoryRecord(category, sizeKb,
                                                    std::move(detail)));
        });
  } else {
    nativeActorPtr->Act(
        [category = std::move(category), sizeKb = j_sizeKb](auto& performance) {
          performance->GetMemoryMonitor().DeallocateMemory(
              lynx::tasm::performance::MemoryRecord(category, sizeKb));
        });
  }
}

static void UpdateMemoryUsage(JNIEnv* env, jobject jcaller, jlong nativePtr,
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
  if (j_detail_key || j_detail_value) {
    std::string detail_key =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_key);
    std::string detail_value =
        lynx::base::android::JNIConvertHelper::ConvertToString(env,
                                                               j_detail_value);
    nativeActorPtr->Act(
        [category = std::move(category), sizeKb = j_sizeKb,
         detail_key = std::move(detail_key),
         detail_value = std::move(detail_key)](auto& performance) {
          auto detail =
              std::make_unique<std::unordered_map<std::string, std::string>>();
          detail->emplace(detail_key, detail_value);
          performance->GetMemoryMonitor().UpdateMemoryUsage(
              lynx::tasm::performance::MemoryRecord(category, sizeKb,
                                                    std::move(detail)));
        });
  } else {
    nativeActorPtr->Act(
        [category = std::move(category), sizeKb = j_sizeKb](auto& performance) {
          performance->GetMemoryMonitor().UpdateMemoryUsage(
              lynx::tasm::performance::MemoryRecord(category, sizeKb));
        });
  }
}

static void SetForceEnable(JNIEnv* env, jobject jcaller, jlong nativePtr,
                           jboolean forceEnable) {
  lynx::tasm::performance::MemoryMonitor::SetForceEnable(forceEnable);
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

static void SetNeedPaintEndTiming(JNIEnv* env, jobject jcaller, jlong nativePtr,
                                  jstring pipelineID) {
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
  auto pipeline_id = static_cast<lynx::tasm::PipelineID>(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, pipelineID));
  nativeActorPtr->Act([pipeline_id](auto& controller) {
    controller->GetTimingHandler().SetNeedPaintEndTiming(pipeline_id);
  });
}

static void MarkPaintEndTimingIfNeeded(JNIEnv* env, jobject jcaller,
                                       jlong nativePtr, jlong usTimestamp) {
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
  nativeActorPtr->Act([usTimestamp](auto& controller) {
    controller->GetTimingHandler().SetPaintEndTimingIfNeeded(
        static_cast<lynx::tasm::timing::TimestampUs>(usTimestamp));
  });
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
