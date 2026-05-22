// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/animation_event_service_impl.h"

#include <string>

#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/scroll_wrapper.h"

namespace clay {

namespace {

clay::ScrollView* FindTargetScrollView(Engine* engine, int32_t target_id) {
  clay::BaseView* target_view =
      engine->GetViewContext()->FindViewByViewId(target_id);
  if (!target_view) {
    return nullptr;
  }
  if (target_view->Is<clay::ScrollView>()) {
    return static_cast<clay::ScrollView*>(target_view);
  } else if (target_view->Is<clay::ScrollWrapper>()) {
    return static_cast<clay::ScrollWrapper*>(target_view)->GetScrollView();
  }
  return nullptr;
}

}  // namespace

void AnimationEventServiceImpl::OnAnimationEvent(
    const clay::ElementId& element_id,
    const clay::AnimationParams& animation_params) {
  std::string animation_name = animation_params.animation_name;
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_->SelectTaskRunner<clay::Owner::kUI>(),
      [engine = engine_, element_id, type = animation_params.event_type,
       name = animation_name] {
        if (engine) {
          engine->OnAnimationEvent(element_id, {type, name.c_str()});
        }
      });
}

void AnimationEventServiceImpl::OnTransitionEvent(
    const clay::ElementId& element_id,
    const clay::AnimationParams& animation_params,
    ClayAnimationPropertyType property_type) {
  std::string animation_name = animation_params.animation_name;
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_->SelectTaskRunner<clay::Owner::kUI>(),
      [engine = engine_, element_id, type = animation_params.event_type,
       name = animation_name, property_type] {
        if (engine) {
          engine->OnTransitionEvent(element_id, {type, name.c_str()},
                                    property_type);
        }
      });
}

void AnimationEventServiceImpl::OnScrolled(const clay::ElementId& element_id,
                                           int32_t session_id,
                                           float scroll_offset,
                                           bool ignore_ui_repaint) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_->SelectTaskRunner<clay::Owner::kUI>(),
      [engine = engine_, element_id = element_id.view_id(), session_id,
       scroll_offset, ignore_ui_repaint] {
        if (engine) {
          clay::ScrollView* target_view =
              FindTargetScrollView(engine.get(), element_id);
          if (target_view) {
            auto* raster_fling_manager = target_view->page_view()
                                             ->nested_scroll_manager()
                                             ->raster_fling_manager();
            if (raster_fling_manager) {
              raster_fling_manager->OnAnimationUpdate(
                  target_view, session_id, scroll_offset, ignore_ui_repaint);
            }
          }
        }
      });
}

void AnimationEventServiceImpl::OnScrollEnd(const clay::ElementId& element_id,
                                            int32_t session_id,
                                            float scroll_offset,
                                            float velocity) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_->SelectTaskRunner<clay::Owner::kUI>(),
      [engine = engine_, element_id = element_id.view_id(), session_id,
       scroll_offset, velocity] {
        if (engine) {
          clay::ScrollView* target_view =
              FindTargetScrollView(engine.get(), element_id);
          if (target_view) {
            auto* raster_fling_manager = target_view->page_view()
                                             ->nested_scroll_manager()
                                             ->raster_fling_manager();
            if (raster_fling_manager) {
              raster_fling_manager->OnAnimationEnd(target_view, session_id,
                                                   scroll_offset, velocity);
            }
          }
        }
      });
}

void AnimationEventServiceImpl::OnInit(clay::ServiceManager& service_manager,
                                       const clay::UIServiceContext& ctx) {
  task_runners_ = service_manager.GetTaskRunners();
  engine_ = ctx.engine->GetWeakPtr();
}

}  // namespace clay
