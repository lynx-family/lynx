// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/tracing/platform/instance_trace_plugin_darwin.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
namespace lynx {
namespace trace {

std::unique_ptr<InstanceCounterTrace::Impl> InstanceTracePluginDarwin::empty_counter_trace_ =
    std::make_unique<InstanceCounterTrace::Impl>();

InstanceTracePluginDarwin::InstanceTracePluginDarwin()
    : counter_trace_impl(std::make_unique<InstanceCounterTraceImpl>()) {}

InstanceTracePluginDarwin::~InstanceTracePluginDarwin() { counter_trace_impl.reset(nullptr); }

void InstanceTracePluginDarwin::DispatchBegin() {
  InstanceCounterTrace::SetImpl(counter_trace_impl.get());
}

void InstanceTracePluginDarwin::DispatchEnd() {
  InstanceCounterTrace::SetImpl(empty_counter_trace_.get());
}

std::string InstanceTracePluginDarwin::Name() { return "instance"; }

}  // namespace trace
}  // namespace lynx
#endif
