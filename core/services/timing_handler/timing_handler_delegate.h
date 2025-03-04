// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_DELEGATE_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_DELEGATE_H_

#include <memory>

#include "core/services/timing_handler/timing_info.h"

// Delegate interface for handling timing events.
namespace lynx {
namespace tasm {
namespace timing {
class TimingHandlerDelegate {
 public:
  TimingHandlerDelegate() = default;

  virtual ~TimingHandlerDelegate() = default;

  virtual const std::shared_ptr<pub::PubValueFactory> &GetValueFactory()
      const = 0;
  virtual void OnTimingSetup(const TimingInfo &timing_info) const = 0;
  virtual void OnTimingUpdate(const TimingInfo &timing_info,
                              const TimingFlag &update_flag) const = 0;
  virtual void OnPerformanceEvent(
      const std::unique_ptr<lynx::pub::Value> performance_entry,
      bool enable_engine_callback) const = 0;
};
}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_DELEGATE_H_
