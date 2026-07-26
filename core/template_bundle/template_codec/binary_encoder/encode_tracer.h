// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODE_TRACER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODE_TRACER_H_

#include <chrono>
#include <string>
#include <vector>

namespace lynx {
namespace tasm {

class EncodeTracer {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  class ScopedEvent {
   public:
    ScopedEvent(EncodeTracer* tracer, const char* name);

    ScopedEvent(const ScopedEvent&) = delete;
    ScopedEvent& operator=(const ScopedEvent&) = delete;

    ~ScopedEvent();

   private:
    EncodeTracer* tracer_;
    const char* name_;
    TimePoint start_;
  };

  EncodeTracer() = default;

  std::string Finish();

 private:
  struct Event {
    const char* name;
    double duration_us;
  };

  void AddEvent(const char* name, TimePoint start);

  TimePoint encode_start_{Clock::now()};
  std::vector<Event> events_;
};

}  // namespace tasm
}  // namespace lynx

#define TASM_ENCODE_TRACE_SCOPE(tracer, name)                                 \
  ::lynx::tasm::EncodeTracer::ScopedEvent encode_trace_event_##name((tracer), \
                                                                    #name)

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODE_TRACER_H_
