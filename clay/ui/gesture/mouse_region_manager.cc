// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/mouse_region_manager.h"

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"

namespace clay {

void MouseRegionManager::RegisterEnterCallback(BaseView* target,
                                               EnterCallback callback) {
  mouse_region_routes_[target].on_enter = callback;
}
void MouseRegionManager::RegisterLeaveCallback(BaseView* target,
                                               LeaveCallback callback) {
  mouse_region_routes_[target].on_leave = callback;
}
void MouseRegionManager::RegisterHoverCallback(BaseView* target,
                                               HoverCallback callback) {
  mouse_region_routes_[target].on_hover = callback;
}

void MouseRegionManager::UnregisterCallback(BaseView* target) {
  // TODO(yangliu): support multi callbacks
  mouse_region_routes_.erase(target);
}

void MouseRegionManager::HandleEvents(BaseView* root,
                                      const std::vector<PointerEvent>& events) {
  for (auto& event : events) {
    HandleEvent(root, event);
  }
}
void MouseRegionManager::HandleEvent(BaseView* root,
                                     const PointerEvent& event) {
  // TODO: Consider to be refactored with TouchEventHandler in future, see:
  // lynx/core/renderer/events/touch_event_handler.cc.
  BaseView* top_view = nullptr;
  if (event.type != PointerEvent::EventType::kCancel) {
    FloatPoint relative_position;
    top_view = root->page_view()->GetTopViewToAcceptEvent(event.position,
                                                          &relative_position);
  }
  std::list<fml::WeakPtr<BaseView>> view_chain;
  if (top_view != nullptr) {
    if (root->page_view()->GetUIComponentDelegate()) {
      // Get the origin elements with ancestors from UIComponentDelegate, and
      // should not affected by z-index attribute.
      std::list<int32_t> element_chain =
          root->page_view()->GetUIComponentDelegate()->GetAncestorElements(
              top_view->GetCallbackId());
      for (auto id : element_chain) {
        auto* view = root->page_view()->FindViewByViewId(id);
        if (view) {
          view_chain.emplace_back(view->GetWeakPtr());
        }
      }
    } else {
      // For cases with no UIComponentDelegate like running unittests, we just
      // get the element chain from the top view.
      view_chain.emplace_back(top_view->GetWeakPtr());
      BaseView* parent = top_view->Parent();
      while (parent != nullptr) {
        view_chain.emplace_back(parent->GetWeakPtr());
        parent = parent->Parent();
      }
    }
  }

  mouse_cursor_manager_->HandleHitTestResult(view_chain);

  if (prev_chain_ == view_chain) {
    // stay in the same mouse region
    auto target_iter = view_chain.begin();
    while (target_iter != view_chain.end()) {
      auto route_iter = mouse_region_routes_.find(target_iter->get());
      if (route_iter != mouse_region_routes_.end() &&
          route_iter->second.on_hover) {
        route_iter->second.on_hover(event);
      }
      ++target_iter;
    }
    return;
  }

  // detect the unchanging mouse regions
  auto curr_riter = view_chain.rbegin();
  auto prev_riter = prev_chain_.rbegin();
  while (curr_riter != view_chain.rend() && prev_riter != prev_chain_.rend() &&
         *curr_riter == *prev_riter) {
    ++curr_riter;
    ++prev_riter;
  }

  // leave old mouse regions from child to parent
  if (prev_riter != prev_chain_.rend()) {
    auto target_iter = prev_chain_.begin();
    auto target_end = prev_riter.base();
    while (target_iter != target_end) {
      auto route_iter = mouse_region_routes_.find(target_iter->get());
      if (route_iter != mouse_region_routes_.end() &&
          route_iter->second.on_leave) {
        route_iter->second.on_leave(event);
      }
      ++target_iter;
    }
  }

  // enter new mouse regions from parent to child
  if (curr_riter != view_chain.rend()) {
    auto target_riter = curr_riter;
    auto target_rend = view_chain.rend();
    while (target_riter != target_rend) {
      auto route_iter = mouse_region_routes_.find(target_riter->get());
      if (route_iter != mouse_region_routes_.end() &&
          route_iter->second.on_enter) {
        route_iter->second.on_enter(event);
      }
      ++target_riter;
    }
  }

  // update chain
  prev_chain_ = view_chain;
}

void MouseRegionManager::InitSubManager(
    MouseCursorManager::ActiveCursorCallback active_cursor_callback) {
  // init MouseCursorManager
  mouse_cursor_manager_ =
      std::make_unique<MouseCursorManager>(active_cursor_callback);
}

void MouseRegionManager::AddCursorHolder(BaseView* holder) {
  FML_DCHECK(mouse_cursor_manager_)
      << "MouseRegionManager : should init sub manager";
  mouse_cursor_manager_->AddCursorHolder(holder);
}

}  // namespace clay
