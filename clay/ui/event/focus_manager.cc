// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/event/focus_manager.h"

#include <cmath>

#include "clay/fml/logging.h"
#include "clay/ui/component/focus/focus_isolate.h"
#include "clay/ui/component/focus_node.h"
#include "clay/ui/event/key_event.h"

namespace clay {
FocusManager::FocusManager(FocusNode* focus_scope_node)
    : focus_scope_node_(focus_scope_node) {}

void FocusManager::RegisterFocusableView(FocusNode* view) {
  FML_DCHECK(view);
  UnregisterFocusableView(view);
  focusable_views_.push_back(view);

  // the focus scope is unfocusable by default, and is set focusable
  // when it has focusable child views
  if (focus_scope_node_ && focusable_views_.size() == 1) {
    focus_scope_node_->StoreFocusable();
    focus_scope_node_->SetFocusable(true);
  }
}

void FocusManager::UnregisterFocusableView(FocusNode* view) {
  FML_DCHECK(view);
  if (current_focus_view_ == view) {
    current_focus_view_ = nullptr;
  }
  for (auto it = focusable_views_.begin(); it != focusable_views_.end(); ++it) {
    if (*it == view) {
      focusable_views_.erase(it);
      // when the child focusable views of a focus scope are all removed,
      // restore the focusable attribute of the focus scope to its original
      // value.
      if (focus_scope_node_ && focusable_views_.empty()) {
        focus_scope_node_->RestoreFocusable();
      }
      break;
    }
  }
}

void FocusManager::RequestFocus(FocusNode* view) {
  FML_DCHECK(view);

  if (current_focus_view_ == view) {
    return;
  }
  if (current_focus_view_) {
    current_focus_view_->ClearFocus();
  }
  current_focus_view_ = view;
  if (focus_scope_node_ && !focus_scope_node_->IsFocused()) {
    focus_scope_node_->RequestFocus(false);
  }

  if (!view->IsFocusScope()) {
    FocusManager* manager = this;
    while (manager) {
      FocusNode* focus_scope = manager->focus_scope_node_;
      if (static_cast<BaseView*>(focus_scope)->Is<FocusIsolate>()) {
        static_cast<FocusIsolate*>(focus_scope)->UpdateLastFocusChild(view);
        break;
      }
      manager = focus_scope->GetParentFocusManager();
    }
  }
}

void FocusManager::ClearFocus(FocusNode* view) {
  FML_DCHECK(view);

  if (current_focus_view_ == view) {
    current_focus_view_ = nullptr;
  }
}

void FocusManager::SetFocusable(bool focusable) {
  focusable_ = focusable;
  if (!focusable_ && current_focus_view_) {
    current_focus_view_->ClearFocus();
  }
}

void FocusManager::SetIsRootScope() {
  SetFocusable(true);
  is_root_ = true;
}

FocusManager* FocusManager::GetRootFocusManager() {
  if (is_root_) {
    return this;
  }
  return focus_scope_node_->GetParentFocusManager();
}

bool FocusManager::DispatchKeyEvent(const KeyEvent* event) {
  if (OnKeyEvent(event)) {
    return true;
  }

  auto now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  if (now < disable_focus_until_time_) {
    return false;
  }

  Direction direction = Direction::kUnknown;
  // On Desktop, we use the Tab key to determine the direction of focus, rather
  // than the arrow keys.
#if !defined(OS_WIN) && !defined(OS_MAC) && !defined(OS_HARMONY)
  KeyCode code = event->GetLogical();
  switch (code) {
    case KeyCode::kArrowUp:
      direction = Direction::kUp;
      break;
    case KeyCode::kArrowDown:
      direction = Direction::kDown;
      break;
    case KeyCode::kArrowLeft:
      direction = Direction::kLeft;
      break;
    case KeyCode::kArrowRight:
      direction = Direction::kRight;
      break;
    default:
      break;
  }
#endif

  FocusManager::TraversalResult traversal_result;
  if (direction != Direction::kUnknown &&
      event->GetType() != KeyEventType::kUp) {
    if (last_trigger_time_ > 0 &&
        (now - last_trigger_time_) < trigger_interval_) {
      // Skip if this key event is triggered too often
      return false;
    }
    last_trigger_time_ = now;
    traversal_result = DoTraversal(direction);
  }
  if (traversal_result.succeed) {
    if (preference_.switch_up_to_once_per_frame) {
      // Trigger focus switching too often (e.g. long press a arrow key) can
      // cause performance issues. Here we disable focus switching for this
      // frame after it has been successfully triggered.
      DisableFocusUntilNextFrame();
    }
    return true;
  }

  return false;
}

FocusManager::TraversalResult FocusManager::DoTraversal(Direction direction) {
  // Handle `next-focus-*` attributes.
  if (auto focus_node = GetLeafFocusedNode()) {
    const auto& next_focus_id = focus_node->GetNextFocusId(direction);
    if (!next_focus_id.empty()) {
      TraversalResult result{false, direction};
      if (auto next_focus_node = FindFocusViewById(next_focus_id)) {
        next_focus_node->RequestFocus();
        result.succeed = true;
      }
      if (result.succeed || !focus_node->NextFocusFallback()) {
        OnTraversalEnd(result);
        return result;
      }
      // Fallback to default traversal
    }
  }

  TraversalResult result =
      DoTraversalInternal(direction, TraversalType::kDefault);
  // Traversal by kDefault is irreversible. It will cause the focus state to get
  // stuck. So we need to try traversal again.
  if (!result.succeed) {
    result = DoTraversalInternal(direction, TraversalType::kCenter);
  }

  OnTraversalEnd(result);
  return result;
}

FocusManager::TraversalResult FocusManager::DoTraversalInternal(
    Direction direction, TraversalType traversal_type) {
  TraversalResult result;
  result.direction = direction;
  if (focusable_views_.empty() || direction == Direction::kUnknown) {
    return result;
  }

  if (current_focus_view_ && current_focus_view_->IsFocusScope()) {
    TraversalResult res =
        current_focus_view_->OnTraversalOnScope(direction, traversal_type);
    if (res.succeed || (current_focus_view_->IsFocusFence() &&
                        !current_focus_view_->allow_escape())) {
      result.succeed = true;
      return result;
    }
  }

  if (!focusable_) {
    return result;
  }

  UpdateFocusRect();
  FocusNode* next_focus_view = FindNextFocusView(direction, traversal_type);
  if (!next_focus_view) {
    return result;
  }

  if (next_focus_view->IsFocusFence()) {
    next_focus_view->OnFocusEntering(direction);
    // Focus fence always has priority to deal with focus.
    result.succeed = true;
    return result;
  }
  next_focus_view->RequestFocus();

  // Check if a node self cannot be focused while doing traversal, continue
  // traversal into its children.
  if (next_focus_view->IsFocusScope() &&
      next_focus_view->GetFocusBehavior() == FocusBehavior::kStepIntoChild) {
    result = next_focus_view->OnTraversalOnScope(direction, traversal_type);
    if (result.succeed == true) {
      // Keep current focus view in current scope.
      return result;
    } else {
      // Find next again.
      next_focus_view = FindNextFocusView(direction, traversal_type);
    }
  }

  if (!next_focus_view) {
    return result;
  }
  next_focus_view->RequestFocus();
  FML_DCHECK(current_focus_view_ == next_focus_view);

  result.succeed = true;
  return result;
}

void FocusManager::OnTraversalEnd(TraversalResult result) {
  if (HasFocusedNode() && current_focus_view_->IsFocusScope()) {
    current_focus_view_->GetFocusManager()->OnTraversalEnd(result);
  }
}

void FocusManager::SortViews(Direction direction) {
  bool vertical =
      (direction == Direction::kUp || direction == Direction::kDown);
  bool forward =
      (direction == Direction::kDown || direction == Direction::kRight);

  const Axis axis =
      (direction == Direction::kLeft || direction == Direction::kRight)
          ? Axis::kX
          : Axis::kY;

  focusable_views_.sort([vertical, forward, axis, this](FocusNode* a,
                                                        FocusNode* b) {
    if (!a) {
      return false;
    }
    if (!b) {
      return true;
    }

    if (a->GetFocusIndex(axis) != b->GetFocusIndex(axis)) {
      return forward ? a->GetFocusIndex(axis) < b->GetFocusIndex(axis)
                     : a->GetFocusIndex(axis) > b->GetFocusIndex(axis);
    }
    FloatRect a_rect = a->focus_rect();
    FloatRect b_rect = b->focus_rect();
    if (vertical) {
      if (forward) {
        return a_rect.y() == b_rect.y() ? a_rect.x() < b_rect.x()
                                        : a_rect.y() < b_rect.y();
      } else {
        return a_rect.MaxY() == b_rect.MaxY()
                   ? (preference_.pick_left_when_same_y
                          ? a_rect.x() < b_rect.x()
                          : a_rect.x() > b_rect.x())
                   : b_rect.MaxY() < a_rect.MaxY();
      }
    } else {
      if (forward) {
        return a_rect.x() == b_rect.x() ? a_rect.y() < b_rect.y()
                                        : a_rect.x() < b_rect.x();
      } else {
        return a_rect.MaxX() == b_rect.MaxX() ? a_rect.y() > b_rect.y()
                                              : b_rect.MaxX() < a_rect.MaxX();
      }
    }
    return true;
  });
}

void FocusManager::SortViewsByCenter(Direction direction) {
  bool forward =
      (direction == Direction::kDown || direction == Direction::kRight);
  focusable_views_.sort([forward, direction](FocusNode* a, FocusNode* b) {
    if (!a) {
      return false;
    }
    if (!b) {
      return true;
    }

    const Axis axis =
        (direction == Direction::kLeft || direction == Direction::kRight)
            ? Axis::kX
            : Axis::kY;
    if (a->GetFocusIndex(axis) != b->GetFocusIndex(axis)) {
      return forward ? a->GetFocusIndex(axis) < b->GetFocusIndex(axis)
                     : a->GetFocusIndex(axis) > b->GetFocusIndex(axis);
    }

    FloatPoint a_center = a->focus_rect().Center();
    FloatPoint b_center = b->focus_rect().Center();
    if ((direction == FocusManager::Direction::kUp ||
         direction == FocusManager::Direction::kDown) &&
        (a_center.y() == b_center.y())) {
      return a_center.x() < a_center.x();
    } else if ((direction == FocusManager::Direction::kLeft ||
                direction == FocusManager::Direction::kRight) &&
               (a_center.x() == b_center.x())) {
      return a_center.y() < a_center.y();
    }

    switch (direction) {
      case FocusManager::Direction::kLeft:
        return a_center.x() > b_center.x();
      case FocusManager::Direction::kRight:
        return a_center.x() < b_center.x();
      case FocusManager::Direction::kUp:
        return a_center.y() > b_center.y();
      case FocusManager::Direction::kDown:
      default:
        return a_center.y() < b_center.y();
    }
    return true;
  });
}

FocusNode* FocusManager::FindNextFocusView(Direction direction,
                                           TraversalType traversal_type) {
  if (traversal_type == TraversalType::kDefault) {
    SortViews(direction);
  } else if (traversal_type == TraversalType::kCenter) {
    SortViewsByCenter(direction);
  }

  const Axis axis =
      (direction == Direction::kLeft || direction == Direction::kRight)
          ? Axis::kX
          : Axis::kY;
  // Filter unfocusable node out.
  std::list<FocusNode*> focusable_views_when_move;
  std::copy_if(
      focusable_views_.begin(), focusable_views_.end(),
      std::back_inserter(focusable_views_when_move),
      [axis](FocusNode* node) { return node->CanAcceptFocusOnAxis(axis); });

  // in root scope, visibleRect is invalid. return the first node.
  if (!current_focus_view_ && is_root_) {
    return focusable_views_when_move.front();
  }

  FloatRect visibleRect;
  if (focus_scope_node_) {
    visibleRect = focus_scope_node_->GetContentVisibleRect();
  }
  if (!current_focus_view_ && first_node_expand_ratio_ > 0.f) {
    switch (direction) {
      case FocusManager::Direction::kLeft:
        visibleRect.Expand(0, 0, 0,
                           visibleRect.width() * first_node_expand_ratio_);
        break;
      case FocusManager::Direction::kRight:
        visibleRect.Expand(0, visibleRect.width() * first_node_expand_ratio_, 0,
                           0);
        break;
      case FocusManager::Direction::kUp:
        visibleRect.Expand(visibleRect.height() * first_node_expand_ratio_, 0,
                           0, 0);
        break;
      case FocusManager::Direction::kDown:
        visibleRect.Expand(0, 0,
                           visibleRect.height() * first_node_expand_ratio_, 0);
        break;
      default:
        break;
    }
  }

  // Forward iterator to the next candidate.
  auto it = focusable_views_when_move.begin();
  if (!current_focus_view_ || current_focus_view_->CanAcceptFocusOnAxis(axis)) {
    // Find next focus node.
    for (; it != focusable_views_when_move.end(); ++it) {
      if (!current_focus_view_) {
        FML_DCHECK(!visibleRect.IsEmpty());
        if (!visibleRect.Intersects((*it)->focus_rect())) {
          continue;
        } else {
          // If there's no focused view, and this node is just in rect.
          return (*it);
        }
      } else if (current_focus_view_ == *it) {
        it++;
        break;
      }
    }
  }

  if (it == focusable_views_when_move.end()) {
    return nullptr;
  }

  bool is_vertical =
      (direction == Direction::kUp || direction == Direction::kDown);
  bool is_horizontal =
      (direction == Direction::kLeft || direction == Direction::kRight);
  FloatRect current_focus_rect = current_focus_view_->focus_rect();
  FocusNode* next_focus_view = nullptr;
  // The first candidate focus node
  FocusNode* candidate = nullptr;
  for (; it != focusable_views_when_move.end(); ++it) {
    int focus_index = (*it)->GetFocusIndex(axis);
    if (focus_index < 0) {
      // As sorted, all nodes following are < 0.
      break;
    }
    FloatRect rect = (*it)->focus_rect();

    if (focus_index == current_focus_view_->GetFocusIndex(axis)) {
      bool outside_band =
          (is_vertical && (rect.MaxX() <= current_focus_rect.x() ||
                           rect.x() >= current_focus_rect.MaxX())) ||
          (is_horizontal && (rect.MaxY() <= current_focus_rect.y() ||
                             rect.y() >= current_focus_rect.MaxY()));
      if ((traversal_type == TraversalType::kDefault ||
           traversal_type == TraversalType::kCenter) &&
          outside_band) {
        if (!candidate) {
          candidate = *it;
        }
        continue;
      }

      bool outside_visible_band =
          (is_vertical && (rect.MaxX() <= visibleRect.x() ||
                           rect.x() >= visibleRect.MaxX())) ||
          (is_horizontal &&
           (rect.MaxY() <= visibleRect.y() || rect.y() >= visibleRect.MaxY()));
      if (traversal_type == TraversalType::kNearest && outside_visible_band) {
        if (!candidate) {
          candidate = *it;
        }
        continue;
      }
    }

    if (next_focus_view == nullptr) {
      if (candidate && current_focus_view_->GetFocusIndex(axis) !=
                           candidate->GetFocusIndex(axis)) {
        // Though there's no suitable node within the band, but there is a
        // customized focus sequence node. For example, there 4 nodes:
        //   [0,2] [0,-1]
        //   [0,0] [1,1] <--
        // Now focus is at [1,1], and ArrowDown pressed, the sorted view may be
        // [1,1], [0, 2], [0, 0], [0, -1]
        // so the closest node is [0, 2], but it is outside the vertical band.
        // So it would be candidate and because of 2 > 1, it is the next focus
        // view.
        next_focus_view = candidate;
        break;
      }
      next_focus_view = *it;
      if (preference_.pick_first) {
        break;
      } else {
        continue;
      }
    }
    if (next_focus_view->GetFocusIndex(axis) != focus_index) {
      // Stop forward as focus_index changed.
      break;
    }
    FloatRect pri_rect = next_focus_view->focus_rect();
    if ((direction == Direction::kUp && pri_rect.MaxY() - rect.MaxY() > 0.1f) ||
        (direction == Direction::kDown && rect.y() - pri_rect.y() > 0.1f) ||
        (direction == Direction::kLeft &&
         pri_rect.MaxX() - rect.MaxX() > 0.1f) ||
        (direction == Direction::kRight && rect.x() - pri_rect.x() > 0.1f)) {
      break;
    }

    FloatPoint pri_distance = pri_rect.Center() - current_focus_rect.Center();
    FloatPoint distance = rect.Center() - current_focus_rect.Center();

    if ((is_vertical && std::abs(distance.x()) < std::abs(pri_distance.x())) ||
        (is_horizontal &&
         std::abs(distance.y()) < std::abs(pri_distance.y()))) {
      next_focus_view = *it;
    }
  }

  return next_focus_view;
}

FocusNode* FocusManager::FindFocusViewById(const std::string& id) {
  for (auto node : focusable_views_) {
    if (node->FocusId() == id) {
      return node;
    } else if (node->IsFocusScope()) {
      if (auto manager = node->GetFocusManager()) {
        if (auto found = manager->FindFocusViewById(id)) {
          return found;
        }
      }
    }
  }
  return nullptr;
}

FocusNode* FocusManager::GetLeafFocusedNode() {
  FocusNode* focus_node = current_focus_view_;
  while (focus_node && focus_node->IsFocusScope()) {
    FocusManager* manager = focus_node->GetFocusManager();
    if (!manager || !manager->current_focus_view_) {
      break;
    }
    focus_node = manager->current_focus_view_;
  }
  return focus_node;
}

FloatRect FocusManager::GetFocusedNodeRect() {
  if (!current_focus_view_ || !focus_scope_node_) {
    return FloatRect();
  }
  FloatRect visible_rect = current_focus_view_->GetContentVisibleRect();
  current_focus_view_->UpdateFocusRect();
  FloatRect focus_rect = current_focus_view_->focus_rect();
  focus_rect.Move(-visible_rect.x(), -visible_rect.y());
  return focus_rect;
}

void FocusManager::UpdateFocusRect() {
  for (auto& focusable_view : focusable_views_) {
    focusable_view->UpdateFocusRect();
  }
}

bool FocusManager::OnKeyEvent(const KeyEvent* event) {
  bool handled = false;
  if (current_focus_view_ && current_focus_view_->IsFocusScope()) {
    handled = current_focus_view_->GetFocusManager()->OnKeyEvent(event);
  } else if (current_focus_view_) {
    handled = current_focus_view_->DispatchKeyEventOnFocusNode(event);
  }
  if (!handled && focus_scope_node_) {
    handled = focus_scope_node_->DispatchKeyEventOnFocusNode(event);
  }
  return handled;
}

void FocusManager::DisableFocusUntilNextFrame() {
  disable_focus_until_time_ =
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds() + 1000;
}

void FocusManager::RestoreFocusSwitching() { disable_focus_until_time_ = 0; }

}  // namespace clay
