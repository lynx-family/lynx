// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>

#include "base/include/base_trace/trace_event_utils.h"
#include "base/include/log/logging.h"
#include "base/platform/android/src/main/jni/gen/LynxBaseTrace_jni.h"
#include "base/platform/android/src/main/jni/gen/LynxBaseTrace_register_jni.h"

namespace lynxbase {
namespace jni {
bool RegisterJNIForLynxBaseTrace(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynxbase

namespace lynx {
namespace base {
namespace trace {
using BaseTraceBeginSectionFunc =
    void (*)(const char* category_group, const char* section_name,
             int64_t trace_id, const char* arg1_name, const char* arg1_val,
             const char* arg2_name, const char* arg2_val);
using BaseTraceEndSectionFunc = void (*)(const char* category_group,
                                         const char* section_name,
                                         int64_t trace_id);

static BaseTraceBeginSectionFunc trace_begin_section_func = nullptr;
static BaseTraceEndSectionFunc trace_end_section_func = nullptr;

bool GetDefaultTraceBackend() {
  void* lib = dlopen("liblynxtrace.so", RTLD_LOCAL | RTLD_NOW);
  if (!lib) {
    LOGE("GetDefaultTraceBackend can't find liblynxtrace.so");
    return false;
  }

  trace_begin_section_func = reinterpret_cast<BaseTraceBeginSectionFunc>(
      dlsym(lib, "TraceEventBeginEx"));
  trace_end_section_func =
      reinterpret_cast<BaseTraceEndSectionFunc>(dlsym(lib, "TraceEventEndEx"));
  if (!trace_begin_section_func || !trace_end_section_func) {
    LOGE("TraceEventBeginEx TraceEventEndEx not found");
    return false;
  }
  return true;
}

void TraceBackend(const char* category, const char* name,
                  BaseTraceEventType phase) {
  switch (phase) {
    case BaseTraceEventType::TYPE_SLICE_BEGIN:
      trace_begin_section_func(category, name, -1, nullptr, nullptr, nullptr,
                               nullptr);
      break;
    case BaseTraceEventType::TYPE_SLICE_END:
      trace_end_section_func(category, name, -1);
      break;
    default:
      break;
  }
}

}  // namespace trace
}  // namespace base
}  // namespace lynx

void InitBaseTrace(JNIEnv* env, jclass jcaller, jlong addr) {
  auto trace_backend_ptr =
      reinterpret_cast<lynx::base::trace::trace_backend_ptr>(addr);
  if (!trace_backend_ptr && lynx::base::trace::GetDefaultTraceBackend()) {
    trace_backend_ptr = lynx::base::trace::TraceBackend;
    LOGV("base trace init success by dlopen liblynxtrace.so.");
  }
  lynx::base::trace::SetTraceBackend(trace_backend_ptr);
}
