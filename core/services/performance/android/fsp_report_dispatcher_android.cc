// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/base/android/android_jni.h"
#include "core/services/performance/performance_controller.h"
#include "platform/android/lynx_android/src/main/jni/gen/FSPReportDispatcher_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/FSPReportDispatcher_register_jni.h"

namespace {

// Native lifetime anchor for the Java dispatcher.
//
// Java owns the initial native reference returned by Create() through
// mNativePtr. Every accepted Java snapshot posts one report-thread task, and
// each task temporarily owns one additional native reference. All of those
// native references point to this same object and therefore share its single
// JNI global reference. Consequently, Destroy() can run concurrently with
// already-posted tasks without invalidating the native object or its global
// reference. No snapshot is stored here; snapshots remain in the Java queue.
class FSPReportDispatcherAndroid
    : public lynx::fml::RefCountedThreadSafe<FSPReportDispatcherAndroid> {
 public:
  FSPReportDispatcherAndroid(JNIEnv* env, jobject dispatcher)
      : dispatcher_(env, dispatcher) {}

  void DispatchOne() const {
    if (dispatcher_.IsNull()) {
      return;
    }
    // Report-thread tasks are native callbacks, so attach the current thread
    // before calling Java. The Java side validates whether the
    // dispatcher/session is still open.
    JNIEnv* env = lynx::base::android::AttachCurrentThread();
    Java_FSPReportDispatcher_dispatchOne(env, dispatcher_.Get());
  }

 private:
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(FSPReportDispatcherAndroid);

  ~FSPReportDispatcherAndroid() = default;

  // This is the dispatcher's only JNI global reference. Posting more native
  // tasks increments the C++ object's ref count, but does not create more JNI
  // global references and does not directly retain individual Java snapshots.
  lynx::base::android::ScopedGlobalJavaRef<jobject> dispatcher_;
};

}  // namespace

jlong Create(JNIEnv* env, jclass, jobject dispatcher) {
  if (dispatcher == nullptr) {
    return 0;
  }
  // AdoptRef creates the initial owner. AbandonRef transfers that owner to Java
  // as an opaque pointer; Destroy() must release it exactly once.
  auto native_dispatcher =
      lynx::fml::AdoptRef(new FSPReportDispatcherAndroid(env, dispatcher));
  return reinterpret_cast<jlong>(native_dispatcher.AbandonRef());
}

jboolean Post(JNIEnv*, jclass, jlong native_ptr) {
  if (native_ptr == 0) {
    return JNI_FALSE;
  }
  auto task_runner =
      lynx::tasm::performance::PerformanceController::GetTaskRunner();
  if (!task_runner) {
    return JNI_FALSE;
  }
  // The task captures an additional native reference, not another Java
  // snapshot/Runnable global reference. Besides keeping the bridge alive if
  // Java closes it before the task executes, posting one task per snapshot
  // preserves ordering relative to timeout and other report-thread tasks.
  auto dispatcher =
      lynx::fml::Ref(reinterpret_cast<FSPReportDispatcherAndroid*>(native_ptr));
  task_runner->PostTask(
      [dispatcher = std::move(dispatcher)]() { dispatcher->DispatchOne(); });
  return JNI_TRUE;
}

void Destroy(JNIEnv*, jclass, jlong native_ptr) {
  if (native_ptr != 0) {
    // Release Java's owner. References captured by queued tasks, if any, keep
    // the object alive until those tasks finish; their callbacks become
    // harmless no-ops on the closed Java dispatcher.
    reinterpret_cast<FSPReportDispatcherAndroid*>(native_ptr)->Release();
  }
}

namespace lynx {
namespace jni {

bool RegisterJNIForFSPReportDispatcher(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx
