// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/testing/event_tracker_mock.h"

#include <sstream>
#include <utility>

#include "core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace tasm {
namespace report {

int32_t EventTrackerWaitableEvent::instance_id_ = -1;
std::vector<MoveOnlyEvent> EventTrackerWaitableEvent::stack_;
EventTrackerWaitableEvent::InstanceParams
    EventTrackerWaitableEvent::generic_info_by_instance_;
EventTrackerWaitableEvent::InstanceParams
    EventTrackerWaitableEvent::extra_params_by_instance_;
std::unordered_map<int32_t, uint32_t>
    EventTrackerWaitableEvent::query_count_by_instance_;

std::shared_ptr<fml::AutoResetWaitableEvent>
EventTrackerWaitableEvent::Await() {
  static base::NoDestructor<std::shared_ptr<fml::AutoResetWaitableEvent>> arwe(
      std::make_shared<fml::AutoResetWaitableEvent>());
  return *arwe;
}

void EventTrackerPlatformImpl::OnEvent(MoveOnlyEvent&& event) {
  assert(event.IsValidInstanceId());
  EventTrackerWaitableEvent::instance_id_ = event.GetInstanceId();
  EventTrackerWaitableEvent::stack_.clear();
  EventTrackerWaitableEvent::stack_.emplace_back(std::move(event));
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::OnEvents(std::vector<MoveOnlyEvent> stack) {
  EventTrackerWaitableEvent::instance_id_ =
      stack.empty() ? kUnknownInstanceId : stack.front().GetInstanceId();
  EventTrackerWaitableEvent::stack_ = std::move(stack);
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string> generic_info) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  for (const auto& [key, value] : generic_info)
    EventTrackerWaitableEvent::generic_info_by_instance_[instance_id][key] =
        value;
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, float> generic_info) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  for (const auto& [key, value] : generic_info) {
    std::ostringstream stream;
    stream << value;
    EventTrackerWaitableEvent::generic_info_by_instance_[instance_id][key] =
        stream.str();
  }
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const std::string& value) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::generic_info_by_instance_[instance_id][key] =
      value;
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 float value) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  std::ostringstream stream;
  stream << value;
  EventTrackerWaitableEvent::generic_info_by_instance_[instance_id][key] =
      stream.str();
  EventTrackerWaitableEvent::Await()->Signal();
}
void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 int64_t value) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::generic_info_by_instance_[instance_id][key] =
      std::to_string(value);
  EventTrackerWaitableEvent::Await()->Signal();
}

std::string EventTrackerPlatformImpl::GetGenericInfoOrExtraParam(
    int32_t instance_id, const std::string& key) {
  ++EventTrackerWaitableEvent::query_count_by_instance_[instance_id];
  for (const auto* store :
       {&EventTrackerWaitableEvent::generic_info_by_instance_,
        &EventTrackerWaitableEvent::extra_params_by_instance_}) {
    const auto instance = store->find(instance_id);
    if (instance == store->end()) continue;
    const auto value = instance->second.find(key);
    if (value != instance->second.end()) return value->second;
  }
  return {};
}

void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::generic_info_by_instance_.erase(instance_id);
  EventTrackerWaitableEvent::extra_params_by_instance_.erase(instance_id);
  EventTrackerWaitableEvent::Await()->Signal();
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
