// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/overlay_manager.h"

#include <algorithm>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/key_event.h"

namespace clay {

namespace {
constexpr const char* kDefaultOverlayIdPrefix = "default_overlay_id_";
}

OverlayManager::OverlayManager(PageView* page_view) : page_view_(page_view) {}

std::string OverlayManager::GenerateNextOverlayId() {
  std::string overlay_id =
      kDefaultOverlayIdPrefix + std::to_string(current_id_++);
  return overlay_id;
}

OverlayView* OverlayManager::GetOverlay(std::string_view overlay_id) const {
  if (overlay_id.empty()) {
    return nullptr;
  }
  const auto it = std::find_if(overlays_.begin(), overlays_.end(),
                               [overlay_id](OverlayView* overlay) {
                                 return overlay_id == overlay->GetOverlayId();
                               });
  if (it == overlays_.end()) {
    FML_DLOG(ERROR) << "Unable to find OverlayView by overlay_id "
                    << overlay_id;
    return nullptr;
  }
  return *it;
}

void OverlayManager::OnHideOverlay(OverlayView* overlay) {
  FML_DCHECK(overlay);
  auto iter = std::find(overlays_.begin(), overlays_.end(), overlay);
  if (iter == overlays_.end()) {
    return;
  }

  overlays_.erase(iter);
  if (overlay->ShouldChangeOffset()) {
    if (overlay->HasEvent(event_attr::kEventDismissOverlay)) {
      page_view_->SendEvent(overlay->id(), event_attr::kEventDismissOverlay,
                            {});
    }
  } else {
    SendOverlayEvent(overlay, "onDismissOverlay");
  }

  if (!overlay->CanEventsPassThroughToViewsBehind() && overlay->Visible()) {
    RestoreFocus();
  }
}

void OverlayManager::OnShowOverlay(OverlayView* overlay) {
  FML_DCHECK(overlay);

  // Avoid add overlay twice
  if (std::find(overlays_.begin(), overlays_.end(), overlay) !=
      overlays_.end()) {
    return;
  }
  if (!overlay->CanEventsPassThroughToViewsBehind() && overlay->Visible()) {
    SaveFocus();
    overlay->RequestFocus();
  }

  // Overlays are ordered by level with descent order. For overlays with the
  // same level, newer is at the end.
  // The actual UI level is managed by the platform side
  auto it = overlays_.rbegin();
  for (; it != overlays_.rend(); it++) {
    if ((*it)->Level() < overlay->Level()) {
      break;
    }
  }
  if (overlays_.size() == 0) {
    overlays_.push_back(overlay);
  } else {
    auto insert_pos = std::next(it.base(), 1);
    overlays_.insert(insert_pos, overlay);
  }
  if (overlay->ShouldChangeOffset()) {
    if (overlay->HasEvent(event_attr::kEventShowOverlay)) {
      // In clay, overlays will never show failure
      page_view_->SendEvent(overlay->id(), event_attr::kEventShowOverlay,
                            {"errorCode", "errorMsg"}, 0, "success");
    } else {
      SendOverlayEvent(overlay, "onShowOverlay");
    }
  }
}

bool OverlayManager::DispatchKeyEvent(const KeyEvent* event) {
  if (IsBackOrEscapeKey(event->GetLogical()) &&
      event->GetType() != KeyEventType::kDown && ignore_up_event_once_) {
    // Block kUp event once.
    ignore_up_event_once_ = false;
    return true;
  }

  for (auto it = overlays_.rbegin(); it != overlays_.rend(); ++it) {
    auto overlay = *it;
    if (!overlay->Visible()) {
      continue;
    }

    if (overlay->GetFocusManager()->DispatchKeyEvent(event)) {
      return true;
    }

    // Check whether should close the overlay.
    if (IsBackOrEscapeKey(event->GetLogical()) &&
        event->GetType() == KeyEventType::kDown) {
      RequestCloseOverlay(overlay->GetOverlayId());
      ignore_up_event_once_ = true;
      return true;
    }

    if (!overlay->CanEventsPassThroughToViewsBehind()) {
      break;
    }
  }

  return false;
}

bool OverlayManager::HitTest(const PointerEvent& event, HitTestResult& result,
                             bool& is_pass_through,
                             PointerEvent& converted_position) {
  for (auto it = overlays_.rbegin(); it != overlays_.rend(); ++it) {
    auto overlay = *it;
    if (!overlay->Visible()) {
      continue;
    }
    auto overlay_result = overlay->HitTest(event, result, is_pass_through);
    if (overlay_result) {
      return true;
    } else if (overlay->ShouldChangeOffset() && is_pass_through) {
      auto offset = overlay->GetTouchOffset();
      converted_position.position.Move(offset.x(), offset.y());
    }
  }
  return false;
}

void OverlayManager::OnReportTopViewEvent(const PointerEvent& event,
                                          ClayEventType type) {
  for (auto it = overlays_.rbegin(); it != overlays_.rend(); ++it) {
    auto overlay = *it;
    if (!overlay->ShouldChangeOffset() || !overlay->Visible()) {
      continue;
    }
    if (overlay->HasEvent(event_attr::kEventOverlayTouch)) {
      int state = 0;
      switch (type) {
        case ClayEventType::kClayEventTypeTouchStart:
          state = 0;
          break;
        case ClayEventType::kClayEventTypeTouchMove:
          state = 1;
          break;
        case ClayEventType::kClayEventTypeTouchEnd:
        case ClayEventType::kClayEventTypeTouchCancel:
          state = 2;
          break;
        default:
          state = -1;
          break;
      }
      // TODO(ZhuChengCheng) we should pass velocity here to aligning the params
      // with lynx Android/iOS
      page_view_->SendEvent(overlay->id(), event_attr::kEventOverlayTouch,
                            {"x", "y", "state"}, event.position.x(),
                            event.position.y(), state);
    }
  }
}

BaseView* OverlayManager::GetTopViewToAcceptEvent(
    const FloatPoint& position, FloatPoint* relative_position,
    bool& is_pass_through, FloatPoint& converted_position,
    int platform_try_hit_id) {
  FML_DCHECK(relative_position);
  bool found_visible_overlay = false;
  Point offset;
  for (auto it = overlays_.rbegin(); it != overlays_.rend(); ++it) {
    auto overlay = *it;
    if (!overlay->Visible()) {
      continue;
    }
    if (overlay->ShouldChangeOffset() && !found_visible_overlay) {
      found_visible_overlay = true;
      // only record top one
      offset = overlay->GetTouchOffset();
    }
    auto view = overlay->GetTopViewToAcceptEvent(position, relative_position,
                                                 platform_try_hit_id);
    if (view) {
      if (view == overlay && overlay->CanEventsPassThroughToViewsBehind()) {
        continue;
      }
      // consumed by overlay child
      return view;
    }
  }
  if (found_visible_overlay) {
    // when pass to flutter view to comsume , should convert cordinate
    is_pass_through = found_visible_overlay;
    converted_position.Move(offset.x(), offset.y());
  }
  return nullptr;
}

void OverlayManager::SaveFocus() {
  focus_saver_.Save(page_view_->GetFocusManager()->GetLeafFocusedNode());
}

void OverlayManager::RestoreFocus() { focus_saver_.Restore(); }

void OverlayManager::RequestCloseOverlay(std::string_view overlay_id) {
  FML_DCHECK(!overlay_id.empty());

  OverlayView* overlay = GetOverlay(overlay_id);
  FML_DCHECK(overlay);
  SendOverlayEvent(overlay, "onRequestClose");
}

void OverlayManager::SendOverlayEvent(OverlayView* overlay,
                                      const char* event_name) const {
  if (!page_view_->GetEventDelegate()) {
    return;
  }
  size_t overlay_count = 0;
  std::vector<const char*> overlay_ids(overlays_.size());
  for (auto& overlay : overlays_) {
    overlay_ids[overlay_count++] = overlay->GetOverlayId().data();
  }

  page_view_->GetEventDelegate()->OnOverlayEvent(
      overlay->id(), overlay->GetOverlayId().data(), overlay_count,
      overlay_ids.data(), event_name);
}

}  // namespace clay
