// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/track_event_wrapper.h"

#include "third_party/perfetto/perfetto.h"

namespace lynx {
namespace perfetto {

uint64_t ThreadTrack::Current() {
  return ::perfetto::ThreadTrack::Current().uuid;
}

void LynxDebugAnnotation::set_name(const std::string& value) {
  return debug_annotation_->set_name(value);
}
void LynxDebugAnnotation::set_bool_value(bool value) {
  return debug_annotation_->set_bool_value(value);
}
void LynxDebugAnnotation::set_uint_value(uint64_t value) {
  return debug_annotation_->set_uint_value(value);
}
void LynxDebugAnnotation::set_int_value(int64_t value) {
  return debug_annotation_->set_int_value(value);
}
void LynxDebugAnnotation::set_double_value(double value) {
  return debug_annotation_->set_double_value(value);
}
void LynxDebugAnnotation::set_string_value(const char* data, size_t size) {
  return debug_annotation_->set_string_value(data, size);
}
void LynxDebugAnnotation::set_string_value(const std::string& value) {
  return debug_annotation_->set_string_value(value);
}

void LynxDebugAnnotation::set_legacy_json_value(const std::string& value) {
  return debug_annotation_->set_legacy_json_value(value);
}

void TrackEvent_LegacyEvent::set_phase(int32_t value) {
  legacy_event_->set_phase(value);
}
void TrackEvent_LegacyEvent::set_unscoped_id(uint64_t value) {
  legacy_event_->set_unscoped_id(value);
}

void TrackEvent_LegacyEvent::set_bind_id(uint64_t value) {
  legacy_event_->set_bind_id(value);
}

void TrackEvent_LegacyEvent::set_flow_direction(FlowDirection value) {
  legacy_event_->set_flow_direction(
      static_cast<
          ::perfetto::protos::pbzero::TrackEvent_LegacyEvent_FlowDirection>(
          value));
}

void TrackEvent::set_name(const std::string& value) {
  ctx_->event()->set_name(value);
}
void TrackEvent::set_track_uuid(uint64_t value) {
  ctx_->event()->set_track_uuid(value);
}
void TrackEvent::add_flow_ids(uint64_t value) {
  ctx_->event()->add_flow_ids(value);
}
void TrackEvent::add_terminating_flow_ids(uint64_t value) {
  ctx_->event()->add_terminating_flow_ids(value);
}
LynxDebugAnnotation* TrackEvent::add_debug_annotations() {
  auto perfetto_debug_annotation = ctx_->event()->add_debug_annotations();
  lynx_debug_annotation_ =
      std::make_unique<LynxDebugAnnotation>(perfetto_debug_annotation);
  return lynx_debug_annotation_.get();
}
void TrackEvent::add_debug_annotations(const std::string& name,
                                       const std::string& value) {
  ctx_->event()->add_debug_annotations(name, value);
}
void TrackEvent::add_debug_annotations(std::string&& name,
                                       std::string&& value) {
  ctx_->event()->add_debug_annotations(name, value);
}
void TrackEvent::set_timestamp_absolute_us(int64_t value) {
  ctx_->event()->set_timestamp_absolute_us(value);
}

TrackEvent_LegacyEvent* TrackEvent::set_legacy_event() {
  auto legacy_event = ctx_->event()->set_legacy_event();
  legacy_event_ = std::make_unique<TrackEvent_LegacyEvent>(legacy_event);
  return legacy_event_.get();
}

}  // namespace perfetto
}  // namespace lynx
