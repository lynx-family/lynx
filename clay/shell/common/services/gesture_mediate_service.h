// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_GESTURE_MEDIATE_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_GESTURE_MEDIATE_SERVICE_H_

#include "clay/common/service/service.h"
#include "clay/ui/gesture/hit_test_responsive_result.h"
#include "clay/ui/gesture/scrollable_direction.h"

namespace clay {

class GestureMediateService
    : public clay::Service<GestureMediateService, clay::Owner::kPlatform,
                           clay::ServiceFlags::kManualRegister> {
 public:
  // Called by Clay
  void UpdateResponsiveResult(HitTestResponsiveResult result) {
    result_ = result;
  }

  // Called by Platform
  void UpdateOuterScrollableDirection(ScrollableDirection direction) {
    outer_scrollable_direction_ = direction;
  }

  // Called by Platform
  HitTestResponsiveResult GetResponsiveResult() { return result_; }

  // Called by Clay
  ScrollableDirection GetOuterScrollableDirection() {
    return outer_scrollable_direction_;
  }

 private:
  HitTestResponsiveResult result_;
  ScrollableDirection outer_scrollable_direction_ = ScrollableDirection::kNone;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_GESTURE_MEDIATE_SERVICE_H_
