// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
#include "base/trace/native/hook_systrace/hook_system_trace.h"
#include "base/trace/native/trace_event.h"
#include "third_party/xhook/libxhook/jni/xhook.h"

// Keep these in sync with system/core/libcutils/include/cutils/trace.h in
// android source code.
#define ATRACE_TAG_NEVER 0          // This tag is never enabled.
#define ATRACE_TAG_ALWAYS (1 << 0)  // This tag is always enabled.
#define ATRACE_TAG_GRAPHICS (1 << 1)
#define ATRACE_TAG_INPUT (1 << 2)
#define ATRACE_TAG_VIEW (1 << 3)
#define ATRACE_TAG_WEBVIEW (1 << 4)
#define ATRACE_TAG_WINDOW_MANAGER (1 << 5)
#define ATRACE_TAG_ACTIVITY_MANAGER (1 << 6)
#define ATRACE_TAG_SYNC_MANAGER (1 << 7)
#define ATRACE_TAG_AUDIO (1 << 8)
#define ATRACE_TAG_VIDEO (1 << 9)
#define ATRACE_TAG_CAMERA (1 << 10)
#define ATRACE_TAG_HAL (1 << 11)
#define ATRACE_TAG_APP (1 << 12)
#define ATRACE_TAG_RESOURCES (1 << 13)
#define ATRACE_TAG_DALVIK (1 << 14)
#define ATRACE_TAG_RS (1 << 15)
#define ATRACE_TAG_BIONIC (1 << 16)
#define ATRACE_TAG_POWER (1 << 17)
#define ATRACE_TAG_PACKAGE_MANAGER (1 << 18)
#define ATRACE_TAG_SYSTEM_SERVER (1 << 19)
#define ATRACE_TAG_DATABASE (1 << 20)
#define ATRACE_TAG_NETWORK (1 << 21)
#define ATRACE_TAG_ADB (1 << 22)
#define ATRACE_TAG_VIBRATOR (1 << 23)
#define ATRACE_TAG_AIDL (1 << 24)
#define ATRACE_TAG_NNAPI (1 << 25)
#define ATRACE_TAG_RRO (1 << 26)
#define ATRACE_TAG_LAST ATRACE_TAG_RRO
#define ATRACE_TAG_ALL ~((uint64_t)(-1) << 27)

namespace lynx {
namespace trace {

using ATraceFunc = struct {
  const char *name;
  void *local_func;
  void *real_func;
};

static void ATraceBeginBody(const char *name) {
  TraceEventBegin(INTERNAL_TRACE_CATEGORY_ATRACE, nullptr,
                  [name](lynx::perfetto::EventContext ctx) {
                    ctx.event()->set_name(name);
                  });
}

static void ATraceEndBody() { TraceEventEnd(INTERNAL_TRACE_CATEGORY_ATRACE); }

static uint64_t ATraceGetEnabledTags() { return ATRACE_TAG_ALL; }

static void ATraceUpdateTags() {}

static uint64_t ATraceGetProperty() { return ATRACE_TAG_ALL; }

static uint64_t s_atrace_enabled_tags = ATRACE_TAG_ALL;

static ATraceFunc atrace_funcs[] = {
    {"atrace_begin_body", reinterpret_cast<void *>(ATraceBeginBody), nullptr},
    {"atrace_end_body", reinterpret_cast<void *>(ATraceEndBody), nullptr},
    {"atrace_update_tags", reinterpret_cast<void *>(&ATraceUpdateTags),
     nullptr},
    {"atrace_get_property", reinterpret_cast<void *>(&ATraceGetProperty),
     nullptr},
    {"atrace_enabled_tags", reinterpret_cast<void *>(&s_atrace_enabled_tags),
     nullptr},
    {"atrace_get_enabled_tags", reinterpret_cast<void *>(ATraceGetEnabledTags),
     nullptr}};

void HookSystemTrace::InstallSystemTraceHooks() {
  // may be modified by other thread, reset to default value
  s_atrace_enabled_tags = ATRACE_TAG_ALL;
  xhook_clear();
  for (auto &func : atrace_funcs) {
    int ret =
        xhook_register(".*\\.so$", func.name, func.local_func, &func.real_func);
    if (ret != 0) {
      LOGE("failed to hook symbol:" << func.name << " ret " << ret);
    }
  }
  xhook_refresh(0);
}

void HookSystemTrace::UninstallSystemTraceHooks() {
  xhook_clear();
  for (auto func : atrace_funcs) {
    if (func.real_func == nullptr) {
      continue;
    }
    int ret = xhook_register(".*\\.so$", func.name, func.real_func, nullptr);
    if (ret != 0) {
      LOGE("failed to uninstall symbol:" << func.name << " ret " << ret);
    }
  }
  xhook_refresh(0);
}

void HookSystemTrace::Install() {
  InstallSystemTraceHooks();
  cpu_info_trace_.DispatchBegin();
}

void HookSystemTrace::Uninstall() {
  UninstallSystemTraceHooks();
  cpu_info_trace_.DispatchEnd();
}

}  // namespace trace
}  // namespace lynx
