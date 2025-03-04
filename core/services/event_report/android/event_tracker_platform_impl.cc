// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker_platform_impl.h"

#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/LynxEventReporter_jni.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/services/event_report/android/event_tracker_android.h"

void RunOnReportThread(JNIEnv* env, jobject jcaller, jobject runnable) {
  auto taskRunner =
      lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner();
  if (taskRunner->RunsTasksOnCurrentThread()) {
    Java_LynxEventReporter_callRunnable(env, runnable);
    return;
  }
  taskRunner->PostTask(
      [runnable =
           lynx::base::android::ScopedGlobalJavaRef<jobject>{env, runnable}]() {
        if (runnable.IsNull()) {
          return;
        }
        JNIEnv* env = lynx::base::android::AttachCurrentThread();
        Java_LynxEventReporter_callRunnable(env, runnable.Get());
      });
}

namespace lynx {
namespace tasm {
namespace report {

void RegisterJni(JNIEnv* env) { (void)RegisterNativesImpl(env); }

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
