// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/android/performance_controller_android.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "core/base/android/jni_helper.h"
#include "core/build/gen/PerformanceController_jni.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/services/performance/memory_monitor/memory_record.h"

static jlong CreateNativePerformanceActorPtr(JNIEnv* env, jobject jcaller) {
  auto* wrapper =
      new lynx::tasm::performance::PerformanceControllerActorWrapper();
  return reinterpret_cast<jlong>(wrapper);
}

static void ReleaseNativePerformanceActorPtr(JNIEnv* env, jobject jcaller,
                                             jlong nativePtr) {
  auto* wrapper = reinterpret_cast<
      lynx::tasm::performance::PerformanceControllerActorWrapper*>(nativePtr);
  delete wrapper;
}

static void AllocateMemory(JNIEnv* env, jobject jcaller, jlong nativePtr,
                           jstring j_category, jfloat j_sizeKb,
                           jstring j_detail_key, jstring j_detail_value) {
  if (nativePtr == 0) {
    return;
  }
  auto* wrapper = reinterpret_cast<
      lynx::tasm::performance::PerformanceControllerActorWrapper*>(nativePtr);
  auto& nativeActorPtr = wrapper->performance_controller_actor_;
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
    nativeActorPtr->ActAsync(
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
    nativeActorPtr->ActAsync(
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
  lynx::tasm::performance::PerformanceControllerActorWrapper* wrapper =
      reinterpret_cast<
          lynx::tasm::performance::PerformanceControllerActorWrapper*>(
          nativePtr);
  auto& nativeActorPtr = wrapper->performance_controller_actor_;
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
    nativeActorPtr->ActAsync(
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
    nativeActorPtr->ActAsync(
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
  lynx::tasm::performance::PerformanceControllerActorWrapper* wrapper =
      reinterpret_cast<
          lynx::tasm::performance::PerformanceControllerActorWrapper*>(
          nativePtr);
  auto& nativeActorPtr = wrapper->performance_controller_actor_;
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
    nativeActorPtr->ActAsync(
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
    nativeActorPtr->ActAsync(
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

namespace lynx {
namespace tasm {
namespace performance {

void PerformanceControllerAndroid::RegisterJNI(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

void PerformanceControllerAndroid::OnPerformanceEvent(
    const std::unique_ptr<lynx::pub::Value> entry) {
  JNIEnv* env = base::android::AttachCurrentThread();
  lepus::Value lepus_entry = pub::ValueUtils::ConvertValueToLepusValue(*entry);
  auto j_entry_map = lynx::tasm::android::ValueConverterAndroid::
      ConvertLepusToJavaOnlyMapForTiming(lepus_entry);
  Java_PerformanceController_onPerformanceEvent(
      env, jni_platform_performance_.Get(), j_entry_map.jni_object());
}

void PerformanceControllerAndroid::SetActor(
    const std::shared_ptr<lynx::shell::LynxActor<PerformanceController>>&
        actor) {
  JNIEnv* env = base::android::AttachCurrentThread();
  jlong ptr_num = Java_PerformanceController_getNativePtr(
      env, jni_platform_performance_.Get());
  if (ptr_num == 0) {
    return;
  }
  auto* wrapper = reinterpret_cast<
      lynx::tasm::performance::PerformanceControllerActorWrapper*>(ptr_num);
  wrapper->performance_controller_actor_ = actor;
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
