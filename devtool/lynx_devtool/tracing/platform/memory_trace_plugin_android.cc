// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "devtool/lynx_devtool/tracing/platform/memory_trace_plugin_android.h"

#include <chrono>
#include <thread>

#include "base/include/fml/message_loop.h"
#include "base/trace/native/trace_defines.h"
#include "core/base/android/jni_helper.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/MemoryTrace_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/MemoryTrace_register_jni.h"

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForMemoryTrace(JNIEnv* env) { return RegisterNativesImpl(env); }

}  // namespace jni
}  // namespace devtool
}  // namespace lynx

static jlong CreateMemoryTrace(JNIEnv* env, jobject jcaller) {
  auto* trace_plugin = new lynx::trace::MemoryTracePluginAndroid(env, jcaller);
  return reinterpret_cast<jlong>(trace_plugin);
}

namespace lynx {
namespace trace {

MemoryTracePluginAndroid::MemoryTracePluginAndroid(JNIEnv* env, jobject owner)
    : weak_owner_(env, owner),
      notification_callback_(
          LYNX_TRACE_MEMORY_PLUGIN_GC_NOTIFICATION,
          [&](const std::string& tag, [[maybe_unused]] intptr_t data) {
            auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
            {
              std::lock_guard<std::mutex> lock(gc_task_mutex_);
              if (gc_task_scheduled_) {
                return;
              }
              gc_task_scheduled_ = true;
            }
            task_runner->PostDelayedTask(
                [this]() {
                  this->RunGC();
                  {
                    std::lock_guard<std::mutex> lock(gc_task_mutex_);
                    gc_task_scheduled_ = false;
                  }
                },
                fml::TimeDelta::FromMilliseconds(100));
          }) {}

void MemoryTracePluginAndroid::DispatchSetup(
    const std::shared_ptr<TraceConfig>& config) {
  force_gc_ = config->memory_trace_force_gc;
  if (force_gc_) {
    RunGC();
  }
}

void MemoryTracePluginAndroid::DispatchEnd() {
  if (force_gc_) {
    RunGC();
  }
}

std::string MemoryTracePluginAndroid::Name() { return "Memory"; }

void MemoryTracePluginAndroid::RunGC() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_MemoryTrace_trimMemory(env, weak_owner_.Get());
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

}  // namespace trace
}  // namespace lynx
#endif
