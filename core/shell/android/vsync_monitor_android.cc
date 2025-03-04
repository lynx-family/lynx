// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/vsync_monitor_android.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "core/base/android/android_jni.h"
#include "core/build/gen/VSyncMonitor_jni.h"

void OnVSync(JNIEnv* env, jclass jcaller, jlong nativePtr,
             jlong frameStartTimeNS, jlong frameEndTimeNS) {
  auto* weak_ptr =
      reinterpret_cast<std::weak_ptr<lynx::shell::VSyncMonitorAndroid>*>(
          nativePtr);
  auto shared_ptr = weak_ptr->lock();
  if (shared_ptr) {
    shared_ptr->OnVSync(frameStartTimeNS, frameEndTimeNS);
  }
  delete weak_ptr;
}

namespace lynx {
namespace shell {

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create() {
  return std::make_shared<lynx::shell::VSyncMonitorAndroid>();
}

// static
bool VSyncMonitorAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

VSyncMonitorAndroid::VSyncMonitorAndroid()
    : is_vsync_triggered_in_ui_thread_(
          tasm::LynxEnv::GetInstance().IsVSyncTriggeredInUiThreadAndroid()) {
  LOGI("VSyncMonitorAndroid Created with is_vsync_triggered_in_ui_thread_ "
       << is_vsync_triggered_in_ui_thread_);
}

void VSyncMonitorAndroid::RequestVSync() {
  auto* weak_self = new std::weak_ptr<VSyncMonitor>(shared_from_this());
  JNIEnv* env = base::android::AttachCurrentThread();
  if (is_vsync_triggered_in_ui_thread_) {
    Java_VSyncMonitor_requestOnUIThread(env,
                                        reinterpret_cast<jlong>(weak_self));
  } else {
    Java_VSyncMonitor_request(env, reinterpret_cast<jlong>(weak_self));
  }
}

void VSyncMonitorAndroid::RequestVSyncOnUIThread(Callback callback) {
  if (callback_) {
    // request during a frame interval, just return
    return;
  }

  callback_ = std::move(callback);

  auto* weak_self = new std::weak_ptr<VSyncMonitor>(shared_from_this());
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VSyncMonitor_requestOnUIThread(env, reinterpret_cast<jlong>(weak_self));
}
}  // namespace shell
}  // namespace lynx
