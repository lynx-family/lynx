// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PRODUCTION_OPPORTUNITY_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PRODUCTION_OPPORTUNITY_H_

#include <cstdint>

#include "base/include/fml/time/time_point.h"

namespace clay::display {

// Raster-thread budget shared by tree, content and retry production. The owner
// begins a cycle from its frame clock and closes it after an actual attempt.
// Inspecting an empty work queue does not consume the opportunity.
class ProductionOpportunity {
 public:
  void Reset() {
    serial_ = 0;
    open_ = false;
    deadline_ = fml::TimePoint();
  }

  void Begin(fml::TimePoint deadline) {
    ++serial_;
    deadline_ = deadline;
    open_ = true;
  }

  bool CanProduce(fml::TimePoint now) const { return open_ && now < deadline_; }

  void Close() { open_ = false; }

  // This serial is local to a producer binding and restarts after Reset().
  uint64_t serial() const { return serial_; }

 private:
  uint64_t serial_ = 0;
  bool open_ = false;
  fml::TimePoint deadline_;
};

}  // namespace clay::display

#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PRODUCTION_OPPORTUNITY_H_
