// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/track_event_wrapper.h"

namespace perfetto {
namespace protos {
namespace pbzero {
class DebugAnnotation {};
}  // namespace pbzero
}  // namespace protos
class EventContext {};
class CounterTrack {};
}  // namespace perfetto

namespace lynx {
namespace perfetto {}  // namespace perfetto
namespace trace {
constexpr ::perfetto::CounterTrack ConvertToPerfCounterTrack(
    const lynx::perfetto::CounterTrack& counter_tarck) {
  return ::perfetto::CounterTrack();
}
}  // namespace trace
}  // namespace lynx

namespace lynx {
namespace perfetto {

uint64_t ThreadTrack::Current() { return 0; }

void LynxDebugAnnotation::set_name(const std::string& value) {}
void LynxDebugAnnotation::set_bool_value(bool value) {}
void LynxDebugAnnotation::set_uint_value(uint64_t value) {}
void LynxDebugAnnotation::set_int_value(int64_t value) {}
void LynxDebugAnnotation::set_double_value(double value) {}
void LynxDebugAnnotation::set_string_value(const char* data, size_t size) {}
void LynxDebugAnnotation::set_string_value(const std::string& value) {}

void LynxDebugAnnotation::set_legacy_json_value(const std::string& value) {}

void TrackEvent_LegacyEvent::set_phase(int32_t value) {}
void TrackEvent_LegacyEvent::set_unscoped_id(uint64_t value) {}
void TrackEvent_LegacyEvent::set_bind_id(uint64_t value) {}
void TrackEvent_LegacyEvent::set_flow_direction(FlowDirection value) {}

void TrackEvent::set_name(const std::string& value) {}
void TrackEvent::set_track_uuid(uint64_t value) {}
void TrackEvent::add_flow_ids(uint64_t value) {}
void TrackEvent::add_terminating_flow_ids(uint64_t value) {}
LynxDebugAnnotation* TrackEvent::add_debug_annotations() { return nullptr; }
void TrackEvent::add_debug_annotations(const std::string& name,
                                       const std::string& value) {}
void TrackEvent::add_debug_annotations(std::string&& name,
                                       std::string&& value) {}
void TrackEvent::set_timestamp_absolute_us(int64_t value) {}

TrackEvent_LegacyEvent* TrackEvent::set_legacy_event() { return nullptr; }

}  // namespace perfetto
}  // namespace lynx
