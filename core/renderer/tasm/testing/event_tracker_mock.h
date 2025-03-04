// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_TESTING_EVENT_TRACKER_MOCK_H_
#define CORE_RENDERER_TASM_TESTING_EVENT_TRACKER_MOCK_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace tasm {
namespace report {

class EventTrackerWaitableEvent {
 public:
  static std::shared_ptr<fml::AutoResetWaitableEvent> Await();
  static int32_t instance_id_;
  static std::vector<MoveOnlyEvent> stack_;
  static std::unordered_map<std::string, std::string> generic_info_;
  static std::unordered_map<std::string, float> generic_float_info_;
  static std::unordered_map<std::string, int64_t> generic_int64_info_;
};
}  // namespace report
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_TESTING_EVENT_TRACKER_MOCK_H_
