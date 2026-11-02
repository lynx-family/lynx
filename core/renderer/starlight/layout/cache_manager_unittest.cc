// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/cache_manager.h"

#include "core/renderer/starlight/layout/box_layout_algorithm.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace starlight {
namespace {

class ComposeMeasureCacheTest : public ::testing::Test {
 protected:
  class TestNode : public LayoutObject {
   public:
    using LayoutObject::LayoutObject;
    using LayoutObject::RemoveAlgorithm;
    using LayoutObject::RemoveAlgorithmRecursive;

    CacheManager& Cache() { return cache_manager_; }
    void SetSynchronized(bool value) {
      inflow_sub_tree_in_sync_with_last_measurement_ = value;
    }
    void RetainBoxAlgorithm() { algorithm_ = new BoxLayoutAlgorithm(this); }
  };

  void SetUp() override {
    configs_.SetQuirksMode(base::Version(4, 1));
    style_.SetDisplay(DisplayType::kXBox);
    style_.SetBoxSizing(BoxSizingType::kBorderBox);
    style_.SetPaddingLeft(NLength::MakeUnitNLength(3));
    style_.SetPaddingRight(NLength::MakeUnitNLength(7));
    style_.SetPaddingTop(NLength::MakeUnitNLength(5));
    style_.SetPaddingBottom(NLength::MakeUnitNLength(11));
    style_.SetBorderLeftWidth(1);
    style_.SetBorderRightWidth(2);
    style_.SetBorderTopWidth(3);
    style_.SetBorderBottomWidth(4);
    RefreshInsets();
    for (auto* node : {&node_, &child_}) {
      node->SetComposeLayout(true);
      node->SetBorderBoundWidth(52);
      node->SetBorderBoundHeight(36);
      node->SetBaseline(7);
      node->SetSynchronized(true);
    }
  }

  void TearDown() override {
    if (node_.FirstChild()) node_.RemoveChild(&child_);
  }

  void RefreshInsets() {
    node_.GetBoxInfo()->InitializeBoxInfo(
        {OneSideConstraint::Definite(100), OneSideConstraint::Definite(80)},
        node_, configs_);
  }

  void Store(TestNode& node, bool final_measure = true) {
    node.Cache().BeginComposeMeasure(range_, final_measure, node);
    node.SetSynchronized(true);
    node.Cache().InsertComposeCacheEntry(node);
  }

  LayoutConfigs configs_;
  LayoutComputedStyle style_{1.f};
  LayoutComputedStyle child_style_{1.f};
  TestNode node_{configs_, &style_};
  TestNode child_{configs_, &child_style_};
  ComposeConstraints range_{0, 100, 0, 80};
};

TEST_F(ComposeMeasureCacheTest, RequiresACompletedMeasurement) {
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  node_.Cache().BeginComposeMeasure(range_, true, node_);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  node_.Cache().InsertComposeCacheEntry(node_);
  EXPECT_TRUE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, MatchesEveryRangeBoundExactly) {
  Store(node_);
  for (const auto& different :
       {ComposeConstraints{0.0001f, 100, 0, 80},
        ComposeConstraints{0, 100.0001f, 0, 80},
        ComposeConstraints{0, 100, 0.0001f, 80},
        ComposeConstraints{0, 100, 0, 80.0001f},
        ComposeConstraints{0, ComposeConstraints::kInfinity, 0, 80},
        ComposeConstraints{0, 100, 0, ComposeConstraints::kInfinity}}) {
    EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(different, true, node_));
  }
  EXPECT_TRUE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, SeparatesSpeculativeAndFinalMeasurements) {
  Store(node_, false);
  EXPECT_TRUE(node_.Cache().CanReuseComposeMeasure(range_, false, node_));
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  Store(node_, true);
  EXPECT_TRUE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, false, node_));
}

TEST_F(ComposeMeasureCacheTest, RejectsChangedGeometryAndSubtreeState) {
  Store(node_);
  node_.SetBorderBoundWidth(53);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  node_.SetBorderBoundWidth(52);
  node_.SetBorderBoundHeight(37);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  node_.SetBorderBoundHeight(36);
  node_.SetBaseline(8);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  node_.SetBaseline(7);
  node_.SetSynchronized(false);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  node_.SetSynchronized(true);
  EXPECT_TRUE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, MatchesResolvedPaddingAndBorderInsets) {
  Store(node_);
  // Edit the resolved inset directly to isolate cache-key matching from the
  // BoxInfo initializer's separate invalidation behavior.
  auto& padding = node_.GetBoxInfo()->padding_;
  const float left = padding[kLeft];
  padding[kLeft] = left + 1;
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  padding[kLeft] = left;
  EXPECT_TRUE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  style_.SetBorderBottomWidth(5);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, ExplicitClearAndRepeatedDirtyInvalidate) {
  Store(node_);
  node_.ClearCache();
  node_.SetSynchronized(true);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  Store(node_);
  node_.MarkDirty();
  node_.SetSynchronized(true);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
  Store(node_);
  node_.MarkDirty();
  node_.SetSynchronized(true);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, CompletionRecordsResultAfterInvalidation) {
  node_.Cache().BeginComposeMeasure(range_, true, node_);
  node_.MarkDirtyAndRequestLayout(true);
  node_.SetSynchronized(true);
  node_.Cache().InsertComposeCacheEntry(node_);
  EXPECT_TRUE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, AlgorithmReleaseInvalidatesComposeResult) {
  node_.RetainBoxAlgorithm();
  Store(node_);
  node_.RemoveAlgorithm();
  node_.SetSynchronized(true);
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, AlgorithmReleasePropagatesSubtreeState) {
  node_.AppendChild(&child_);
  child_.RetainBoxAlgorithm();
  Store(node_);
  Store(child_);
  node_.RemoveAlgorithmRecursive();
  EXPECT_FALSE(node_.IsInflowSubTreeInSyncWithLastMeasurement());
  EXPECT_FALSE(node_.Cache().CanReuseComposeMeasure(range_, true, node_));
}

TEST_F(ComposeMeasureCacheTest, ComposeInvalidationPreservesCssEntries) {
  const Constraints css{OneSideConstraint::Definite(100),
                        OneSideConstraint::Definite(80)};
  node_.Cache().InsertCacheEntry(css, 100, 80);
  node_.Cache().ResetComposeCache();
  EXPECT_NE(nullptr, node_.Cache().FindAvailableCacheEntry(css, node_).cache_);
  node_.SetComposeLayout(false);
  node_.RetainBoxAlgorithm();
  node_.RemoveAlgorithm();
  EXPECT_NE(nullptr, node_.Cache().FindAvailableCacheEntry(css, node_).cache_);
  node_.Cache().BeginComposeMeasure(range_, true, node_);
  EXPECT_EQ(nullptr, node_.Cache().FindAvailableCacheEntry(css, node_).cache_);
}

}  // namespace
}  // namespace starlight
}  // namespace lynx
