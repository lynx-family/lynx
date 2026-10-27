// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_adapter_updater.h"

#include <limits>

#include "clay/fml/logging.h"
#include "clay/gfx/animation/value_animator.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/list/base_list_view.h"

namespace clay {

static constexpr int64_t kUpdateAnimationDuration = 300;  // ms

ListAdapterUpdater::ListAdapterUpdater(BaseListView* list_view)
    : list_view_(list_view) {
  FML_DCHECK(list_view);
  animator_ = std::make_unique<ValueAnimator>();
  animator_->SetDuration(kUpdateAnimationDuration);
  animator_->SetAnimationHandler(list_view->GetAnimationHandler());
  animator_->AddUpdateListener(this);
}

ListAdapterUpdater::~ListAdapterUpdater() = default;

void ListAdapterUpdater::SaveItemStartAttribute(ListItemViewHolder* item) {
  auto& pair = animations_[item];
  pair.first = {item->GetLayoutOrigin(), item->GetView()->Opacity()};
}

void ListAdapterUpdater::SaveItemEndAttribute(ListItemViewHolder* item) {
  auto itr = animations_.find(item);
  if (itr != animations_.end()) {
    auto& pair = animations_[item];
    pair.second = {item->GetLayoutOrigin(), item->GetView()->Opacity()};
    FloatPoint from = pair.first.value().origin;
    item->Layout(from);
    return;
  }
  auto pos = item->GetPosition();
  if (insert_positions_.find(pos) != insert_positions_.end()) {
    FloatPoint origin = item->GetLayoutOrigin();
    float opacity = item->GetView()->Opacity();
    auto& pair = animations_[item];
    pair.first = {origin, 0.f};
    pair.second = {origin, opacity};
    insert_positions_.erase(pos);
  }
}

void ListAdapterUpdater::AddInsertItemPosition(int pos) {
  insert_positions_.insert(pos);
}

void ListAdapterUpdater::CompactAnimations() {
  for (auto itr = animations_.begin(); itr != animations_.end();) {
    auto& attr_pair = itr->second;
    if (!attr_pair.second.has_value()) {
      itr = animations_.erase(itr);
    } else {
      itr++;
    }
  }
}

void ListAdapterUpdater::PerformUpdate() {
  CompactAnimations();
  animator_->Start();
}

void ListAdapterUpdater::EndUpdate() { animator_->End(); }

void ListAdapterUpdater::OnAnimationUpdate(ValueAnimator& animator) {
  const float fraction = animator.GetAnimatedFraction();
  auto animation_block = [this](const float fraction) {
    for (auto& itr : animations_) {
      auto* view_holder = itr.first;
      auto& attr = itr.second;
      FloatPoint from = attr.first.value().origin;
      FloatPoint to = attr.second.value().origin;
      float origin_opacity = attr.first.value().opacity;
      float target_opacity = attr.second.value().opacity;
      float deltaX = (to.x() - from.x()) * fraction;
      float deltaY = (to.y() - from.y()) * fraction;
      float delta_opacity = target_opacity - origin_opacity;
      from.Move(deltaX, deltaY);
      view_holder->Layout(from);
      view_holder->GetView()->SetOpacity(origin_opacity +
                                         delta_opacity * fraction);
      view_holder->FlushLayout();
    }
  };
  if (fraction == 0.f && !enter_update_) {
    enter_update_ = true;
    list_view_->page_view()->RequestPaint();
  } else if (fraction == 1.f) {
    enter_update_ = false;
    animation_block(1.f);
    insert_positions_.clear();
    animations_.clear();
  } else {
    animation_block(fraction);
  }
}

}  // namespace clay
