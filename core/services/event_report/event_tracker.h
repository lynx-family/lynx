// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_EVENT_REPORT_EVENT_TRACKER_H_
#define CORE_SERVICES_EVENT_REPORT_EVENT_TRACKER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "core/renderer/page_config.h"

namespace lynx {
namespace tasm {
namespace report {

// Instance ID is used to identify a LynxShell runtime environment. It can be
// used in event reporting to associate events with common parameter
// information. The ID is of type int32_t and is unique and incremented with
// each LynxShell creation during the app's runtime.
// Represents an unknown instance ID. Typically set proactively during event
// reporting, indicating that the current event does not need to distinguish the
// LynxShell runtime environment and does not need to associate common
// parameters.
static constexpr int32_t kUnknownInstanceId = -1;
// Represents an uninitialized instance ID. Used as an initial value, indicating
// that it needs to be automatically obtained by LynxActor::AfterInvoke.
static constexpr int32_t kUninitializedInstanceId = -2;

/// Event of reporting.
struct MoveOnlyEvent {
 public:
  /// Setter of event name.
  void SetName(const char* name) { name_ = name; }
  /// Getter of event name.
  const std::string& GetName() const { return name_; }

  void SetProps(const char* key, int value) { int_props_.insert({key, value}); }

  void SetProps(const char* key, unsigned int value) {
    double_props_.insert({key, value});
  }

  void SetProps(const char* key, uint64_t value) {
    double_props_.insert({key, value});
  }

  void SetProps(const char* key, int64_t value) {
    double_props_.insert({key, value});
  }

  void SetProps(const char* key, const char* value) {
    string_props_.insert({key, value});
  }

  void SetProps(const char* key, const std::string& value) {
    string_props_.insert({key, value});
  }

  void SetProps(const char* key, bool value) {
    int_props_.insert({key, value});
  }

  void SetProps(const char* key, double value) {
    double_props_.insert({key, value});
  }

  const std::unordered_map<std::string, std::string>& GetStringProps() const {
    return string_props_;
  }

  const std::unordered_map<std::string, int>& GetIntProps() const {
    return int_props_;
  }

  const std::unordered_map<std::string, double>& GetDoubleProps() const {
    return double_props_;
  }

  MoveOnlyEvent() = default;
  ~MoveOnlyEvent() = default;

  MoveOnlyEvent(MoveOnlyEvent&) = delete;
  MoveOnlyEvent(const MoveOnlyEvent&) = delete;
  MoveOnlyEvent& operator=(const MoveOnlyEvent&) = delete;

  MoveOnlyEvent(MoveOnlyEvent&& other)
      : name_(std::move(other.name_)),
        string_props_(std::move(other.string_props_)),
        int_props_(std::move(other.int_props_)),
        double_props_(std::move(other.double_props_)) {}

  MoveOnlyEvent& operator=(MoveOnlyEvent&& other) {
    name_ = std::move(other.name_);
    string_props_ = std::move(other.string_props_);
    int_props_ = std::move(other.int_props_);
    double_props_ = std::move(other.double_props_);
    return *this;
  }

 private:
  std::string name_;
  std::unordered_map<std::string, std::string> string_props_;
  std::unordered_map<std::string, int> int_props_;
  std::unordered_map<std::string, double> double_props_;
};

namespace test {
void GetEventParams(MoveOnlyEvent&, int);
}  // namespace test

//
// Tracker for event reporting.
//
// If you need to report events, you can use the report interface,like:
//  、、、
//      tasm::EventTracker::OnEvent([enable_user_bytecode=enable_user_bytecode_](MoveOnlyEvent&
//      event){ event.SetName("lynx_bytecode");
//      event.SetProps("use_new_bytecode", enable_user_bytecode);
//      event.SetProps("has_bytecode", false);
//      });
//  、、、
//
// In JS、layout、tasm、main thread, it has an instance of thread local. The
// 'Flush(T&)' method will pass all the events you report to the native facade,
// At the same time, it will carry common data about lynx view.
//
class EventTracker {
 public:
  using EventBuilder = base::MoveOnlyClosure<void, MoveOnlyEvent&>;
  /// Cache custom event to the event stack and upload them later.
  /// Can be called from any thread.
  /// @param builder Builder of event, Builder will be called on report
  /// kLynxReportEventName.
  static void OnEvent(EventBuilder builder);
  /// Update generic info of template instance by PageConfig.
  /// Can be called from any thread.
  /// @param instance_id  The unique id of template instance.
  /// @param config Page config of template instance.
  static void UpdateGenericInfoByPageConfig(
      int32_t instance_id, const std::shared_ptr<tasm::PageConfig>& config);
  /// Update the generic info of template instance.
  /// @param instance_id The unique id of template instance.
  /// @param key key of the generic info
  /// @param value string value of the generic info
  static void UpdateGenericInfo(int32_t instance_id, std::string key,
                                std::string value);
  /// Update the generic info of template instance.
  /// @param instance_id The unique id of template instance.
  /// @param key key of the generic info
  /// @param value double value of the generic info
  static void UpdateGenericInfo(int32_t instance_id, std::string key,
                                float value);
  /// Update the generic info of template instance.
  /// @param instance_id The unique id of template instance.
  /// @param key key of the generic info
  /// @param value int64_t value of the generic info
  static void UpdateGenericInfo(int32_t instance_id, std::string key,
                                int64_t value);
  /// Clear the cache, which includes extra parameters and generic info directly
  /// mapped by instance id.
  /// @param instance_id The unique id of template instance.
  static void ClearCache(int32_t instance_id);
  // Flush all `std::vector<EventBuilder>` to platform with
  // template instance id.
  static void Flush(int32_t instance_id);

 private:
  static EventTracker* Instance();

  EventTracker(){};
  EventTracker(const EventTracker& timing) = delete;
  EventTracker& operator=(const EventTracker&) = delete;
  EventTracker(EventTracker&&) = delete;
  EventTracker& operator=(EventTracker&&) = delete;

  std::vector<EventBuilder> tracker_event_builder_stack_;

  friend void test::GetEventParams(MoveOnlyEvent&, int);
};

}  // namespace report
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_EVENT_REPORT_EVENT_TRACKER_H_
