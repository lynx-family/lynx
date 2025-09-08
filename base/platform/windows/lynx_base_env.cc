// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/platform/windows/lynx_base_env.h"

#include "base/include/base_trace/trace_event_utils.h"
#include "base/include/log/logging_base.h"
#include "base/trace/native/trace_event.h"

namespace lynx {
namespace base {
namespace {
void trace_backend(const char* category, const char* name,
                   trace::BaseTraceEventType phase) {
  switch (phase) {
    case trace::BaseTraceEventType::TYPE_SLICE_BEGIN:
      TRACE_EVENT_BEGIN(category, name);
      break;
    case trace::BaseTraceEventType::TYPE_SLICE_END:
      TRACE_EVENT_END(category);
      break;
    case trace::BaseTraceEventType::TYPE_INSTANT:
      TRACE_EVENT_INSTANT(category, name);
      break;
    default:
      break;
  }
}

void initBaseTrace() { trace::SetTraceBackend(trace_backend); }
}  // namespace

LynxBaseEnv* LynxBaseEnv::Instance() {
  static LynxBaseEnv instance;
  return &instance;
}

void LynxBaseEnv::Init(bool is_print_log_to_all_channel) {
  lynx::InitLynxBaseLog(is_print_log_to_all_channel);
  initBaseTrace();
}

void LynxBaseEnv::OnlyInitBaseTrace() { initBaseTrace(); }

}  // namespace base
}  // namespace lynx
