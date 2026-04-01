// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/event_tracker_proxy_impl.h"

#include <utility>

#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace shell {
namespace {

void PopulateMoveOnlyEvent(const EventTrackerEvent& source,
                           tasm::report::MoveOnlyEvent& target) {
  if (!source.name.empty()) {
    target.SetName(source.name.c_str());
  }
  for (const auto& [key, value] : source.string_props) {
    target.SetProps(key.c_str(), value);
  }
  for (const auto& [key, value] : source.int_props) {
    target.SetProps(key.c_str(), value);
  }
  for (const auto& [key, value] : source.double_props) {
    target.SetProps(key.c_str(), value);
  }
}

}  // namespace

void EventTrackerProxyImpl::OnEvent(EventBuilder builder) {
  if (!builder) {
    return;
  }
  tasm::report::EventTracker::OnEvent(
      [builder =
           std::move(builder)](tasm::report::MoveOnlyEvent& event) mutable {
        EventTrackerEvent public_event;
        builder(public_event);
        PopulateMoveOnlyEvent(public_event, event);
      });
}

void EventTrackerProxyImpl::UpdateGenericInfo(int32_t instance_id,
                                              std::string key,
                                              std::string value) {
  tasm::report::EventTracker::UpdateGenericInfo(instance_id, std::move(key),
                                                std::move(value));
}

void EventTrackerProxyImpl::UpdateGenericInfo(int32_t instance_id,
                                              std::string key, float value) {
  tasm::report::EventTracker::UpdateGenericInfo(instance_id, std::move(key),
                                                value);
}

void EventTrackerProxyImpl::UpdateGenericInfo(int32_t instance_id,
                                              std::string key, int64_t value) {
  tasm::report::EventTracker::UpdateGenericInfo(instance_id, std::move(key),
                                                value);
}

void EventTrackerProxyImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, float>&& prop_map) {
  tasm::report::EventTracker::UpdateGenericInfo(instance_id,
                                                std::move(prop_map));
}

void EventTrackerProxyImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string>&& prop_map) {
  tasm::report::EventTracker::UpdateGenericInfo(instance_id,
                                                std::move(prop_map));
}

void EventTrackerProxyImpl::ClearCache(int32_t instance_id) {
  tasm::report::EventTracker::ClearCache(instance_id);
}

void EventTrackerProxyImpl::Flush(int32_t instance_id) {
  tasm::report::EventTracker::Flush(instance_id);
}

}  // namespace shell
}  // namespace lynx
