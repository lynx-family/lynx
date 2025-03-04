// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_INSTANCE_COUNTER_TRACE_H_
#define BASE_TRACE_NATIVE_INSTANCE_COUNTER_TRACE_H_

#include <stdint.h>

#include "base/include/compiler_specific.h"
#include "base/trace/native/trace_export.h"

namespace lynx {
namespace trace {

class InstanceCounterTrace {
 public:
  class TRACE_EXPORT Impl {
   public:
    Impl() = default;

    virtual ~Impl() = default;

    virtual void JsHeapMemoryUsedTraceImpl(const uint64_t jsHeapMemory){};
  };

  InstanceCounterTrace() = delete;

  ~InstanceCounterTrace() = delete;

  TRACE_EXPORT static void SetImpl(Impl* impl) { impl_ = impl; }

  static void JsHeapMemoryUsedTrace(const uint64_t jsHeapMemory) {
    if (LIKELY(!impl_)) {
      return;
    }
    impl_->JsHeapMemoryUsedTraceImpl(jsHeapMemory);
  }

 private:
  TRACE_EXPORT static Impl* impl_;
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_INSTANCE_COUNTER_TRACE_H_
