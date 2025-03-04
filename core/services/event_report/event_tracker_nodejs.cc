// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <map>

#include "base/trace/native/trace_event.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace tasm {
namespace report {

EventTracker* EventTracker::Instance() {
  static thread_local EventTracker instance_;
  return &instance_;
}

void EventTracker::OnEvent(EventBuilder builder) {
  EventTracker* instance = EventTracker::Instance();
  instance->tracker_event_builder_stack_.push_back(std::move(builder));
}

void EventTracker::UpdateGenericInfoByPageConfig(
    int32_t instance_id, const std::shared_ptr<tasm::PageConfig>& config) {}

void EventTracker::UpdateGenericInfo(int32_t instance_id, std::string key,
                                     std::string value) {}

void EventTracker::UpdateGenericInfo(int32_t instance_id, std::string key,
                                     float value) {}

void EventTracker::Flush(int32_t instance_id) {}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
