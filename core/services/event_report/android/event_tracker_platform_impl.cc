// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker_platform_impl.h"

#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxEventReporter_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxEventReporter_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxEventReporter(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

void RunOnReportThread(JNIEnv* env, jobject jcaller, jobject runnable,
                       jlong delay_ms) {
  auto taskRunner =
      lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner();
  if (!taskRunner) {
    return;
  }
  auto task = [runnable = lynx::base::android::ScopedGlobalJavaRef<jobject>{
                   env, runnable}]() {
    if (runnable.IsNull()) {
      return;
    }
    JNIEnv* env = lynx::base::android::AttachCurrentThread();
    Java_LynxEventReporter_callRunnable(env, runnable.Get());
  };
  if (delay_ms > 0) {
    taskRunner->PostDelayedTask(std::move(task),
                                fml::TimeDelta::FromMilliseconds(delay_ms));
  } else {
    taskRunner->PostTask(std::move(task));
  }
}

namespace lynx {
namespace tasm {
namespace report {

static void DoReportEvent(JNIEnv* env, int32_t instance_id,
                          MoveOnlyEvent&& event) {
  LOGI("EventTracker onEvent with name: " << event.GetName());
  auto j_event_name = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
      env, event.GetName().c_str());
  base::android::JavaOnlyMap props;
  for (auto const& item : event.GetStringProps()) {
    props.PushString(item.first, item.second);
  }
  for (auto const& item : event.GetIntProps()) {
    props.PushInt(item.first, item.second);
  }
  for (auto const& item : event.GetDoubleProps()) {
    props.PushDouble(item.first, item.second);
  }
  Java_LynxEventReporter_onEvent(env, instance_id, j_event_name.Get(),
                                 props.jni_object());
}

void EventTrackerPlatformImpl::OnEvent(int32_t instance_id,
                                       MoveOnlyEvent&& event) {
  DoReportEvent(base::android::AttachCurrentThread(), instance_id,
                std::move(event));
}

void EventTrackerPlatformImpl::OnEvents(int32_t instance_id,
                                        std::vector<MoveOnlyEvent> stack) {
  JNIEnv* env = base::android::AttachCurrentThread();
  for (auto& event : stack) {
    DoReportEvent(env, instance_id, std::move(event));
  }
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string> generic_info) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::JavaOnlyMap java_config;
  for (auto const& item : generic_info) {
    java_config.PushString(item.first, item.second);
  }
  Java_LynxEventReporter_updateGenericInfo(env, instance_id,
                                           java_config.jni_object());
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, float> generic_info) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::JavaOnlyMap java_config;
  for (auto const& item : generic_info) {
    java_config.PushDouble(item.first, item.second);
  }
  Java_LynxEventReporter_updateGenericInfo(env, instance_id,
                                           java_config.jni_object());
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const std::string& value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::JavaOnlyMap java_config;
  java_config.PushString(key, value);
  Java_LynxEventReporter_updateGenericInfo(env, instance_id,
                                           java_config.jni_object());
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 int64_t value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::JavaOnlyMap java_config;
  java_config.PushInt64(key.c_str(), value);
  Java_LynxEventReporter_updateGenericInfo(env, instance_id,
                                           java_config.jni_object());
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const float value) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::JavaOnlyMap java_config;
  java_config.PushDouble(key, value);
  Java_LynxEventReporter_updateGenericInfo(env, instance_id,
                                           java_config.jni_object());
}

void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {
  // It is not implemented on Android yet, because the Java layer can directly
  // call LynxEventReporter.clearCache.
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
