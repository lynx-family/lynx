// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_TRACING_INSTANCE_COUNTER_TRACE_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_TRACING_INSTANCE_COUNTER_TRACE_IMPL_H_

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include <stdint.h>

#include "base/include/fml/thread.h"
#include "base/trace/native/instance_counter_trace.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace trace {

class BASE_EXPORT_FOR_DEVTOOL InstanceCounterTraceImpl
    : public InstanceCounterTrace::Impl {
 public:
  InstanceCounterTraceImpl();

  virtual ~InstanceCounterTraceImpl() = default;

  virtual void JsHeapMemoryUsedTraceImpl(const uint64_t jsHeapMemory) override;

  static void IncrementNodeCounter(tasm::Element* element);

  static void DecrementNodeCounter(tasm::Element* element);

  static void InitNodeCounter();

 private:
  fml::Thread thread_;
  static uint64_t node_count_;
};

}  // namespace trace
}  // namespace lynx
#endif
#endif  // DEVTOOL_LYNX_DEVTOOL_TRACING_INSTANCE_COUNTER_TRACE_IMPL_H_
