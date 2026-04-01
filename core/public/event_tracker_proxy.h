// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_EVENT_TRACKER_PROXY_H_
#define CORE_PUBLIC_EVENT_TRACKER_PROXY_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/closure.h"

namespace lynx {
namespace shell {

struct EventTrackerEvent {
  std::string name;
  std::unordered_map<std::string, std::string> string_props;
  std::unordered_map<std::string, int> int_props;
  std::unordered_map<std::string, double> double_props;
};

class EventTrackerProxy {
 public:
  using EventBuilder = base::MoveOnlyClosure<void, EventTrackerEvent&>;

  virtual ~EventTrackerProxy() = default;

  virtual void OnEvent(EventBuilder builder) = 0;

  virtual void UpdateGenericInfo(int32_t instance_id, std::string key,
                                 std::string value) = 0;
  virtual void UpdateGenericInfo(int32_t instance_id, std::string key,
                                 float value) = 0;
  virtual void UpdateGenericInfo(int32_t instance_id, std::string key,
                                 int64_t value) = 0;
  virtual void UpdateGenericInfo(
      int32_t instance_id,
      std::unordered_map<std::string, float>&& prop_map) = 0;
  virtual void UpdateGenericInfo(
      int32_t instance_id,
      std::unordered_map<std::string, std::string>&& prop_map) = 0;
  virtual void ClearCache(int32_t instance_id) = 0;
  virtual void Flush(int32_t instance_id) = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_EVENT_TRACKER_PROXY_H_
