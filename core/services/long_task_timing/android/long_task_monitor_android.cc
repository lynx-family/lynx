// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/long_task_timing/android/long_task_monitor_android.h"

#include <string>

#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/LynxLongTaskMonitor_jni.h"
#include "core/services/long_task_timing/long_task_monitor.h"

static void WillProcessTask(JNIEnv* env, jclass jcaller, jstring jType,
                            jstring jName, jstring jTaskInfo,
                            jint jInstanceId) {
  std::string type =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, jType);
  std::string name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, jName);
  std::string taskInfo =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, jTaskInfo);
  int32_t instanceId = static_cast<int32_t>(jInstanceId);
  lynx::tasm::timing::LongTaskMonitor::Instance()->WillProcessTask(
      type, name, taskInfo, instanceId);
}

static void UpdateLongTaskTimingIfNeed(JNIEnv* env, jclass jcaller,
                                       jstring jType, jstring jName,
                                       jstring jTaskInfo) {
  lynx::tasm::timing::LongTaskTiming* timing =
      lynx::tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
  if (timing == nullptr) {
    return;
  }

  if (jType != nullptr) {
    timing->task_type_ =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, jType);
  }

  if (jName != nullptr) {
    timing->task_name_ =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, jName);
  }

  if (jTaskInfo != nullptr) {
    timing->task_info_ =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, jTaskInfo);
  }
}

static void DidProcessTask(JNIEnv* env, jclass jcaller) {
  lynx::tasm::timing::LongTaskMonitor::Instance()->DidProcessTask();
}

namespace lynx {
namespace tasm {
namespace timing {

void RegisterJniLongTaskMonitor(JNIEnv* env) { (void)RegisterNativesImpl(env); }

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
