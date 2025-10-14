// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/focus_node.h"

#include "clay/fml/logging.h"

namespace clay {

FocusNode::FocusNode() = default;
FocusNode::~FocusNode() {
  if (life_listeners_) {
    for (auto* listener : *life_listeners_) {
      listener->OnFocusNodeDestructed(this);
    }
  }
}

FocusManager* FocusNode::GetFocusManager() {
  if (IsFocusScope()) {
    return focus_manager_.get();
  }
  return GetParentFocusManager();
}

void FocusNode::SetFocusable(bool focusable) {
  if (pending_focusable_ == focusable) {
    return;
  }
  pending_focusable_ = focusable;
  SetFocusableInternal(pending_focusable_);
}

void FocusNode::RequestFocus(bool is_leaf) {
  if (!focusable_ || is_focused_) {
    if (!focusable_ && pending_focusable_) {
      pending_request_focus_ = true;
    }
    return;
  }
  is_focused_ = true;
  SetHasDefaultFocusRing(is_focused_);
  FocusManager* manager = GetParentFocusManager();
  if (manager) {
    manager->RequestFocus(this);
  }
  if (IsFocusScope()) {
    focus_manager_->SetFocusable(true);
  }
  if (!restoring_focus_) {
    FocusHasChanged(is_focused_, is_leaf);
  }
}

void FocusNode::ClearFocus() {
  if (!focusable_ || !is_focused_) {
    return;
  }
  is_focused_ = false;
  SetHasDefaultFocusRing(is_focused_);
  FocusManager* manager = GetParentFocusManager();
  if (manager) {
    manager->ClearFocus(this);
  }
  if (IsFocusScope()) {
    focus_manager_->SetFocusable(false);
  }
  FocusHasChanged(is_focused_, true);
}

void FocusNode::SetIsFocusScope() {
  focus_manager_ = std::make_unique<FocusManager>(this);
}

void FocusNode::SetIsFocusFence() {
  FML_DCHECK(IsFocusScope());
  is_fence_ = true;
}

void FocusNode::UpdateFocusRect() {
  focus_rect_ = CalcFocusRect();
  // focus-isolate use self rect rather than child's.
  if (IsFocusScope() && IsFocused() && GetFocusManager()->HasFocusedNode() &&
      !IsFocusFence()) {
    FloatRect rect = GetFocusManager()->GetFocusedNodeRect();
    FloatSize offset = GetThicknessOffset();
    rect.Move(offset.width(), offset.height());
    rect.Move(focus_rect_.x(), focus_rect_.y());
    focus_rect_ = rect;
  }
}

void FocusNode::OnFocusAttach() {
  SetFocusableInternal(pending_focusable_);
  restoring_focus_ = false;
}

void FocusNode::OnFocusDetach() {
  restoring_focus_ = is_focused_ || pending_request_focus_;
  SetFocusableInternal(false);
  pending_request_focus_ = restoring_focus_;
}

int FocusNode::GetFocusIndex(Axis axis) const {
  switch (axis) {
    case Axis::kX:
      return focus_index_.x();
    case Axis::kY:
      return focus_index_.y();
    default:
      FML_DCHECK(false);
      return 0;
  }
}

bool FocusNode::CanAcceptFocusOnAxis(Axis axis) const {
  // TODO(yulitao): Skip invisible nodes.
  switch (axis) {
    case Axis::kX:
      return focus_index_.x() >= 0;
    case Axis::kY:
      return focus_index_.y() >= 0;
    default:
      FML_DCHECK(false);
      return true;
  }
}

void FocusNode::SetFocusableInternal(bool focusable) {
  if (focusable == focusable_) {
    return;
  }
  FocusManager* manager = GetParentFocusManager();
  if (!manager) {
    return;
  }
  if (focusable) {
    manager->RegisterFocusableView(this);
  } else {
    ClearFocus();
    manager->UnregisterFocusableView(this);
    if (life_listeners_) {
      auto life_listeners_copy = *life_listeners_;
      for (auto* listener : life_listeners_copy) {
        listener->OnFocusNodeUnregistered(this);
      }
    }
  }
  focusable_ = focusable;
  if (focusable_ && pending_request_focus_) {
    pending_request_focus_ = false;
    // If focus has already set, then no need to restore.
    // And list items may reuse, it also may cause some problem.
    if (!GetFocusManager()->GetRootFocusManager()->GetLeafFocusedNode()) {
      RequestFocus();
    }
  }
}

FocusManager::TraversalResult FocusNode::OnTraversalOnScope(
    FocusManager::Direction direction,
    FocusManager::TraversalType traversal_type) {
  if (!IsFocusScope()) {
    return FocusManager::TraversalResult();
  }
  FocusManager::TraversalResult res =
      GetFocusManager()->DoTraversalInternal(direction, traversal_type);
  UpdateFocusRing();
  return res;
}

void FocusNode::UpdateFocusRing() {
  if (focus_manager_->HasFocusedNode()) {
    SetHasDefaultFocusRing(false);
  } else if (is_focused_) {
    SetHasDefaultFocusRing(true);
  }
}

const std::string& FocusNode::GetNextFocusId(
    FocusManager::Direction direction) const {
  switch (direction) {
    case FocusManager::Direction::kUp:
      return next_focus_up_;
    case FocusManager::Direction::kDown:
      return next_focus_down_;
    case FocusManager::Direction::kLeft:
      return next_focus_left_;
    default:
      FML_DCHECK(false);
    case FocusManager::Direction::kRight:
      return next_focus_right_;
  }
}

}  // namespace clay
