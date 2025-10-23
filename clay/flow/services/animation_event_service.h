// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_SERVICES_ANIMATION_EVENT_SERVICE_H_
#define CLAY_FLOW_SERVICES_ANIMATION_EVENT_SERVICE_H_

#include "clay/common/element_id.h"
#include "clay/common/service/service.h"
#include "clay/gfx/animation/animation_data.h"

namespace clay {

class AnimationEventService
    : public clay::Service<AnimationEventService, clay::Owner::kUI,
                           clay::ServiceFlags::kManualRegister |
                               clay::ServiceFlags::kMultiThread> {
 public:
  virtual void OnAnimationEvent(
      const clay::ElementId& element_id,
      const clay::AnimationParams& animation_params) = 0;
  virtual void OnTransitionEvent(const clay::ElementId& element_id,
                                 const clay::AnimationParams& animation_params,
                                 ClayAnimationPropertyType property_type) = 0;

  virtual void OnScrolled(const clay::ElementId& element_id, int32_t session_id,
                          float scroll_offset, bool ignore_ui_repaint) = 0;

  virtual void OnScrollEnd(const clay::ElementId& element_id,
                           int32_t session_id, float scroll_offset,
                           float velocity) = 0;
};

}  // namespace clay

#endif  // CLAY_FLOW_SERVICES_ANIMATION_EVENT_SERVICE_H_
