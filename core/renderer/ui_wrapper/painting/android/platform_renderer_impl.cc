// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_impl.h"

#include <utility>

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx::tasm {

void PlatformRendererImpl::UpdateDisplayList(const DisplayList& display_list) {
  if (!IsValid()) {
    return;
  }

  // Call platform-specific implementation
  OnUpdateDisplayList(display_list);

  // Update all children
  for (auto& child : children_) {
    if (child->IsValid()) {
      child->UpdateDisplayList(display_list);
    }
  }
}

void PlatformRendererImpl::AddChild(std::unique_ptr<PlatformRenderer> child) {
  if (!IsValid() || !child) {
    return;
  }

  auto* child_impl = static_cast<PlatformRendererImpl*>(child.get());
  if (child_impl->parent_ != nullptr) {
    // Child already has a parent, remove it first
    child_impl->RemoveFromParent();
  }

  // Set parent relationship
  child_impl->parent_ = this;

  // Call platform-specific implementation
  OnAddChild(child.get());

  // Add to children list
  children_.push_back(std::move(child));
}

void PlatformRendererImpl::RemoveFromParent() {
  if (!IsValid() || parent_ == nullptr) {
    return;
  }

  // Call platform-specific implementation
  OnRemoveFromParent();

  // Remove from parent's children list
  auto& siblings = parent_->children_;
  auto it =
      std::find_if(siblings.begin(), siblings.end(),
                   [this](const std::unique_ptr<PlatformRenderer>& child) {
                     return child.get() == this;
                   });

  if (it != siblings.end()) {
    siblings.erase(it);
  }

  // Clear parent relationship
  parent_ = nullptr;
}

}  // namespace lynx::tasm
