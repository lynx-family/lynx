// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_item_view_holder.h"

#include <sstream>
#include <string>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_adapter.h"

namespace clay {

ListItemViewHolder::ListItemViewHolder() = default;
ListItemViewHolder::~ListItemViewHolder() = default;

void ListItemViewHolder::ResetOnRecycle() {
  if (IsRemoved() || NeedsUpdate() || IsInvalid()) {
    Reset();
  }

  // End all transition animations to avoid remaining dirty animations after
  // reuse. We won't end keyframe animations here, as this may result in the
  // animations not being resumed.
  if (auto view = GetView()) {
    view->EndAllTransitionsRecursively();
  }
}

void ListItemViewHolder::Reset() {
  position_ = kNoPosition;
  flags_ = kFlagNone;
  ClearPayloads();
}

void ListItemViewHolder::SetView(BaseView* view) {
  if (view) {
    FML_DCHECK(!view_);
    view_ = view;
  } else {
    FML_DCHECK(view_);
    view_ = nullptr;
  }
}

float ListItemViewHolder::GetTop() const {
  if (layout_location_) {
    return layout_location_->y();
  }

  if (GetView() == nullptr) {
    return 0.f;
  }
  return GetView()->Top();
}

float ListItemViewHolder::GetLeft() const {
  if (layout_location_) {
    return layout_location_->x();
  }

  if (GetView() == nullptr) {
    return 0.f;
  }
  return GetView()->Left();
}

float ListItemViewHolder::GetWidth() const {
  if (GetView() == nullptr) {
    return 0.f;
  }
  return GetView()->Width();
}

float ListItemViewHolder::GetHeight() const {
  if (GetView() == nullptr) {
    return 0.f;
  }
  return GetView()->Height();
}

FloatPoint ListItemViewHolder::GetLayoutOrigin() const {
  if (layout_location_) {
    return layout_location_.value();
  }
  auto* view = GetView();
  if (view == nullptr) {
    return {0.f, 0.f};
  } else {
    return {view->Left(), view->Top()};
  }
}

void ListItemViewHolder::SetPosition(int position) {
  if (position > kNoPosition) {
    last_valid_position_ = position;
  }
  position_ = position;
}

void ListItemViewHolder::Layout(const FloatPoint& location) {
  layout_location_ = location;
  ClearAdjustedLocation();
}

void ListItemViewHolder::LayoutWithOffset(const FloatPoint& offset) {
  if (!GetView() || !GetViewAttached()) {
    return;
  }
  if (!layout_location_) {
    layout_location_ = {GetView()->Left(), GetView()->Top()};
  }
  layout_location_->Move(offset.x(), offset.y());
  ClearAdjustedLocation();
}

void ListItemViewHolder::FlushLayout() {
  {
    auto view = GetView();
    // Setting width/height should not dirty the layout bit.
    BaseView::LayoutIgnoreHelper helper(view);

    if (view && GetViewAttached() && layout_location_) {
      // NOTE: The layout result is relative to the padding box of the list. We
      // need to set the location relative to the border box of the list.
      auto parent = view->Parent();
      FML_DCHECK(parent->Is<BaseListView>());
      view->SetX(layout_location_->x() + parent->BorderLeft());
      view->SetY(adjusted_location_.value_or(layout_location_->y()) +
                 parent->BorderTop());
      if (sticky_type_ != ListItemStickyType::kNone) {
        parent->BringChildToFront(view);
      }
    }
  }
}

void ListItemViewHolder::OffsetPosition(int offset, bool apply_to_pre_layout) {
  SetPosition(position_ + offset);
}

void ListItemViewHolder::FlagRemovedAndOffsetPosition(
    int new_position, int offset, bool apply_to_pre_layout) {
  flags_ = static_cast<Flag>(flags_ | kFlagRemoved);
  OffsetPosition(offset, apply_to_pre_layout);
  SetPosition(new_position);
}

void ListItemViewHolder::SetFlags(Flag flags, Flag mask) {
  flags_ = static_cast<Flag>((flags_ & ~mask) | (flags & mask));
}

void ListItemViewHolder::AddFlags(Flag flags) {
  flags_ = static_cast<Flag>(flags_ | flags);
}

void ListItemViewHolder::RemoveFlags(Flag flags) {
  SetFlags(Flag::kFlagNone, flags);
}

void ListItemViewHolder::SetViewVisible(bool visible) {
  if (view_) {
    if (view_->Visible() != visible) {
      view_->SetVisible(visible);
      if (observer_) {
        observer_->OnVisibilityChanged(this, visible);
      }
    }
  }
}

void ListItemViewHolder::AddChangePayload(
    std::unique_ptr<ListAdapter::Payload> payload) {
  if (payload == nullptr) {
    AddFlags(Flag::kFlagAdapterFullUpdate);
  } else if ((flags_ & Flag::kFlagAdapterFullUpdate) == 0) {
    payloads_.emplace_back(std::move(payload));
  }
}

void ListItemViewHolder::ClearPayloads() {
  payloads_.clear();
  flags_ = static_cast<Flag>(flags_ & ~kFlagAdapterFullUpdate);
}

const std::vector<std::unique_ptr<ListAdapter::Payload>>&
ListItemViewHolder::GetPayloads() const {
  if ((flags_ & Flag::kFlagAdapterFullUpdate) == 0) {
    return payloads_;
  } else {
    static const std::vector<std::unique_ptr<ListAdapter::Payload>> kDummy;
    return kDummy;
  }
}

std::string ListItemViewHolder::ToString() const {
#if (DEBUG_LIST)
  std::stringstream ss;
  ss << "pos:" << GetPosition() << " type:" << GetItemViewType()
     << " view_id:" << (view_ != nullptr ? view_->id() : -1)
     << " isAttached:" << (GetViewAttached() ? "true" : "false")
     << " isVisible:" << (GetViewVisible() ? "true" : "false")
     << " isBound:" << (IsBound() ? "true" : "false");
  if (layout_location_) {
    ss << " loc:" << layout_location_->x() << "," << layout_location_->y();
  }
  return ss.str();
#else
  return "";
#endif
}

}  // namespace clay
