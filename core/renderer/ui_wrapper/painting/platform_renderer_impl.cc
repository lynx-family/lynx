// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

#include <algorithm>
#include <utility>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx::tasm {

namespace {

bool IsOverlayTag(const base::String& tag) {
  return tag.IsEquals("overlay") || tag.IsEquals("x-overlay-ng");
}

void CopyMetrics(const float* source, float target[4]) {
  if (source == nullptr) {
    std::fill_n(target, 4, 0.f);
    return;
  }
  std::copy_n(source, 4, target);
}

}  // namespace

PlatformRendererImpl::PlatformRendererImpl(int id, PlatformRendererType type,
                                           const base::String& tag)
    : id_(id), type_(type), tag_name_(tag), opacity_{} {
  is_platform_extended_renderer_ =
      (type_ == PlatformRendererType::kUnknown && !tag_name_.empty());
  is_overlay_ = IsOverlayTag(tag_name_);
}

PlatformRendererImpl::~PlatformRendererImpl() {
  for (const auto& child : children_) {
    auto* child_impl = static_cast<PlatformRendererImpl*>(child.get());
    if (child_impl != nullptr && child_impl->parent_ == this) {
      child_impl->parent_ = nullptr;
      child_impl->is_ui_owner_child_ = false;
    }
  }
}

base::String PlatformRendererImpl::GetExtendedRendererTagName() const {
  if (!tag_name_.empty()) {
    return tag_name_;
  }
  switch (type_) {
    case PlatformRendererType::kScroll:
      return base::String(BASE_STATIC_STRING(kElementScrollViewTag));
    case PlatformRendererType::kList:
      return base::String(BASE_STATIC_STRING(kElementListContainerTag));
    case PlatformRendererType::kListItem:
      return base::String(BASE_STATIC_STRING(kElementListItemTag));
    case PlatformRendererType::kView:
      return base::String(BASE_STATIC_STRING(kElementViewTag));
    case PlatformRendererType::kImage:
      return base::String(BASE_STATIC_STRING(kElementImageTag));
    case PlatformRendererType::kText:
      return base::String(BASE_STATIC_STRING(kElementTextTag));
    case PlatformRendererType::kPage:
      return base::String(BASE_STATIC_STRING(kElementPageTag));
    default:
      return base::String();
  }
}

bool PlatformRendererImpl::ShouldUpdateUIOwnerForChild(
    const PlatformRendererImpl& child) const {
  return child.fragment_parent_id_ == id_ && is_platform_extended_renderer_ &&
         (child.is_platform_extended_renderer_ ||
          child.is_direct_child_of_compatible_component_);
}

void PlatformRendererImpl::UpdateDisplayList(DisplayList display_list) {
  // Call platform-specific implementation
  UpdateSubtreeProperty(display_list);
  OnUpdateDisplayList(std::move(display_list));
}

void PlatformRendererImpl::UpdateAttributes(
    const fml::RefPtr<PropBundle>& attributes, bool tends_to_flatten) {
  OnUpdateAttributes(attributes, tends_to_flatten);
}

void PlatformRendererImpl::UpdateLayoutMetrics(float left, float top,
                                               float width, float height,
                                               const float* paddings,
                                               const float* margins,
                                               const float* borders) {
  layout_frame_[0] = left;
  layout_frame_[1] = top;
  layout_frame_[2] = width;
  layout_frame_[3] = height;
  CopyMetrics(paddings, layout_paddings_);
  CopyMetrics(margins, layout_margins_);
  CopyMetrics(borders, layout_borders_);
  has_layout_metrics_ = true;
}

void PlatformRendererImpl::AddChild(fml::RefPtr<PlatformRenderer> child,
                                    int index) {
  if (!child) {
    return;
  }

  auto* child_impl = static_cast<PlatformRendererImpl*>(child.get());
  if (child_impl->parent_ != nullptr) {
    // Child already has a parent, remove it first
    child_impl->RemoveFromParent();
  }

  const bool should_append =
      index < 0 || static_cast<size_t>(index) >= children_.size();
  const int insert_index = should_append ? -1 : index;
  const bool should_update_ui_owner = ShouldUpdateUIOwnerForChild(*child_impl);

  // Set parent relationship
  child_impl->parent_ = this;
  child_impl->is_ui_owner_child_ = should_update_ui_owner;

  // Call platform-specific implementation
  OnAddChild(child.get(), insert_index, should_update_ui_owner);

  // Add to children list
  if (should_append) {
    children_.push_back(std::move(child));
  } else {
    children_.insert(children_.begin() + index, std::move(child));
  }
}

void PlatformRendererImpl::RemoveFromParent() {
  if (parent_ == nullptr) {
    return;
  }

  PlatformRendererImpl* parent = parent_;
  const bool should_update_ui_owner = is_ui_owner_child_;

  // Call platform-specific implementation
  OnRemoveFromParent(should_update_ui_owner);

  // Remove from parent's children list
  auto& siblings = parent->children_;
  auto it = std::find_if(siblings.begin(), siblings.end(),
                         [this](const fml::RefPtr<PlatformRenderer>& child) {
                           return child.get() == this;
                         });

  if (it != siblings.end()) {
    siblings.erase(it);
  }

  // Clear parent relationship
  parent_ = nullptr;
  is_ui_owner_child_ = false;
}

void PlatformRendererImpl::ReleaseSelf() const { delete this; }

void PlatformRendererImpl::UpdateSubtreeProperty(
    const DisplayList& display_list) {
  for (size_t i = 0; i < display_list.GetSubtreePropertiesSize(); i++) {
    const SubtreeProperty* p = display_list.GetSubtreePropertiesData() + i;
    switch (p->type) {
      case DisplayListSubtreePropertyOpType::kOpacity:
        opacity_ = *p;
        break;
      case DisplayListSubtreePropertyOpType::kTransform:
        *transform_ = *p;
        break;
      case DisplayListSubtreePropertyOpType::kFilter:
        break;
    }
  }
  OnUpdateSubtreeProperties(display_list);
}

}  // namespace lynx::tasm
