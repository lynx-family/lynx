// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/android/fsp_report_dispatcher_android.h"

#include <jni.h>

#include <atomic>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/base/android/android_jni.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "platform/android/lynx_android/src/main/jni/gen/FSPReportDispatcher_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/FSPReportDispatcher_register_jni.h"

namespace {

std::atomic<jint> g_active_dispatcher_global_ref_count{0};

class FSPReportDispatcherAndroid
    : public lynx::fml::RefCountedThreadSafe<FSPReportDispatcherAndroid> {
 public:
  FSPReportDispatcherAndroid(JNIEnv* env, jobject dispatcher)
      : dispatcher_(env, dispatcher) {
    if (!dispatcher_.IsNull()) {
      g_active_dispatcher_global_ref_count.fetch_add(1,
                                                     std::memory_order_relaxed);
    }
  }

  void DispatchOne() const {
    if (dispatcher_.IsNull()) {
      return;
    }
    JNIEnv* env = lynx::base::android::AttachCurrentThread();
    Java_FSPReportDispatcher_dispatchOne(env, dispatcher_.Get());
  }

 private:
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(FSPReportDispatcherAndroid);

  ~FSPReportDispatcherAndroid() {
    if (!dispatcher_.IsNull()) {
      dispatcher_.Reset();
      g_active_dispatcher_global_ref_count.fetch_sub(1,
                                                     std::memory_order_relaxed);
    }
  }

  lynx::base::android::ScopedGlobalJavaRef<jobject> dispatcher_;
};

}  // namespace

jlong Create(JNIEnv* env, jclass, jobject dispatcher) {
  if (dispatcher == nullptr) {
    return 0;
  }
  auto native_dispatcher =
      lynx::fml::AdoptRef(new FSPReportDispatcherAndroid(env, dispatcher));
  return reinterpret_cast<jlong>(native_dispatcher.AbandonRef());
}

jboolean Post(JNIEnv*, jclass, jlong native_ptr) {
  if (native_ptr == 0) {
    return JNI_FALSE;
  }
  auto task_runner =
      lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner();
  if (!task_runner) {
    return JNI_FALSE;
  }
  auto dispatcher =
      lynx::fml::Ref(reinterpret_cast<FSPReportDispatcherAndroid*>(native_ptr));
  task_runner->PostTask(
      [dispatcher = std::move(dispatcher)]() { dispatcher->DispatchOne(); });
  return JNI_TRUE;
}

void Destroy(JNIEnv*, jclass, jlong native_ptr) {
  reinterpret_cast<FSPReportDispatcherAndroid*>(native_ptr)->Release();
}

namespace lynx {
namespace tasm {
namespace report {

int GetFSPReportDispatcherGlobalRefCountForTesting() {
  return g_active_dispatcher_global_ref_count.load(std::memory_order_relaxed);
}

}  // namespace report
}  // namespace tasm
namespace jni {

bool RegisterJNIForFSPReportDispatcher(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx
