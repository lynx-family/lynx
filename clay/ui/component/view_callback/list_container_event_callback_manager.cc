// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//

#include "clay/ui/component/view_callback/list_container_event_callback_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/list_container/list_container_view.h"
#include "clay/ui/component/page_view.h"

namespace clay {

ListContainerEventCallbackManager::ListContainerEventCallbackManager(
    BaseView* view, int32_t callback_id, PageView* page_view)
    : ScrollEventCallbackManager(view, callback_id) {
  page_view_ = page_view;
}

ListContainerEventCallbackManager::~ListContainerEventCallbackManager() =
    default;

void ListContainerEventCallbackManager::NotifyScrollStateChange(
    ScrollState old_state, ScrollState current_state, float velocity,
    bool is_dragging) const {
  if (ShouldSendEvent(kScrollStateChange)) {
    SendScrollStateChangeEvent(current_state);
  }
}

bool ListContainerEventCallbackManager::ShouldSendEvent(
    ScrollEvents event) const {
  if (disable_platform_scroll_event_ &&
      (event == kScrollEvent || event == kScrollToUpper ||
       event == kScrollToLower)) {
    return false;
  }
  return ScrollEventCallbackManager::ShouldSendEvent(event);
}

void ListContainerEventCallbackManager::SendScrollEvent(
    const char* event_name, const FloatPoint& scrolled,
    const FloatPoint& offset, const FloatSize& content, const bool is_dragging,
    [[maybe_unused]] const EventSource event_source) const {
  clay::Value::Array cells_array;
  if (needs_visible_cells_) {
    cells_array = static_cast<ListContainerView*>(view_)->GetVisibleCells();
  }
  page_view_->SendEvent(
      callback_id_, event_name,
      {"scrollLeft", "scrollTop", "scrollHeight", "scrollWidth", "deltaX",
       "deltaY", "isDragging", "attachedCells", "eventSource"},
      offset.x(), offset.y(), content.height(), content.width(), scrolled.x(),
      scrolled.y(), is_dragging, std::move(cells_array),
      static_cast<int>(event_source));
}

void ListContainerEventCallbackManager::SendScrollStateChangeEvent(
    ScrollState state) const {
  std::vector<float> left_array, right_array, top_array, bottom_array;
  std::vector<int> position_array;
  std::vector<std::string> id_array;
  std::vector<std::string> item_key_array;
  clay::Value::Map args;
  args["state"] = clay::Value(static_cast<int>(state));
  if (needs_visible_cells_) {
    args["attachedCells"] =
        clay::Value(static_cast<ListContainerView*>(view_)->GetVisibleCells());
  }
  page_view_->SendCustomEvent(callback_id_, event_attr::kEventScrollStateChange,
                              std::move(args));
}
};  // namespace clay
