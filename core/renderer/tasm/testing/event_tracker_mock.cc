// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/testing/event_tracker_mock.h"

#include <utility>

#include "core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace tasm {
namespace report {

int32_t EventTrackerWaitableEvent::instance_id_ = -1;
std::vector<MoveOnlyEvent> EventTrackerWaitableEvent::stack_;
std::unordered_map<std::string, std::string>
    EventTrackerWaitableEvent::generic_info_;
std::unordered_map<std::string, float>
    EventTrackerWaitableEvent::generic_float_info_;
std::unordered_map<std::string, int64_t>
    EventTrackerWaitableEvent::generic_int64_info_;

std::shared_ptr<fml::AutoResetWaitableEvent>
EventTrackerWaitableEvent::Await() {
  static base::NoDestructor<std::shared_ptr<fml::AutoResetWaitableEvent>> arwe(
      std::make_shared<fml::AutoResetWaitableEvent>());
  return *arwe;
}

void EventTrackerPlatformImpl::OnEvent(int32_t instance_id,
                                       MoveOnlyEvent&& event) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::stack_.clear();
  EventTrackerWaitableEvent::stack_.emplace_back(std::move(event));
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::OnEvents(int32_t instance_id,
                                        std::vector<MoveOnlyEvent> stack) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::stack_ = std::move(stack);
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string> generic_info) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::generic_info_ = std::move(generic_info);
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const std::string& value) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::generic_info_.insert({key, value});
  EventTrackerWaitableEvent::Await()->Signal();
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 float value) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::generic_float_info_.insert({key, value});
  EventTrackerWaitableEvent::Await()->Signal();
}
void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 int64_t value) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::generic_int64_info_.insert({key, value});
  EventTrackerWaitableEvent::Await()->Signal();
}
void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {
  EventTrackerWaitableEvent::instance_id_ = instance_id;
  EventTrackerWaitableEvent::Await()->Signal();
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
