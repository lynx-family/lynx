// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/tracing/instance_counter_trace_impl.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace trace {

uint64_t InstanceCounterTraceImpl::node_count_ = 0;

InstanceCounterTrace::Impl* __attribute__((weak)) InstanceCounterTrace::impl_;

InstanceCounterTraceImpl::InstanceCounterTraceImpl()
    : thread_("CrRendererMain") {}

void InstanceCounterTraceImpl::JsHeapMemoryUsedTraceImpl(
    const uint64_t jsHeapMemory) {
#if ENABLE_TRACE_PERFETTO
  thread_.GetTaskRunner()->PostTask([jsHeapMemory] {
    TRACE_EVENT(LYNX_TRACE_CATEGORY_DEVTOOL_TIMELINE, "UpdateCounters",
                [=](lynx::perfetto::EventContext ctx) {
                  auto* legacy_event = ctx.event()->set_legacy_event();
                  legacy_event->set_phase('I');
                  legacy_event->set_unscoped_id(1);
                  auto* debug = ctx.event()->add_debug_annotations();
                  std::string data =
                      R"({"jsHeapSizeUsed":)" + std::to_string(jsHeapMemory);
                  data += R"(,"nodes":)" + std::to_string(node_count_) + "}";
                  debug->set_name("data");
                  debug->set_legacy_json_value(data);
                });
  });
#endif
}

void InstanceCounterTraceImpl::IncrementNodeCounter(tasm::Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  node_count_++;
  for (auto& i : element->GetChildren()) {
    IncrementNodeCounter(i);
  }
}

void InstanceCounterTraceImpl::DecrementNodeCounter(tasm::Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  node_count_--;
  for (auto& i : element->GetChildren()) {
    DecrementNodeCounter(i);
  }
}

void InstanceCounterTraceImpl::InitNodeCounter() { node_count_ = 0; }

}  // namespace trace
}  // namespace lynx
#endif
