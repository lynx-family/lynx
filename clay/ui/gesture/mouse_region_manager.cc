// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/mouse_region_manager.h"

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"

namespace clay {
namespace {

// Must match the tag name used to register the platform view factory
// (see `[LynxView registerViewFactory:@"x-webview"
// withClass:LynxWebView.class]`). When the hit-tested top view is this native
// view, the platform WKWebView should own the cursor exclusively. If Clay also
// dispatches a cursor via
// `[NSCursor set]`, the two owners fight on every mouse-move event and the
// cursor flickers between the CSS "pointer" and the default arrow.
constexpr const char kXWebViewTag[] = "x-webview";

bool IsCursorOwnedByPlatformView(BaseView* top_view) {
  if (top_view == nullptr) {
    return false;
  }
  if (!top_view->Is<NativeView>()) {
    return false;
  }
  return top_view->GetName() == kXWebViewTag;
}

}  // namespace

MouseRegionManager::~MouseRegionManager() {
#if defined(OS_WIN) || defined(OS_MAC)
  Reset();
#endif
}

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
  const bool force_empty = event.type == PointerEvent::EventType::kCancel
#if defined(OS_WIN) || defined(OS_MAC)
                           ||
                           event.type == PointerEvent::EventType::kRemoveEvent
#endif
      ;
  ViewChain view_chain =
      force_empty ? ViewChain{} : BuildViewChain(root, event, &top_view);

  // If the cursor is currently over a platform-owned native view (e.g.
  // x-webview / WKWebView), skip the cursor dispatch so the platform view
  // remains the sole cursor owner and no flickering occurs. Enter, leave,
  // and hover region routing still updates "prev_chain_" normally, so the
  // Clay-computed cursor is restored on the next event after the pointer
  // leaves the x-webview area.
  if (!IsCursorOwnedByPlatformView(top_view)) {
    mouse_cursor_manager_->HandleHitTestResult(view_chain);
  }

  if (prev_chain_ == view_chain) {
    // stay in the same mouse region
    if (!root->page_view()->AlignMouseEventWithW3C()) {
      auto target_iter = view_chain.begin();
      while (target_iter != view_chain.end()) {
        auto route_iter = mouse_region_routes_.find(target_iter->get());
        if (route_iter != mouse_region_routes_.end() &&
            route_iter->second.on_hover) {
          route_iter->second.on_hover(event);
        }
        ++target_iter;
      }
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

MouseRegionManager::ViewChain MouseRegionManager::BuildViewChain(
    BaseView* root, const PointerEvent& event, BaseView** top_view) const {
  ViewChain view_chain;
  if (top_view) {
    *top_view = nullptr;
  }

  FloatPoint relative_position;
  BaseView* hit_view = root->page_view()->GetTopViewToAcceptEvent(
      event.position, &relative_position);
  if (hit_view == nullptr) {
    return view_chain;
  }
  if (top_view) {
    *top_view = hit_view;
  }

  if (root->page_view()->GetUIComponentDelegate()) {
    std::list<int32_t> element_chain =
        root->page_view()->GetUIComponentDelegate()->GetAncestorElements(
            hit_view->GetCallbackId());
    for (auto id : element_chain) {
      auto* view = root->page_view()->FindViewByViewId(id);
      if (view) {
        view_chain.emplace_back(view->GetWeakPtr());
      }
    }
    return view_chain;
  }

  view_chain.emplace_back(hit_view->GetWeakPtr());
  for (BaseView* parent = hit_view->Parent(); parent != nullptr;
       parent = parent->Parent()) {
    view_chain.emplace_back(parent->GetWeakPtr());
  }
  return view_chain;
}

#if defined(OS_WIN) || defined(OS_MAC)
int MouseRegionManager::GetTargetSign(const ViewChain& chain) {
  for (const auto& target : chain) {
    if (target) {
      return target->GetCallbackId();
    }
  }
  return -1;
}

void MouseRegionManager::UpdatePointerChain(const PointerEvent& event,
                                            const ViewChain& view_chain) {
  const auto dispatch_pointer_boundary =
      [&event](const ViewChain::value_type& target, const char* event_name,
               int related_target_sign) {
        if (target) {
          target->OnPointerBoundaryEvent(event_name, event,
                                         related_target_sign);
        }
      };
  const PointerKey key{event.device, event.pointer_id};
  auto previous_iter = pointer_chains_.find(key);
  ViewChain empty_chain;
  ViewChain& previous_chain = previous_iter == pointer_chains_.end()
                                  ? empty_chain
                                  : previous_iter->second;
  bool removed_detached_target = false;
  while (
      !previous_chain.empty() &&
      (!previous_chain.front() || !previous_chain.front()->attach_to_tree())) {
    previous_chain.pop_front();
    removed_detached_target = true;
  }
  if (previous_chain == view_chain) {
    if (removed_detached_target && !view_chain.empty()) {
      dispatch_pointer_boundary(view_chain.front(), "pointerover", -1);
    }
    if (view_chain.empty() && previous_iter != pointer_chains_.end()) {
      pointer_chains_.erase(previous_iter);
    }
    return;
  }

  const int previous_target_sign = GetTargetSign(previous_chain);
  const int current_target_sign = GetTargetSign(view_chain);

  if (!previous_chain.empty()) {
    dispatch_pointer_boundary(previous_chain.front(), "pointerout",
                              current_target_sign);
  }

  auto current_reverse = view_chain.rbegin();
  auto previous_reverse = previous_chain.rbegin();
  while (current_reverse != view_chain.rend() &&
         previous_reverse != previous_chain.rend() &&
         *current_reverse == *previous_reverse) {
    ++current_reverse;
    ++previous_reverse;
  }

  for (auto target = previous_chain.begin(); target != previous_reverse.base();
       ++target) {
    dispatch_pointer_boundary(*target, "pointerleave", current_target_sign);
  }

  if (!view_chain.empty()) {
    dispatch_pointer_boundary(view_chain.front(), "pointerover",
                              previous_target_sign);
  }

  for (auto target = current_reverse; target != view_chain.rend(); ++target) {
    dispatch_pointer_boundary(*target, "pointerenter", previous_target_sign);
  }

  if (view_chain.empty()) {
    pointer_chains_.erase(key);
  } else {
    pointer_chains_.insert_or_assign(key, view_chain);
  }
}

void MouseRegionManager::RefreshPointerEventTarget(BaseView* root,
                                                   const PointerEvent& event) {
  PointerEvent boundary_event = event;
  boundary_event.button = -1;
  UpdatePointerChain(boundary_event, BuildViewChain(root, boundary_event));
}

void MouseRegionManager::SchedulePointerEventTargetRefresh(BaseView* root) {
  if (last_pointer_events_.empty()) {
    return;
  }
  pending_refresh_root_ = root->GetWeakPtr();
  root->page_view()->GetAnimationHandler()->AddAnimationFrameCallback(this, 0);
}

const PointerEvent* MouseRegionManager::GetLastPointerEvent(
    const PointerEvent& event) const {
  auto previous =
      last_pointer_events_.find(PointerKey{event.device, event.pointer_id});
  return previous == last_pointer_events_.end() ? nullptr : &previous->second;
}

bool MouseRegionManager::DoAnimationFrame(int64_t, bool) {
  auto* root = pending_refresh_root_.get();
  if (!root) {
    return true;
  }
  root->page_view()->GetAnimationHandler()->RemoveCallback(this);
  pending_refresh_root_ = {};
  RefreshPointerEventTargets(root);
  return true;
}

void MouseRegionManager::RefreshPointerEventTargets(BaseView* root) {
  std::vector<PointerEvent> events;
  events.reserve(last_pointer_events_.size());
  for (const auto& [key, event] : last_pointer_events_) {
    if (implicitly_captured_pointers_.find(key) ==
        implicitly_captured_pointers_.end()) {
      events.push_back(event);
    }
  }
  for (const auto& event : events) {
    RefreshPointerEventTarget(root, event);
  }
}

void MouseRegionManager::HandlePointerEventBefore(BaseView* root,
                                                  const PointerEvent& event) {
  if (event.device == PointerEvent::DeviceType::kTrackpad) {
    return;
  }

  const PointerKey key{event.device, event.pointer_id};
  if (event.type != PointerEvent::EventType::kCancel &&
      event.type != PointerEvent::EventType::kRemoveEvent) {
    last_pointer_events_.insert_or_assign(key, event);
  }
  if (event.type == PointerEvent::EventType::kDownEvent &&
      event.device != PointerEvent::DeviceType::kMouse) {
    implicitly_captured_pointers_.insert(key);
  }

  bool update_chain = false;
  switch (event.device) {
    case PointerEvent::DeviceType::kTouch:
      update_chain = event.type == PointerEvent::EventType::kDownEvent;
      break;
    case PointerEvent::DeviceType::kStylus:
    case PointerEvent::DeviceType::kInvertedStylus:
      update_chain = event.type == PointerEvent::EventType::kAddEvent ||
                     event.type == PointerEvent::EventType::kHoverEvent ||
                     event.type == PointerEvent::EventType::kDownEvent;
      break;
    case PointerEvent::DeviceType::kMouse:
      update_chain = event.type == PointerEvent::EventType::kAddEvent ||
                     event.type == PointerEvent::EventType::kHoverEvent ||
                     event.type == PointerEvent::EventType::kDownEvent ||
                     event.type == PointerEvent::EventType::kMoveEvent ||
                     event.type == PointerEvent::EventType::kUpEvent;
      break;
    case PointerEvent::DeviceType::kTrackpad:
      break;
  }
  if (!update_chain) {
    return;
  }

  PointerEvent boundary_event = event;
  boundary_event.button = -1;
  if (event.type == PointerEvent::EventType::kDownEvent &&
      event.device != PointerEvent::DeviceType::kMouse) {
    boundary_event.buttons = 0;
  }
  UpdatePointerChain(boundary_event, BuildViewChain(root, boundary_event));
}

void MouseRegionManager::HandlePointerEventAfter(BaseView* root,
                                                 const PointerEvent& event) {
  if (event.device == PointerEvent::DeviceType::kTrackpad) {
    return;
  }

  const PointerKey key{event.device, event.pointer_id};

  bool clear_chain = event.type == PointerEvent::EventType::kRemoveEvent ||
                     event.type == PointerEvent::EventType::kCancel ||
                     (event.device == PointerEvent::DeviceType::kTouch &&
                      event.type == PointerEvent::EventType::kUpEvent);
  if (clear_chain) {
    PointerEvent boundary_event = event;
    boundary_event.button = -1;
    UpdatePointerChain(boundary_event, {});
    implicitly_captured_pointers_.erase(key);
    last_pointer_events_.erase(key);
    return;
  }

  bool release_stylus_capture =
      (event.device == PointerEvent::DeviceType::kStylus ||
       event.device == PointerEvent::DeviceType::kInvertedStylus) &&
      event.type == PointerEvent::EventType::kUpEvent;
  if (release_stylus_capture) {
    implicitly_captured_pointers_.erase(key);
    PointerEvent boundary_event = event;
    boundary_event.button = -1;
    UpdatePointerChain(boundary_event, BuildViewChain(root, boundary_event));
  }
}
#endif

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

void MouseRegionManager::ForceUpdateCursor() {
  // Mirror the guard in HandleEvent: while the cursor hovers over an
  // x-webview, suppress any forced cursor refresh triggered by SetCursor so
  // Clay does not fight WKWebView's own cursor updates.
  if (prev_chain_.empty()) {
    return;
  }
  BaseView* top_view = prev_chain_.front().get();
  if (IsCursorOwnedByPlatformView(top_view)) {
    return;
  }
  mouse_cursor_manager_->HandleHitTestResult(prev_chain_);
}

#if defined(OS_WIN) || defined(OS_MAC)
void MouseRegionManager::Reset() {
  if (pending_refresh_root_) {
    pending_refresh_root_->page_view()->GetAnimationHandler()->RemoveCallback(
        this);
    pending_refresh_root_ = {};
  }
  prev_chain_.clear();
  pointer_chains_.clear();
  last_pointer_events_.clear();
  implicitly_captured_pointers_.clear();
}
#endif

}  // namespace clay
