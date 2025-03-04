// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/trace_event_utils_perfetto.h"

namespace lynx {
namespace trace {
uint64_t GetFlowId() { return 0; }
void TraceEventImplementation(const char* category_name, const char* name,
                              TraceEventType phase,
                              const lynx::perfetto::Track* track_id,
                              const uint64_t& timestamp,
                              const FuncType& callback) {}
void TraceEventImplementation(const char* category_name,
                              const std::string& name, TraceEventType phase,
                              const lynx::perfetto::Track* track_id,
                              const uint64_t& timestamp,
                              const FuncType& callback) {}
void TraceEventImplementation(const char* category_name,
                              const lynx::perfetto::CounterTrack& counter_track,
                              TraceEventType phase, const uint64_t& timestamp,
                              const uint64_t& counter) {}
bool TraceEventCategoryEnabled(const char* category) { return false; }
void TraceRuntimeProfile(const std::string& runtime_profile,
                         const uint64_t track_id, const int32_t profile_id) {}

}  // namespace trace
}  // namespace lynx
