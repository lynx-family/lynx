// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/list/list_container/list_container_view.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

class ListContainerViewTest : public UITest {};

TEST_F_UI(ListContainerViewTest,
          ContentSizeUpdateAppliesVerticalDeltaToPreviousOffset) {
  auto list = std::make_unique<ListContainerView>(-1, page_.get(), -1);
  page_->AddChild(list.get());
  list->SetBound(0.f, 0.f, 100.f, 100.f);

  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, list->GetScrollOffset().y());

  list->UpdateContentOffsetForListContainer(250.f, 0.f, -50.f);
  EXPECT_FLOAT_EQ(150.f, list->GetScrollOffset().y());
}

TEST_F_UI(ListContainerViewTest, ContentSizeUpdateClampsOffsetWithoutDelta) {
  auto list = std::make_unique<ListContainerView>(-1, page_.get(), -1);
  page_->AddChild(list.get());
  list->SetBound(0.f, 0.f, 100.f, 100.f);

  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, list->GetScrollOffset().y());

  list->UpdateContentOffsetForListContainer(200.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(100.f, list->GetScrollOffset().y());
}

TEST_F_UI(ListContainerViewTest,
          ContentSizeUpdateAppliesHorizontalDeltaToPreviousOffset) {
  auto list = std::make_unique<ListContainerView>(-1, page_.get(), -1);
  page_->AddChild(list.get());
  list->SetScrollDirection(ScrollDirection::kHorizontal);
  list->SetBound(0.f, 0.f, 100.f, 100.f);

  list->UpdateContentOffsetForListContainer(300.f, 200.f, 0.f);
  EXPECT_FLOAT_EQ(200.f, list->GetScrollOffset().x());

  list->UpdateContentOffsetForListContainer(250.f, -50.f, 0.f);
  EXPECT_FLOAT_EQ(150.f, list->GetScrollOffset().x());
}

// Load-more during overscroll: the user pulled past the bottom edge, then new
// data grows the content. The rubber-band displacement that sat beyond the old
// max range must be promoted into a now-valid scroll offset (consumed), instead
// of snapping the viewport back to the pre-resize offset. This is the case an
// unconditional "always use pre-resize offset" fix regresses (it would leave
// the result at exactly 200 and the page would jump back).
TEST_F_UI(ListContainerViewTest, ContentGrowthDuringOverscrollConsumesOffset) {
  auto list = std::make_unique<ListContainerView>(-1, page_.get(), -1);
  page_->AddChild(list.get());
  list->SetOverscrollEnabled(true);
  list->SetBound(0.f, 0.f, 100.f, 100.f);

  // Scroll to the bottom of a 300 tall content (max scroll = 300 - 100 = 200).
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, list->GetScrollOffset().y());

  // Simulate pulling past the bottom edge. This moves the render (paint) offset
  // beyond the old max range while the logical scroll offset stays clamped.
  list->SetOverscrollOffset({0.f, 200.f});
  EXPECT_TRUE(list->IsUnderOverscroll());
  EXPECT_FLOAT_EQ(200.f, list->GetScrollOffset().y());

  // New data arrives: content grows to 400 (new max scroll = 300), no anchor
  // delta. The previously out-of-range paint displacement is now valid scroll.
  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);

  // The consumed overscroll must be preserved: strictly greater than the
  // pre-resize 200 (the buggy unconditional fix yields exactly 200), and still
  // within the new max range.
  EXPECT_GT(list->GetScrollOffset().y(), 200.f);
  EXPECT_LE(list->GetScrollOffset().y(), 300.f);
}

// Content shrink during overscroll: even if a resize happens mid-overscroll,
// a shrink with a negative delta must not be clamped twice. The delta is
// relative to the pre-resize logical offset, so we must apply it from there
// (not from the offset already corrected by SetMaxContent).
TEST_F_UI(ListContainerViewTest,
          ContentShrinkDuringOverscrollAvoidsDoubleClamp) {
  auto list = std::make_unique<ListContainerView>(-1, page_.get(), -1);
  page_->AddChild(list.get());
  list->SetOverscrollEnabled(true);
  list->SetBound(0.f, 0.f, 100.f, 100.f);

  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, list->GetScrollOffset().y());

  list->SetOverscrollOffset({0.f, 200.f});
  EXPECT_TRUE(list->IsUnderOverscroll());

  // Close a 50 tall item: content shrinks to 250 (new max = 150), delta = -50
  // relative to the pre-resize offset of 200 -> 150. If the shrink were applied
  // on top of the SetMaxContent-corrected offset, the result would be 100 and a
  // 50px blank strip would appear at the bottom.
  list->UpdateContentOffsetForListContainer(250.f, 0.f, -50.f);
  EXPECT_FLOAT_EQ(150.f, list->GetScrollOffset().y());
}

// Horizontal symmetry for the load-more-during-overscroll case.
TEST_F_UI(ListContainerViewTest,
          HorizontalContentGrowthDuringOverscrollConsumesOffset) {
  auto list = std::make_unique<ListContainerView>(-1, page_.get(), -1);
  page_->AddChild(list.get());
  list->SetScrollDirection(ScrollDirection::kHorizontal);
  list->SetOverscrollEnabled(true);
  list->SetBound(0.f, 0.f, 100.f, 100.f);

  list->UpdateContentOffsetForListContainer(300.f, 200.f, 0.f);
  EXPECT_FLOAT_EQ(200.f, list->GetScrollOffset().x());

  list->SetOverscrollOffset({200.f, 0.f});
  EXPECT_TRUE(list->IsUnderOverscroll());

  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
  EXPECT_GT(list->GetScrollOffset().x(), 200.f);
  EXPECT_LE(list->GetScrollOffset().x(), 300.f);
}

}  // namespace clay
