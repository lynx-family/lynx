// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/list/list_container/list_container_view.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

namespace {
class RecordingListDelegate : public ListContainerView::Delegate {
 public:
  void OnListViewDidLayout() override {}
  void OnScrollByListContainer(float x, float y, float original_x,
                               float original_y) override {
    offsets.emplace_back(x, y);
    originals.emplace_back(original_x, original_y);
  }
  void OnScrollToPosition(int, float, int, bool) override {}
  void OnScrollStopped() override {}

  std::vector<FloatPoint> offsets;
  std::vector<FloatPoint> originals;
};
}  // namespace

// Correctness suite for the three-offset scroll model
// (see spec/list_scroll_offset_spec.md):
//   * scroll_offset_     - purely logical, always clamped to [0, MaxScroll]
//   * OverscrollOffset() - rubber-band displacement, tracked separately
//   * paint offset       - scroll_offset_ + overscroll, composed at paint
//
// The focus is ListContainerView::UpdateContentOffsetForListContainer, which
// re-applies a ListElement delta after a content-size change without double
// adjusting the viewport.
class ListContainerViewTest : public UITest {
 protected:
  std::unique_ptr<ListContainerView> MakeList(ScrollDirection dir,
                                              float w = 100.f,
                                              float h = 100.f) {
    auto list = std::make_unique<ListContainerView>(-1, page_.get(), -1);
    page_->AddChild(list.get());
    if (dir == ScrollDirection::kHorizontal) {
      list->SetScrollDirection(ScrollDirection::kHorizontal);
    }
    list->SetBound(0.f, 0.f, w, h);
    return list;
  }

  float OffY(const std::unique_ptr<ListContainerView>& l) {
    return l->GetScrollOffset().y();
  }
  float OffX(const std::unique_ptr<ListContainerView>& l) {
    return l->GetScrollOffset().x();
  }
};

// ---------------------------------------------------------------------------
// Quadrant 1: normal (no overscroll) content grow, delta applied to previous.
// ---------------------------------------------------------------------------
TEST_F_UI(ListContainerViewTest, GrowAppliesDeltaToPreviousOffsetVertical) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, OffY(list));
}

TEST_F_UI(ListContainerViewTest, GrowAppliesDeltaToPreviousOffsetHorizontal) {
  auto list = MakeList(ScrollDirection::kHorizontal);
  list->UpdateContentOffsetForListContainer(300.f, 200.f, 0.f);
  EXPECT_FLOAT_EQ(200.f, OffX(list));
}

// A subsequent growth with a positive anchor delta (e.g. head prepend) must be
// applied against the freshly-grown max range, not clamped against the stale
// smaller one.
TEST_F_UI(ListContainerViewTest, GrowWithPositiveAnchorDeltaUsesNewRange) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(250.f, 0.f, 150.f);
  EXPECT_FLOAT_EQ(150.f, OffY(list));
  // content 250 -> 400 (new max 300), anchor delta +100 -> 250, within new max.
  list->UpdateContentOffsetForListContainer(400.f, 0.f, 100.f);
  EXPECT_FLOAT_EQ(250.f, OffY(list));
}

// ---------------------------------------------------------------------------
// Quadrant 2: content shrink with negative delta must not double clamp.
// ---------------------------------------------------------------------------
TEST_F_UI(ListContainerViewTest, ShrinkWithNegativeDeltaNoDoubleClampVertical) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, OffY(list));
  // Close a 50-tall item: content 250 (new max 150), delta -50 -> 150.
  list->UpdateContentOffsetForListContainer(250.f, 0.f, -50.f);
  EXPECT_FLOAT_EQ(150.f, OffY(list));
}

TEST_F_UI(ListContainerViewTest,
          ShrinkWithNegativeDeltaNoDoubleClampHorizontal) {
  auto list = MakeList(ScrollDirection::kHorizontal);
  list->UpdateContentOffsetForListContainer(300.f, 200.f, 0.f);
  EXPECT_FLOAT_EQ(200.f, OffX(list));
  list->UpdateContentOffsetForListContainer(250.f, -50.f, 0.f);
  EXPECT_FLOAT_EQ(150.f, OffX(list));
}

// Shrink with zero delta still clamps the logical offset into the new range.
TEST_F_UI(ListContainerViewTest, ShrinkWithZeroDeltaClampsIntoNewRange) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, OffY(list));
  // content 200 (new max 100), delta 0 -> clamp 200 down to 100.
  list->UpdateContentOffsetForListContainer(200.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(100.f, OffY(list));
}

// ---------------------------------------------------------------------------
// Quadrant 3: overscroll + content grow (load-more) consumes the pending
// rubber-band displacement instead of snapping back to the pre-resize offset.
// ---------------------------------------------------------------------------
TEST_F_UI(ListContainerViewTest, OverscrollGrowConsumesOverscrollVertical) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->SetOverscrollEnabled(true);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, OffY(list));

  // Pull past the bottom edge: logical stays clamped, overscroll records it.
  list->SetOverscrollOffset({0.f, 200.f});
  EXPECT_TRUE(list->IsUnderOverscroll());
  EXPECT_FLOAT_EQ(200.f, OffY(list));

  // Load more: content grows 300 -> 400 (new max 300), delta 0. The consumed
  // displacement must be preserved: strictly past the pre-resize 200.
  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
  EXPECT_GT(OffY(list), 200.f);
  EXPECT_LE(OffY(list), 300.f);
}

TEST_F_UI(ListContainerViewTest, OverscrollGrowConsumesOverscrollHorizontal) {
  auto list = MakeList(ScrollDirection::kHorizontal);
  list->SetOverscrollEnabled(true);
  list->UpdateContentOffsetForListContainer(300.f, 200.f, 0.f);
  EXPECT_FLOAT_EQ(200.f, OffX(list));

  list->SetOverscrollOffset({200.f, 0.f});
  EXPECT_TRUE(list->IsUnderOverscroll());

  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
  EXPECT_GT(OffX(list), 200.f);
  EXPECT_LE(OffX(list), 300.f);
}

// ---------------------------------------------------------------------------
// Quadrant 4: overscroll + content shrink must still not double clamp.
// ---------------------------------------------------------------------------
TEST_F_UI(ListContainerViewTest, OverscrollShrinkNoDoubleClamp) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->SetOverscrollEnabled(true);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  EXPECT_FLOAT_EQ(200.f, OffY(list));

  list->SetOverscrollOffset({0.f, 200.f});
  EXPECT_TRUE(list->IsUnderOverscroll());

  // Shrink 250 (new max 150), delta -50 -> 150 (not 100).
  list->UpdateContentOffsetForListContainer(250.f, 0.f, -50.f);
  EXPECT_FLOAT_EQ(150.f, OffY(list));
}

// ---------------------------------------------------------------------------
// Invariant: when not overscrolled, the logical offset equals the paint offset
// after a content update (three-offset compose is a no-op without overscroll).
// ---------------------------------------------------------------------------
TEST_F_UI(ListContainerViewTest, LogicalEqualsPaintWithoutOverscroll) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 120.f);
  EXPECT_FALSE(list->IsUnderOverscroll());
  const float logical = OffY(list);
  const float paint = list->GetRenderScroll()->GetPaintOffsetForScroll().y();
  EXPECT_FLOAT_EQ(logical, paint);
}

// SetMaxContent clears the overscroll state as part of a content-size change.
TEST_F_UI(ListContainerViewTest, ContentResizeClearsOverscrollState) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->SetOverscrollEnabled(true);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetOverscrollOffset({0.f, 60.f});
  EXPECT_TRUE(list->IsUnderOverscroll());

  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
  EXPECT_FALSE(list->IsUnderOverscroll());
}

// Repeated no-op content updates (same size, zero delta) keep the offset stable
// and never drift.
TEST_F_UI(ListContainerViewTest, RepeatedNoopUpdatesAreStable) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 150.f);
  EXPECT_FLOAT_EQ(150.f, OffY(list));
  for (int i = 0; i < 5; ++i) {
    list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
  }
  EXPECT_FLOAT_EQ(150.f, OffY(list));
}

// Growth from an initial zero offset with zero delta stays at the top.
TEST_F_UI(ListContainerViewTest, GrowFromTopWithZeroDeltaStaysAtTop) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(0.f, OffY(list));
  list->UpdateContentOffsetForListContainer(500.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(0.f, OffY(list));
}

// Negative target (over-correction) is clamped to zero, never negative.
TEST_F_UI(ListContainerViewTest, NegativeTargetClampsToZero) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 50.f);
  EXPECT_FLOAT_EQ(50.f, OffY(list));
  // delta -120 would drive below zero -> clamp to 0.
  list->UpdateContentOffsetForListContainer(300.f, 0.f, -120.f);
  EXPECT_FLOAT_EQ(0.f, OffY(list));
}

TEST_F_UI(ListContainerViewTest, DirectionSwitchResetsOffsetsAndRenderAxis) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->SetOverscrollEnabled(true);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetOverscrollOffset({0.f, 60.f});

  list->SetAttribute("scroll-orientation", clay::Value("horizontal"));
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
  EXPECT_EQ(list->GetScrollDirection(), ScrollDirection::kHorizontal);
  EXPECT_EQ(list->GetRenderScroll()->GetScrollDirection(),
            ScrollDirection::kHorizontal);
  EXPECT_FLOAT_EQ(0.f, OffX(list));
  EXPECT_FLOAT_EQ(0.f, OffY(list));
  EXPECT_FALSE(list->IsUnderOverscroll());
  EXPECT_TRUE(list->ClampedOverscrollOffset().IsOrigin());
  EXPECT_FLOAT_EQ(0.f, list->GetRenderScroll()->ScrollTop());

  list->OnScrollUpdate(80.f);
  EXPECT_FLOAT_EQ(80.f, OffX(list));
  EXPECT_FLOAT_EQ(0.f, OffY(list));
  list->SetAttribute("scroll-orientation", clay::Value("vertical"));
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
  EXPECT_EQ(list->GetRenderScroll()->GetScrollDirection(),
            ScrollDirection::kVertical);
  EXPECT_FLOAT_EQ(0.f, OffX(list));
  EXPECT_FLOAT_EQ(0.f, OffY(list));
}

TEST_F_UI(ListContainerViewTest, BounceAttributesFollowScrollViewPolicy) {
  auto list = MakeList(ScrollDirection::kVertical);
  auto scroll = std::make_unique<ScrollView>(-2, page_.get());
  page_->AddChild(scroll.get());
  for (const char* attribute : {"bounces", "bounce"}) {
    for (bool initial : {false, true}) {
      for (bool value : {false, true}) {
        list->SetOverscrollEnabled(initial);
        scroll->SetOverscrollEnabled(initial);
        list->SetAttribute(attribute, clay::Value(value));
        scroll->SetAttribute(attribute, clay::Value(value));
        EXPECT_EQ(scroll->IsOverscrollEnabled(false),
                  list->IsOverscrollEnabled(false));
        EXPECT_EQ(scroll->IsOverscrollEnabled(true),
                  list->IsOverscrollEnabled(true));
      }
    }
  }
  page_->RemoveChild(scroll.get());
  page_->RemoveChild(list.get());
}

TEST_F_UI(ListContainerViewTest, SameDirectionPreservesOverscroll) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetOverscrollOffset({0.f, 60.f});
  const float paint = list->GetRenderScroll()->ScrollTop();
  list->SetScrollDirection(ScrollDirection::kVertical);
  EXPECT_TRUE(list->IsUnderOverscroll());
  EXPECT_FLOAT_EQ(200.f, OffY(list));
  EXPECT_FLOAT_EQ(paint, list->GetRenderScroll()->ScrollTop());
}

TEST_F_UI(ListContainerViewTest,
          SteadyUpdatePreservesLogicalAndPaintDuringPull) {
  for (auto direction :
       {ScrollDirection::kVertical, ScrollDirection::kHorizontal}) {
    auto list = MakeList(direction);
    const bool vertical = direction == ScrollDirection::kVertical;
    list->UpdateContentOffsetForListContainer(300.f, vertical ? 0.f : 200.f,
                                              vertical ? 200.f : 0.f);
    list->SetOverscrollOffset(vertical ? FloatPoint(0.f, 60.f)
                                       : FloatPoint(60.f, 0.f));
    const auto paint = list->GetRenderScroll()->ScrollOffset();
    const auto raw = list->OverscrollOffset();
    for (int i = 0; i < 3; ++i) {
      list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
      EXPECT_FLOAT_EQ(200.f, vertical ? OffY(list) : OffX(list));
      EXPECT_EQ(paint, list->GetRenderScroll()->ScrollOffset());
      EXPECT_EQ(raw, list->OverscrollOffset());
    }
    page_->RemoveChild(list.get());
  }
}

TEST_F_UI(ListContainerViewTest, GrowthConsumesVisibleStretchNotRawDrag) {
  for (auto direction :
       {ScrollDirection::kVertical, ScrollDirection::kHorizontal}) {
    auto list = MakeList(direction);
    const bool vertical = direction == ScrollDirection::kVertical;
    list->UpdateContentOffsetForListContainer(300.f, vertical ? 0.f : 200.f,
                                              vertical ? 200.f : 0.f);
    list->SetOverscrollOffset(vertical ? FloatPoint(0.f, 60.f)
                                       : FloatPoint(60.f, 0.f));
    const auto paint = list->GetRenderScroll()->ScrollOffset();
    const float visible_offset = vertical ? paint.y() : paint.x();
    ASSERT_GT(visible_offset, 200.f);
    ASSERT_LT(visible_offset, 260.f);
    list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(visible_offset, vertical ? OffY(list) : OffX(list));
    EXPECT_EQ(paint, list->GetRenderScroll()->ScrollOffset());
    EXPECT_FALSE(list->IsUnderOverscroll());
    page_->RemoveChild(list.get());
  }
}

TEST_F_UI(ListContainerViewTest, GrowthDuringBounceUsesAnimatedVisibleStretch) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetClampedOverscrollOffset({0.f, 20.f});
  EXPECT_FLOAT_EQ(220.f, list->GetRenderScroll()->ScrollTop());
  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(220.f, OffY(list));
  EXPECT_FLOAT_EQ(220.f, list->GetRenderScroll()->ScrollTop());
}

TEST_F_UI(ListContainerViewTest,
          OverscrollGrowthNotifiesLayoutOfConsumedStretch) {
  for (auto direction :
       {ScrollDirection::kVertical, ScrollDirection::kHorizontal}) {
    RecordingListDelegate delegate;
    auto list = MakeList(direction);
    const bool vertical = direction == ScrollDirection::kVertical;
    list->UpdateContentOffsetForListContainer(300.f, vertical ? 0.f : 200.f,
                                              vertical ? 200.f : 0.f);
    list->SetOverscrollOffset(vertical ? FloatPoint(0.f, 60.f)
                                       : FloatPoint(60.f, 0.f));
    const float visible = vertical ? list->ClampedOverscrollOffset().y()
                                   : list->ClampedOverscrollOffset().x();
    list->SetDelegate(&delegate);
    list->UpdateContentOffsetForListContainer(400.f, vertical ? 0.f : 10.f,
                                              vertical ? 10.f : 0.f);
    ASSERT_EQ(1u, delegate.offsets.size());
    EXPECT_FLOAT_EQ(210.f + visible, vertical ? delegate.offsets[0].y()
                                              : delegate.offsets[0].x());
    EXPECT_EQ(list->GetScrollOffset(), delegate.offsets[0]);
    EXPECT_EQ(delegate.offsets, delegate.originals);
    // Filling the newly visible cells must not start an offset feedback loop.
    list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
    EXPECT_EQ(1u, delegate.offsets.size());
    page_->RemoveChild(list.get());
  }
}

TEST_F_UI(ListContainerViewTest,
          OverscrollGrowthNotifiesOnlyClampedFinalOffset) {
  RecordingListDelegate delegate;
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetClampedOverscrollOffset({0.f, 40.f});
  list->SetDelegate(&delegate);
  list->UpdateContentOffsetForListContainer(310.f, 0.f, 0.f);
  ASSERT_EQ(1u, delegate.offsets.size());
  EXPECT_FLOAT_EQ(210.f, delegate.offsets[0].y());
  EXPECT_EQ(delegate.offsets, delegate.originals);
}

TEST_F_UI(ListContainerViewTest, GrowthWithoutExtraLogicalMotionDoesNotNotify) {
  RecordingListDelegate delegate;
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
  list->SetDelegate(&delegate);
  list->UpdateContentOffsetForListContainer(350.f, 0.f, 20.f);
  EXPECT_TRUE(delegate.offsets.empty());
  list->OnScrollUpdate(0.f);
  delegate.offsets.clear();
  list->SetOverscrollOffset({0.f, -60.f});
  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(0.f, OffY(list));
  EXPECT_TRUE(delegate.offsets.empty());
}

TEST_F_UI(ListContainerViewTest, SteadyAndShrinkDoNotEchoLayoutOffset) {
  RecordingListDelegate delegate;
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetOverscrollOffset({0.f, 60.f});
  list->SetDelegate(&delegate);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
  EXPECT_TRUE(delegate.offsets.empty());
  list->UpdateContentOffsetForListContainer(250.f, 0.f, -50.f);
  EXPECT_FLOAT_EQ(150.f, OffY(list));
  EXPECT_TRUE(delegate.offsets.empty());
}

TEST_F_UI(ListContainerViewTest, SteadyAnchorAdjustmentRetiresOldStretch) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetOverscrollOffset({0.f, 60.f});
  list->UpdateContentOffsetForListContainer(300.f, 0.f, -20.f);
  EXPECT_FLOAT_EQ(180.f, OffY(list));
  EXPECT_FLOAT_EQ(180.f, list->GetRenderScroll()->ScrollTop());
  EXPECT_FALSE(list->IsUnderOverscroll());
}

TEST_F_UI(ListContainerViewTest, HorizontalOverscrollShrinkClearsStretch) {
  auto list = MakeList(ScrollDirection::kHorizontal);
  list->UpdateContentOffsetForListContainer(300.f, 200.f, 0.f);
  list->SetOverscrollOffset({60.f, 0.f});
  list->UpdateContentOffsetForListContainer(250.f, -50.f, 0.f);
  EXPECT_FLOAT_EQ(150.f, OffX(list));
  EXPECT_FLOAT_EQ(150.f, list->GetRenderScroll()->ScrollLeft());
  EXPECT_FALSE(list->IsUnderOverscroll());
}

TEST_F_UI(ListContainerViewTest, GrowthSmallerThanStretchClampsAtNewEdge) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetOverscrollOffset({0.f, 60.f});
  ASSERT_GT(list->ClampedOverscrollOffset().y(), 10.f);
  list->UpdateContentOffsetForListContainer(310.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(210.f, OffY(list));
  EXPECT_FLOAT_EQ(210.f, list->GetRenderScroll()->ScrollTop());
  EXPECT_FALSE(list->IsUnderOverscroll());
}

TEST_F_UI(ListContainerViewTest,
          UpperOverscrollGrowthDoesNotMakeOffsetNegative) {
  auto list = MakeList(ScrollDirection::kHorizontal);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 0.f);
  list->SetOverscrollOffset({-60.f, 0.f});
  ASSERT_LT(list->GetRenderScroll()->ScrollLeft(), 0.f);
  list->UpdateContentOffsetForListContainer(400.f, 0.f, 0.f);
  EXPECT_FLOAT_EQ(0.f, OffX(list));
  EXPECT_FLOAT_EQ(0.f, list->GetRenderScroll()->ScrollLeft());
  EXPECT_FALSE(list->IsUnderOverscroll());
}

TEST_F_UI(ListContainerViewTest, DirectionChangeStopsBounceAnimation) {
  auto list = MakeList(ScrollDirection::kVertical);
  list->SetOverscrollEnabled(true);
  list->UpdateContentOffsetForListContainer(300.f, 0.f, 200.f);
  list->SetOverscrollOffset({0.f, 60.f});
  ASSERT_TRUE(list->DoBounce(0.f));
  list->SetScrollDirection(ScrollDirection::kHorizontal);
  EXPECT_EQ(list->GetScrollStatus(), Scrollable::ScrollStatus::kIdle);
  EXPECT_FALSE(list->IsUnderOverscroll());
  EXPECT_TRUE(list->GetRenderScroll()->ScrollOffset().IsOrigin());
}

}  // namespace clay
