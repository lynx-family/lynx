// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACK_EVENT_WRAPPER_H_
#define BASE_TRACE_NATIVE_TRACK_EVENT_WRAPPER_H_

#include <memory>
#include <string>

#include "base/include/closure.h"
#include "base/trace/native/trace_export.h"

namespace perfetto {
namespace protos {
namespace pbzero {
class DebugAnnotation;
class TrackEvent_LegacyEvent;
}  // namespace pbzero
}  // namespace protos
class EventContext;
class CounterTrack;
}  // namespace perfetto

namespace lynx {
namespace perfetto {
class CounterTrack;
}  // namespace perfetto
namespace trace {
constexpr ::perfetto::CounterTrack ConvertToPerfCounterTrack(
    const lynx::perfetto::CounterTrack& counter_tarck);
}  // namespace trace
}  // namespace lynx

/**
 * Why use the lynx::perfetto namespace here?
 * Currently, lynx uses a large number of classes such as
 * lynx::perfetto::EventContext internally, and we want to keep the way that we
 * used it before, so leave the namespace of the associated class unchanged
 * here.
 */
namespace lynx {

namespace perfetto {

// At present, Track is not exposed to the public and is not recommended for
// use.
class Track {
 public:
  Track() = delete;
  Track(uint64_t id) : id_(id){};
  ~Track() = default;
  uint64_t id() const { return id_; }

 private:
  uint64_t id_;
};

class TRACE_EXPORT ThreadTrack {
 public:
  static uint64_t Current();
};

enum Unit : int32_t {
  UNIT_UNSPECIFIED = 0,
  UNIT_TIME_NS = 1,
  UNIT_COUNT = 2,
  UNIT_SIZE_BYTES = 3,
};

class TRACE_EXPORT CounterTrack {
 public:
  // |name| must be a string with static lifetime.
  constexpr explicit CounterTrack(const char* name)
      : name_(name), category_(nullptr) {}

  // |unit_name| is a free-form description of the unit used by this counter. It
  // must have static lifetime.
  constexpr CounterTrack(const char* name, const char* unit_name)
      : name_(name), category_(nullptr), unit_name_(unit_name) {}

  constexpr CounterTrack(const char* name, Unit unit)
      : name_(name), category_(nullptr), unit_(unit) {}

  constexpr CounterTrack(const char* name, const char* unit_name,
                         bool is_global)
      : name_(name),
        category_(nullptr),
        unit_name_(unit_name),
        is_global_(is_global) {}

  constexpr CounterTrack(const char* name, Unit unit, bool is_global)
      : name_(name), category_(nullptr), unit_(unit), is_global_(is_global) {}

  static constexpr CounterTrack Global(const char* name,
                                       const char* unit_name) {
    return CounterTrack(name, unit_name, true);
  }

  static constexpr CounterTrack Global(const char* name, Unit unit) {
    return CounterTrack(name, unit, true);
  }

  static constexpr CounterTrack Global(const char* name) {
    return Global(name, nullptr);
  }

  constexpr CounterTrack set_unit(Unit unit) const {
    return CounterTrack(name_, category_, unit, unit_name_, unit_multiplier_,
                        is_incremental_);
  }

  constexpr CounterTrack set_unit_name(const char* unit_name) const {
    return CounterTrack(name_, category_, unit_, unit_name, unit_multiplier_,
                        is_incremental_);
  }

  constexpr CounterTrack set_unit_multiplier(int64_t unit_multiplier) const {
    return CounterTrack(name_, category_, unit_, unit_name_, unit_multiplier,
                        is_incremental_);
  }

  constexpr CounterTrack set_category(const char* category) const {
    return CounterTrack(name_, category, unit_, unit_name_, unit_multiplier_,
                        is_incremental_);
  }

  constexpr CounterTrack set_incremental(bool is_incremental) const {
    return CounterTrack(name_, category_, unit_, unit_name_, unit_multiplier_,
                        is_incremental);
  }

 private:
  friend constexpr ::perfetto::CounterTrack
  lynx::trace::ConvertToPerfCounterTrack(const CounterTrack& counter_tarck);
  constexpr CounterTrack(const char* name, const char* category, Unit unit,
                         const char* unit_name, int64_t unit_multiplier,
                         bool is_incremental)
      : name_(name),
        category_(category),
        unit_(unit),
        unit_name_(unit_name),
        unit_multiplier_(unit_multiplier),
        is_incremental_(is_incremental) {}

  const char* const name_;
  const char* const category_;
  Unit unit_ = UNIT_UNSPECIFIED;
  const char* const unit_name_ = nullptr;
  int64_t unit_multiplier_ = 1;
  bool is_incremental_ = false;
  bool is_global_ = false;
};

class TRACE_EXPORT LynxDebugAnnotation {
 public:
  LynxDebugAnnotation(
      ::perfetto::protos::pbzero::DebugAnnotation* debug_annotation)
      : debug_annotation_(debug_annotation) {}
  ~LynxDebugAnnotation() = default;

  void set_name(const std::string& value);
  void set_bool_value(bool value);
  void set_uint_value(uint64_t value);
  void set_int_value(int64_t value);

  void set_double_value(double value);
  void set_string_value(const char* data, size_t size);
  void set_string_value(const std::string& value);

  void set_legacy_json_value(const std::string& value);

 private:
  LynxDebugAnnotation() = delete;
  ::perfetto::protos::pbzero::DebugAnnotation* debug_annotation_ = nullptr;
};

enum FlowDirection {
  FLOW_UNSPECIFIED = 0,
  FLOW_IN = 1,
  FLOW_OUT = 2,
  FLOW_INOUT = 3,
};

class TRACE_EXPORT TrackEvent_LegacyEvent {
 public:
  TrackEvent_LegacyEvent(
      ::perfetto::protos::pbzero::TrackEvent_LegacyEvent* legacy_event)
      : legacy_event_(legacy_event) {}
  ~TrackEvent_LegacyEvent() = default;

  void set_phase(int32_t value);
  void set_unscoped_id(uint64_t value);
  void set_bind_id(uint64_t value);
  void set_flow_direction(FlowDirection value);

 private:
  TrackEvent_LegacyEvent() = delete;
  ::perfetto::protos::pbzero::TrackEvent_LegacyEvent* legacy_event_ = nullptr;
};

class TRACE_EXPORT TrackEvent {
 public:
  enum TrackEvent_Type : int32_t {
    TrackEvent_Type_TYPE_UNSPECIFIED = 0,
    TrackEvent_Type_TYPE_SLICE_BEGIN = 1,
    TrackEvent_Type_TYPE_SLICE_END = 2,
    TrackEvent_Type_TYPE_INSTANT = 3,
    TrackEvent_Type_TYPE_COUNTER = 4,
  };
  TrackEvent() = delete;
  TrackEvent(::perfetto::EventContext* ctx) : ctx_(ctx){};
  ~TrackEvent() = default;

  void set_name(const std::string& value);
  void set_track_uuid(uint64_t value);
  void add_flow_ids(uint64_t value);
  void add_terminating_flow_ids(uint64_t value);
  LynxDebugAnnotation* add_debug_annotations();
  void add_debug_annotations(const std::string& name, const std::string& value);
  void add_debug_annotations(std::string&& name, std::string&& value);
  void set_timestamp_absolute_us(int64_t value);
  TrackEvent_LegacyEvent* set_legacy_event();

 private:
  ::perfetto::EventContext* ctx_ = nullptr;
  std::unique_ptr<LynxDebugAnnotation> lynx_debug_annotation_ = nullptr;
  std::unique_ptr<TrackEvent_LegacyEvent> legacy_event_ = nullptr;
};

class TRACE_EXPORT EventContext {
 public:
  EventContext(TrackEvent* event) : event_(event) {}
  TrackEvent* event() const { return event_; }

 private:
  TrackEvent* event_;
};
}  // namespace perfetto
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_TRACK_EVENT_WRAPPER_H_
