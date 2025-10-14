// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_NESTED_SCROLL_RASTER_FLING_MANAGER_H_
#define CLAY_UI_COMPONENT_NESTED_SCROLL_RASTER_FLING_MANAGER_H_

#include "clay/ui/component/nested_scroll/nested_scrollable.h"

namespace clay {
class NestedScrollManager;

class RasterFlingManager {
 public:
  explicit RasterFlingManager(NestedScrollManager* nested_scroll_manager);

  // Start a new raster fling animation.
  // |scrollable| is the target scrollable to fling.
  bool StartAnimation(NestedScrollable* scrollable, float velocity);
  // Stop the current raster fling animation if exists.
  void StopAnimation();
  // It's called when the fling animation is finished from raster.
  // |value| is the final value after fling animation.
  // |velocity| is the remaining velocity after fling animation.
  // We can use the remaining velocity to do the nested scroll if needed.
  void OnAnimationEnd(NestedScrollable* scrollable, int32_t session_id,
                      float value, float velocity);
  // It's called when the fling animation is updated from raster.
  void OnAnimationUpdate(NestedScrollable* scrollable, int32_t session_id,
                         float scroll_offset, bool ignore_ui_repaint);

 private:
  NestedScrollManager* nested_scroll_manager_;
  // Hold the weak pointer of the current scrollable to make it safe-access.
  fml::WeakPtr<BaseView> current_scrollable_;
  int32_t current_session_id_ = 0;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_NESTED_SCROLL_RASTER_FLING_MANAGER_H_
