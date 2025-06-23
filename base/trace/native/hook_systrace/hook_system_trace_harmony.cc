// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/include/log/logging.h"
#include "base/trace/native/hook_systrace/hook_system_trace.h"
#include "base/trace/native/trace_event.h"
#include "third_party/xhook/libxhook/jni/xhook.h"

namespace lynx {
namespace trace {

using ATraceFunc = struct {
  const char *name;
  void *local_func;
  void *real_func;
};

static void HiTraceBeginBody(uint64_t label, const std::string &name,
                             __attribute__((unused)) float limit = -1) {
  TRACE_EVENT_BEGIN(INTERNAL_TRACE_CATEGORY_ATRACE, nullptr,
                    [&name, &label](lynx::perfetto::EventContext ctx) {
                      ctx.event()->set_name(name);
                      ctx.event()->add_debug_annotations("label",
                                                         std::to_string(label));
                    });
}
static void HiTraceEndBody(uint64_t label) {
  TRACE_EVENT_END(INTERNAL_TRACE_CATEGORY_ATRACE,
                  [&label](lynx::perfetto::EventContext ctx) {
                    ctx.event()->add_debug_annotations("label",
                                                       std::to_string(label));
                  });
}

static void HiTraceBeginBodyAsync(uint64_t label, const std::string &name,
                                  int32_t taskId,
                                  __attribute__((unused)) float limit = -1) {
  TRACE_EVENT(INTERNAL_TRACE_CATEGORY_ATRACE, nullptr,
              [&label, &name, &taskId](lynx::perfetto::EventContext ctx) {
                ctx.event()->set_name(name);
                ctx.event()->add_debug_annotations("label",
                                                   std::to_string(label));
                auto legacy_event = ctx.event()->set_legacy_event();
                legacy_event->set_phase('S');
                legacy_event->set_bind_id(taskId);
                legacy_event->set_unscoped_id(taskId);
                legacy_event->set_flow_direction(
                    lynx::perfetto::FlowDirection::FLOW_IN);
              });
}
static void HiTraceEndBodyAsync(uint64_t label, const std::string &name,
                                int32_t taskId) {
  TRACE_EVENT(INTERNAL_TRACE_CATEGORY_ATRACE, nullptr,
              [&label, &name, &taskId](lynx::perfetto::EventContext ctx) {
                ctx.event()->set_name(name);
                ctx.event()->add_debug_annotations("label",
                                                   std::to_string(label));
                auto legacy_event = ctx.event()->set_legacy_event();
                legacy_event->set_phase('F');
                legacy_event->set_bind_id(taskId);
                legacy_event->set_unscoped_id(taskId);
                legacy_event->set_flow_direction(
                    lynx::perfetto::FlowDirection::FLOW_OUT);
              });
}
static bool HiTraceIsTagEnabled(__attribute__((unused)) uint64_t tag) {
  return true;
}

__attribute__((unused)) static ATraceFunc atrace_funcs[] = {
    {"StartTrace", reinterpret_cast<void *>(HiTraceBeginBody), nullptr},
    {"FinishTrace", reinterpret_cast<void *>(HiTraceEndBody), nullptr},
    {"IsTagEnabled", reinterpret_cast<void *>(HiTraceIsTagEnabled), nullptr},
    {"StartAsyncTrace", reinterpret_cast<void *>(HiTraceBeginBodyAsync),
     nullptr},
    {"FinishAsyncTrace", reinterpret_cast<void *>(HiTraceEndBodyAsync),
     nullptr},
};

void HookSystemTrace::InstallSystemTraceHooks() {
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
