// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_VIEW_CALLBACK_LIST_CONTAINER_EVENT_CALLBACK_MANAGER_H_
#define CLAY_UI_COMPONENT_VIEW_CALLBACK_LIST_CONTAINER_EVENT_CALLBACK_MANAGER_H_

#include "clay/ui/component/view_callback/scroll_event_callback_manager.h"

namespace clay {

class PageView;

class ListContainerEventCallbackManager : public ScrollEventCallbackManager {
 public:
  ListContainerEventCallbackManager(BaseView* view, int32_t callback_id,
                                    PageView* page_view);
  ~ListContainerEventCallbackManager();

  void NotifyScrollStateChange(ScrollState old_state, ScrollState current_state,
                               float velocity, bool is_dragging) const override;

  void SendScrollEvent(const char* event_name, const FloatPoint& scrolled,
                       const FloatPoint& offset, const FloatSize& content,
                       const bool is_dragging) const override;

  void SendScrollStateChangeEvent(ScrollState state) const;

  void SetNeedsVisibleCells(bool enable) { needs_visible_cells_ = enable; }

 private:
  bool needs_visible_cells_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_VIEW_CALLBACK_LIST_CONTAINER_EVENT_CALLBACK_MANAGER_H_
