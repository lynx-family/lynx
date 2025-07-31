// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/vsync_monitor_android.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "core/base/android/android_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/VSyncMonitor_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/VSyncMonitor_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForVSyncMonitor(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

void OnVSync(JNIEnv* env, jclass jcaller, jlong nativePtr,
             jlong frameStartTimeNS, jlong frameEndTimeNS) {
  auto* weak_ptr =
      reinterpret_cast<std::weak_ptr<lynx::base::VSyncMonitorAndroid>*>(
          nativePtr);
  auto shared_ptr = weak_ptr->lock();
  if (shared_ptr) {
    shared_ptr->OnVSync(frameStartTimeNS, frameEndTimeNS);
  }
  delete weak_ptr;
}

namespace lynx {
namespace base {

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  return std::make_shared<lynx::base::VSyncMonitorAndroid>(is_on_ui_thread);
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

void VSyncMonitorAndroid::RequestVSyncOnUIThread() {
  auto* weak_self = new std::weak_ptr<VSyncMonitor>(shared_from_this());
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VSyncMonitor_requestOnUIThread(env, reinterpret_cast<jlong>(weak_self));
}

}  // namespace base
}  // namespace lynx
