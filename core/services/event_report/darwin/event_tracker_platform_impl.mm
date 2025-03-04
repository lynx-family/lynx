// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker_platform_impl.h"

#import "LynxEventReporter.h"
#import "LynxLog.h"

namespace lynx {
namespace tasm {
namespace report {

void EventTrackerPlatformImpl::OnEvent(int32_t instance_id, MoveOnlyEvent&& event) {
  LLogInfo(@"EventTracker onEvent with name: %s", event.GetName().c_str());
  NSString* eventName = [NSString stringWithUTF8String:event.GetName().c_str()];
  if (!eventName) {
    return;
  }
  NSMutableDictionary* props = [NSMutableDictionary dictionary];
  for (auto const& item : event.GetStringProps()) {
    NSString* key = [NSString stringWithUTF8String:item.first.c_str()];
    NSString* value = [NSString stringWithUTF8String:item.second.c_str()];
    if (key && value) {
      [props setObject:value forKey:key];
    }
  }
  for (auto const& item : event.GetIntProps()) {
    NSString* key = [NSString stringWithUTF8String:item.first.c_str()];
    NSNumber* value = [NSNumber numberWithInt:item.second];
    if (key && value) {
      [props setObject:value forKey:key];
    }
  }
  for (auto const& item : event.GetDoubleProps()) {
    NSString* key = [NSString stringWithUTF8String:item.first.c_str()];
    NSNumber* value = [NSNumber numberWithDouble:item.second];
    if (key && value) {
      [props setObject:value forKey:key];
    }
  }
  [LynxEventReporter onEvent:eventName instanceId:instance_id props:props.copy];
}

void EventTrackerPlatformImpl::OnEvents(int32_t instance_id, std::vector<MoveOnlyEvent> stack) {
  for (auto& event : stack) {
    OnEvent(instance_id, std::move(event));
  }
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, std::string> generic_info) {
  for (auto const& item : generic_info) {
    NSString* key = [NSString stringWithUTF8String:item.first.c_str()];
    NSString* value = [NSString stringWithUTF8String:item.second.c_str()];
    [LynxEventReporter updateGenericInfo:value key:key instanceId:instance_id];
  }
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id, const std::string& key,
                                                 const std::string& value) {
  NSString* keyNSStr = [NSString stringWithUTF8String:key.c_str()];
  NSString* valueNSStr = [NSString stringWithUTF8String:value.c_str()];
  [LynxEventReporter updateGenericInfo:valueNSStr key:keyNSStr instanceId:instance_id];
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id, const std::string& key,
                                                 int64_t value) {
  NSString* keyNSStr = [NSString stringWithUTF8String:key.c_str()];
  [LynxEventReporter updateGenericInfo:@(value) key:keyNSStr instanceId:instance_id];
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id, const std::string& key,
                                                 const float value) {
  NSString* keyNSStr = [NSString stringWithUTF8String:key.c_str()];
  NSNumber* valueNSNum = [[NSNumber alloc] initWithFloat:value];
  [LynxEventReporter updateGenericInfo:valueNSNum key:keyNSStr instanceId:instance_id];
}

void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {
  [LynxEventReporter clearCacheForInstanceId:instance_id];
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
