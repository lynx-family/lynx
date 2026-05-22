// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_ANIMATION_EVENT_SERVICE_IMPL_H_
#define CLAY_SHELL_COMMON_SERVICES_ANIMATION_EVENT_SERVICE_IMPL_H_

#include <memory>

#include "clay/common/element_id.h"
#include "clay/flow/services/animation_event_service.h"
#include "clay/gfx/animation/animation_data.h"
#include "clay/shell/common/engine.h"

namespace clay {

class AnimationEventServiceImpl : public AnimationEventService {
 private:
  void OnAnimationEvent(const clay::ElementId& element_id,
                        const clay::AnimationParams& animation_params) override;

  void OnTransitionEvent(const clay::ElementId& element_id,
                         const clay::AnimationParams& animation_params,
                         ClayAnimationPropertyType property_type) override;

  void OnScrolled(const clay::ElementId& element_id, int32_t session_id,
                  float scroll_offset, bool ignore_ui_repaint) override;

  void OnScrollEnd(const clay::ElementId& element_id, int32_t session_id,
                   float scroll_offset, float velocity) override;

  void OnInit(clay::ServiceManager& service_manager,
              const clay::UIServiceContext& ctx) override;

  fml::WeakPtr<Engine> engine_;
  std::shared_ptr<clay::ServiceTaskRunners> task_runners_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_ANIMATION_EVENT_SERVICE_IMPL_H_
