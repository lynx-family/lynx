// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_SCROLLER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_SCROLLER_H_

#include <memory>
#include <optional>
#include <string>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/ui/component/list/layout_types.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {
namespace internal {

class ScrollerAnimator;

}  // namespace internal

class BaseListView;
class ListAdapter;
class ListChildrenHelper;
class ListLayoutManager;

// This class handles the smooth scrolling.
class ListScroller {
 public:
  explicit ListScroller(BaseListView* list_view);
  ~ListScroller();

  static AlignTo StringToAlign(const std::string& str);

  void ScrollToPosition(
      bool smooth, int position, float offset, AlignTo align_to,
      const std::string& id, const std::optional<FloatRect> target_rect,
      std::function<void(uint32_t, const std::string&)> callback);
  bool Scrolling() const;
  void StopScroll(bool result = false);

 private:
  FRIEND_TEST(ListLayoutManagerLinearTest, DISABLED_ScrollToPositionSmooth);
  FRIEND_TEST(ListLayoutManagerLinearTest,
              DISABLED_ScrollToPositionSmoothDataChange);

  ListLayoutManager* GetLayoutManager() const;
  ListAdapter* GetAdapter() const;
  ListChildrenHelper* GetChildrenHelper() const;
  AnimationHandler* GetAnimationHandler() const;

  void StartAnimator();
  void StopAnimator();

  bool ScrollImmediately();
  bool OnAnimation(int64_t frame_time);
  void UpdatePositions();
  bool IsFarAwayFromTarget(int to_scroll, float estimated_distance);
  float DistanceToTarget(ListItemViewHolder* target_item);

  bool ComputeScrollOffset(const float& distance, const int64_t& now_time,
                           float& target_offset);

  int ComputeScrollDuration(int dx, int dy);
  int32_t CalculatePxPerFrame() const;

  BaseListView* list_view_ = nullptr;
  int position_ = ListItemViewHolder::kNoPosition;
  std::string id_;
  float offset_ = 0.f;
  AlignTo align_to_ = AlignTo::kNone;
  std::optional<FloatRect> target_rect_;  // Relative to item if has value
  std::function<void(uint32_t, const std::string&)> callback_;

  std::unique_ptr<internal::ScrollerAnimator> animator_;

  int start_position_ = ListItemViewHolder::kNoPosition;
  int end_position_ = ListItemViewHolder::kNoPosition;

  int64_t start_time_;
  std::optional<int64_t> duration_time_;
  std::optional<float> target_distance_;
  float last_distance_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_SCROLLER_H_
