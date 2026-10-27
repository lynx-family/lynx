// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_UPDATER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_UPDATER_H_

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "clay/gfx/animation/animator_listener.h"
#include "clay/gfx/animation/value_animator.h"
#include "clay/gfx/geometry/float_point.h"

namespace clay {

class BaseListView;
class ListItemViewHolder;
class ValueAnimator;

class ListAdapterUpdater : public AnimatorUpdateListener {
 public:
  struct Attribute {
    FloatPoint origin;
    float opacity;
  };
  explicit ListAdapterUpdater(BaseListView* list_view);
  virtual ~ListAdapterUpdater();

  bool IsRunning() { return animator_->IsRunning(); }
  bool hasPendingUpdateAnimation() { return !animations_.empty(); }

  void SaveItemStartAttribute(ListItemViewHolder* item);
  void SaveItemEndAttribute(ListItemViewHolder* item);
  void AddInsertItemPosition(int pos);
  void PerformUpdate();
  void EndUpdate();

  void OnAnimationUpdate(ValueAnimator& animator) override;

 private:
  using AnimationAttributePair =
      std::pair<std::optional<Attribute>, std::optional<Attribute>>;
  void CompactAnimations();
  BaseListView* list_view_ = nullptr;
  std::unordered_map<ListItemViewHolder*, AnimationAttributePair> animations_;
  std::unordered_set<int> insert_positions_;
  std::unique_ptr<ValueAnimator> animator_;
  bool enter_update_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_UPDATER_H_
