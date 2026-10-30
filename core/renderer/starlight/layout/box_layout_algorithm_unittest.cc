// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/box_layout_algorithm.h"

#include <memory>
#include <vector>

#include "core/renderer/starlight/layout/layout_object.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace starlight {
namespace {

class BoxLayoutAlgorithmTest : public ::testing::Test {
 protected:
  void SetUp() override { configs_.SetQuirksMode(base::Version(3, 1)); }

  LayoutObject* CreateNode(DisplayType display = DisplayType::kXBox) {
    styles_.push_back(std::make_unique<LayoutComputedStyle>(1.f));
    auto* style = styles_.back().get();
    style->SetDisplay(display);
    style->SetBoxSizing(BoxSizingType::kBorderBox);
    nodes_.push_back(std::make_unique<LayoutObject>(configs_, style));
    return nodes_.back().get();
  }

  LayoutObject* CreateFixedChild() {
    auto* child = CreateNode();
    child->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(71));
    child->GetCSSMutableStyle()->SetHeight(NLength::MakeUnitNLength(93));
    // Non-leaf nodes can return a speculative size without updating bounds.
    child->AppendChild(CreateNode());
    return child;
  }

  LayoutObject* CreatePaddedBox(LayoutObject* child, bool compose = false) {
    auto* box = CreateNode();
    box->SetComposeLayout(compose);
    box->GetCSSMutableStyle()->SetPaddingLeft(NLength::MakeUnitNLength(12));
    box->GetCSSMutableStyle()->SetPaddingTop(NLength::MakeUnitNLength(4));
    box->AppendChild(child);
    return box;
  }

  struct ComposeMeasurement {
    std::vector<ComposeConstraints> ranges;
    FloatSize preferred{20, 15, 0};
  };

  LayoutObject* CreateComposeChild(ComposeMeasurement* measurement) {
    auto* child = CreateNode();
    child->SetComposeLayout(true);
    child->SetContext(measurement);
    child->SetSLComposeMeasureFunc([](void* context,
                                      const ComposeConstraints& range, bool) {
      auto* measurement = static_cast<ComposeMeasurement*>(context);
      measurement->ranges.push_back(range);
      return FloatSize(range.ConstrainWidth(measurement->preferred.width_),
                       range.ConstrainHeight(measurement->preferred.height_),
                       0);
    });
    return child;
  }

  struct NativeMeasurement {
    LayoutObject* node{nullptr};
    FloatSize preferred{20, 15, 0};
    std::vector<Constraints> ranges;
  };

  LayoutObject* CreateNativeChild(NativeMeasurement* measurement) {
    auto* child = CreateNode();
    measurement->node = child;
    child->SetContext(measurement);
    child->SetSLMeasureFunc(
        [](void* context, const Constraints& constraints, bool) {
          auto* measurement = static_cast<NativeMeasurement*>(context);
          measurement->ranges.push_back(constraints);
          auto axis = [](const OneSideConstraint& constraint, float preferred) {
            if (constraint.Mode() == SLMeasureModeIndefinite) return preferred;
            if (constraint.Mode() == SLMeasureModeDefinite)
              return constraint.Size();
            return std::min(preferred, constraint.Size());
          };
          return FloatSize(
              axis(constraints[kHorizontal], measurement->preferred.width_),
              axis(constraints[kVertical], measurement->preferred.height_), 0);
        });
    return child;
  }

  void ExpectRange(const ComposeConstraints& range, float min_width,
                   float max_width, float min_height, float max_height) {
    EXPECT_FLOAT_EQ(min_width, range.min_width);
    EXPECT_FLOAT_EQ(max_width, range.max_width);
    EXPECT_FLOAT_EQ(min_height, range.min_height);
    EXPECT_FLOAT_EQ(max_height, range.max_height);
  }

  void ExpectFlexLayout(bool is_row) {
    auto* root = CreateNode(DisplayType::kFlex);
    auto* root_style = root->GetCSSMutableStyle();
    root_style->SetWidth(NLength::MakeUnitNLength(332));
    root_style->SetHeight(NLength::MakeUnitNLength(200));
    root_style->SetFlexDirection(is_row ? FlexDirectionType::kRow
                                        : FlexDirectionType::kColumn);
    auto* flexible = CreateNode();
    flexible->GetCSSMutableStyle()->SetFlexBasis(NLength::MakeUnitNLength(0));
    flexible->GetCSSMutableStyle()->SetFlexGrow(1);
    auto* child = CreateFixedChild();
    auto* box = CreatePaddedBox(child);
    root->AppendChild(flexible);
    root->AppendChild(box);

    root->ReLayout();

    EXPECT_FLOAT_EQ(71, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(93, child->GetBorderBoundHeight());
    EXPECT_FLOAT_EQ(12, child->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(4, child->GetBorderBoundTopFromParentPaddingBound());
    if (is_row) {
      EXPECT_FLOAT_EQ(249, flexible->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(83, box->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(249, box->GetBorderBoundLeftFromParentPaddingBound());
    } else {
      EXPECT_FLOAT_EQ(103, flexible->GetBorderBoundHeight());
      EXPECT_FLOAT_EQ(97, box->GetBorderBoundHeight());
      EXPECT_FLOAT_EQ(103, box->GetBorderBoundTopFromParentPaddingBound());
    }
  }

 private:
  LayoutConfigs configs_;
  std::vector<std::unique_ptr<LayoutComputedStyle>> styles_;
  std::vector<std::unique_ptr<LayoutObject>> nodes_;
};

TEST_F(BoxLayoutAlgorithmTest, SpeculativeSizeIncludesChildAndMargins) {
  auto* child = CreateFixedChild();
  child->GetCSSMutableStyle()->SetMarginLeft(NLength::MakeUnitNLength(3));
  child->GetCSSMutableStyle()->SetMarginRight(NLength::MakeUnitNLength(5));
  child->GetCSSMutableStyle()->SetMarginTop(NLength::MakeUnitNLength(7));
  child->GetCSSMutableStyle()->SetMarginBottom(NLength::MakeUnitNLength(9));
  auto* box = CreatePaddedBox(child);

  const auto size = box->UpdateMeasureByPlatform(Constraints(), false);

  EXPECT_FLOAT_EQ(91, size.width_);
  EXPECT_FLOAT_EQ(113, size.height_);
  // Prediction must not force final measurement of the fixed-size subtree.
  EXPECT_FLOAT_EQ(0, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(0, child->GetBorderBoundHeight());

  const auto final_size = box->UpdateMeasureByPlatform(Constraints(), true);
  EXPECT_FLOAT_EQ(size.width_, final_size.width_);
  EXPECT_FLOAT_EQ(size.height_, final_size.height_);
  EXPECT_FLOAT_EQ(71, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(93, child->GetBorderBoundHeight());
}

TEST_F(BoxLayoutAlgorithmTest, CachedSizeDoesNotUseCurrentChildBounds) {
  auto* child = CreateFixedChild();
  auto* box = CreatePaddedBox(child);
  child->UpdateMeasureByPlatform(Constraints(), true);
  ASSERT_FLOAT_EQ(71, child->GetBorderBoundWidth());
  ASSERT_FLOAT_EQ(93, child->GetBorderBoundHeight());

  Constraints smaller;
  smaller[kHorizontal] = OneSideConstraint::Definite(40);
  smaller[kVertical] = OneSideConstraint::Definite(50);
  child->UpdateMeasure(smaller, true);
  ASSERT_FLOAT_EQ(40, child->GetBorderBoundWidth());
  ASSERT_FLOAT_EQ(50, child->GetBorderBoundHeight());

  const auto size = box->UpdateMeasureByPlatform(Constraints(), false);

  EXPECT_FLOAT_EQ(83, size.width_);
  EXPECT_FLOAT_EQ(97, size.height_);
  EXPECT_FLOAT_EQ(40, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(50, child->GetBorderBoundHeight());

  const auto final_size = box->UpdateMeasureByPlatform(Constraints(), true);
  EXPECT_FLOAT_EQ(size.width_, final_size.width_);
  EXPECT_FLOAT_EQ(size.height_, final_size.height_);
  EXPECT_FLOAT_EQ(71, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(93, child->GetBorderBoundHeight());
}

TEST_F(BoxLayoutAlgorithmTest, FlexRowReservesPaddedBoxWidth) {
  ExpectFlexLayout(true);
}

TEST_F(BoxLayoutAlgorithmTest, FlexColumnReservesPaddedBoxHeight) {
  ExpectFlexLayout(false);
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxPropagatesContentRanges) {
  constexpr float infinity = ComposeConstraints::kInfinity;
  for (bool propagate : {false, true}) {
    SCOPED_TRACE(propagate);
    ComposeMeasurement measurement;
    auto* child = CreateComposeChild(&measurement);
    auto* box = CreatePaddedBox(child, true);
    box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(propagate);
    for (const auto& range : {ComposeConstraints{50, 100, 30, 80},
                              ComposeConstraints{60, 100, 40, 80},
                              ComposeConstraints{50, infinity, 30, infinity},
                              ComposeConstraints{50, 50, 30, 30}}) {
      SCOPED_TRACE(range.min_width);
      for (bool final_measure : {false, true}) {
        SCOPED_TRACE(final_measure);
        measurement.ranges.clear();
        const auto size =
            box->UpdateMeasureWithComposeConstraints(range, final_measure);
        ASSERT_EQ(1u, measurement.ranges.size());
        const auto content = range.Inset(12, 4);
        ExpectRange(measurement.ranges.front(),
                    propagate ? content.min_width : 0, content.max_width,
                    propagate ? content.min_height : 0, content.max_height);
        EXPECT_FLOAT_EQ(range.min_width, size.width_);
        EXPECT_FLOAT_EQ(range.min_height, size.height_);
        if (final_measure) {
          box->AlignmentByPlatform(0, 0);
          EXPECT_FLOAT_EQ(12,
                          child->GetBorderBoundLeftFromParentPaddingBound());
          EXPECT_FLOAT_EQ(4, child->GetBorderBoundTopFromParentPaddingBound());
        }
        box->MarkNotDirty();
        child->MarkNotDirty();
      }
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxRangesSurviveNestedBoxes) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  auto* inner = CreatePaddedBox(child, true);
  auto* middle = CreatePaddedBox(inner, true);
  auto* outer = CreatePaddedBox(middle, true);
  for (auto* box : {inner, middle, outer})
    box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  for (float minimum : {70.f, 80.f}) {
    SCOPED_TRACE(minimum);
    measurement.ranges.clear();
    const auto size = outer->UpdateMeasureWithComposeConstraints(
        {minimum, 120, 30, ComposeConstraints::kInfinity}, true);
    ASSERT_EQ(1u, measurement.ranges.size());
    ExpectRange(measurement.ranges.front(), minimum - 36, 84, 18,
                ComposeConstraints::kInfinity);
    EXPECT_FLOAT_EQ(minimum, size.width_);
    EXPECT_FLOAT_EQ(30, size.height_);
    outer->AlignmentByPlatform(0, 0);
    for (auto* node : {child, inner, middle, outer}) node->MarkNotDirty();
  }
  middle->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(false);
  measurement.ranges.clear();
  outer->UpdateMeasureWithComposeConstraints({80, 120, 30, 90}, true);
  ASSERT_EQ(1u, measurement.ranges.size());
  ExpectRange(measurement.ranges.front(), 0, 84, 0, 78);
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxConstrainsSizeAndFillHints) {
  for (bool fill : {false, true}) {
    SCOPED_TRACE(fill);
    ComposeMeasurement measurement;
    auto* child = CreateComposeChild(&measurement);
    auto* style = child->GetCSSMutableStyle();
    style->SetWidth(fill ? NLength::MakePercentageNLength(50)
                         : NLength::MakeUnitNLength(200));
    style->SetHeight(NLength::MakeUnitNLength(200));
    auto* box = CreatePaddedBox(child, true);
    const auto size =
        box->UpdateMeasureWithComposeConstraints({0, 100, 0, 80}, true);
    ASSERT_EQ(1u, measurement.ranges.size());
    const float width = fill ? 44 : 88;
    ExpectRange(measurement.ranges.front(), width, width, 76, 76);
    EXPECT_FLOAT_EQ(width + 12, size.width_);
    EXPECT_FLOAT_EQ(80, size.height_);
    // An unbounded fill axis remains flexible.
    if (fill) {
      measurement.ranges.clear();
      box->UpdateMeasureWithComposeConstraints(ComposeConstraints(), true);
      ASSERT_EQ(1u, measurement.ranges.size());
      ExpectRange(measurement.ranges.front(), 0, ComposeConstraints::kInfinity,
                  200, 200);
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxKeepsPercentagePaddingBase) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  child->GetCSSMutableStyle()->SetPaddingLeft(
      NLength::MakePercentageNLength(10));
  auto* box = CreateNode();
  box->SetComposeLayout(true);
  box->AppendChild(child);
  const auto size =
      box->UpdateMeasureWithComposeConstraints({0, 100, 0, 80}, true);
  ASSERT_EQ(1u, measurement.ranges.size());
  ExpectRange(measurement.ranges.front(), 0, 90, 0, 80);
  EXPECT_FLOAT_EQ(30, size.width_);
  EXPECT_FLOAT_EQ(10, child->GetLayoutPaddingLeft());
  box->AlignmentByPlatform(0, 0);
  EXPECT_FLOAT_EQ(30, child->GetBorderBoundWidth());
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxMatchParentUsesChosenContentSize) {
  ComposeMeasurement regular_measurement;
  ComposeMeasurement match_measurement;
  auto* regular = CreateComposeChild(&regular_measurement);
  auto* match = CreateComposeChild(&match_measurement);
  match->GetCSSMutableStyle()->SetXBoxMatchParentSize(true);
  match->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(200));
  match->GetCSSMutableStyle()->SetMarginLeft(NLength::MakeUnitNLength(3));
  match->GetCSSMutableStyle()->SetMarginTop(NLength::MakeUnitNLength(2));
  auto* box = CreatePaddedBox(regular, true);
  box->AppendChild(match);
  const auto size =
      box->UpdateMeasureWithComposeConstraints({50, 100, 30, 80}, true);
  EXPECT_TRUE(match_measurement.ranges.empty());
  EXPECT_FLOAT_EQ(50, size.width_);
  EXPECT_FLOAT_EQ(30, size.height_);
  box->AlignmentByPlatform(0, 0);
  ASSERT_EQ(1u, match_measurement.ranges.size());
  ExpectRange(match_measurement.ranges.front(), 35, 35, 24, 24);
  EXPECT_FLOAT_EQ(15, match->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(6, match->GetBorderBoundTopFromParentPaddingBound());
}

TEST_F(BoxLayoutAlgorithmTest, BoxMatchParentMeasuresDuringPlacement) {
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    NativeMeasurement regular_measurement;
    NativeMeasurement match_measurement;
    match_measurement.preferred = FloatSize(200, 120, 0);
    auto* regular = CreateNativeChild(&regular_measurement);
    auto* match = CreateNativeChild(&match_measurement);
    match->GetCSSMutableStyle()->SetXBoxMatchParentSize(true);
    auto* box = CreatePaddedBox(regular, compose);
    box->AppendChild(match);
    if (!compose) {
      box->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(50));
      box->GetCSSMutableStyle()->SetHeight(NLength::MakeUnitNLength(30));
    }

    for (int pass = 0; pass < 2; ++pass) {
      SCOPED_TRACE(pass);
      const auto size = compose ? box->UpdateMeasureWithComposeConstraints(
                                      {50, 100, 30, 80}, true)
                                : box->UpdateMeasureByPlatform(
                                      {OneSideConstraint::Definite(50),
                                       OneSideConstraint::Definite(30)},
                                      true);
      EXPECT_FLOAT_EQ(50, size.width_);
      EXPECT_FLOAT_EQ(30, size.height_);
      EXPECT_EQ(pass == 0 ? 0u : 1u, match_measurement.ranges.size());
      box->AlignmentByPlatform(0, 0);
      ASSERT_EQ(1u, match_measurement.ranges.size());
      const auto& constraints = match_measurement.ranges.front();
      EXPECT_EQ(SLMeasureModeDefinite, constraints[kHorizontal].Mode());
      EXPECT_EQ(SLMeasureModeDefinite, constraints[kVertical].Mode());
      EXPECT_FLOAT_EQ(38, constraints[kHorizontal].Size());
      EXPECT_FLOAT_EQ(26, constraints[kVertical].Size());
      EXPECT_FLOAT_EQ(38, match->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(26, match->GetBorderBoundHeight());
      EXPECT_FLOAT_EQ(12, match->GetBorderBoundLeftFromParentPaddingBound());
      EXPECT_FLOAT_EQ(4, match->GetBorderBoundTopFromParentPaddingBound());
      for (auto* node : {box, regular, match}) node->MarkNotDirty();
    }
    EXPECT_EQ(1u, regular_measurement.ranges.size());
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxEmptyAndOnlyMatchParentSize) {
  auto* empty = CreateNode();
  empty->SetComposeLayout(true);
  const auto empty_size =
      empty->UpdateMeasureWithComposeConstraints({50, 100, 30, 80}, true);
  EXPECT_FLOAT_EQ(50, empty_size.width_);
  EXPECT_FLOAT_EQ(30, empty_size.height_);

  ComposeMeasurement measurement;
  auto* match = CreateComposeChild(&measurement);
  match->GetCSSMutableStyle()->SetXBoxMatchParentSize(true);
  auto* box = CreatePaddedBox(match, true);
  const auto size =
      box->UpdateMeasureWithComposeConstraints({0, 100, 0, 80}, true);
  EXPECT_TRUE(measurement.ranges.empty());
  EXPECT_FLOAT_EQ(12, size.width_);
  EXPECT_FLOAT_EQ(4, size.height_);
  box->AlignmentByPlatform(0, 0);
  ASSERT_EQ(1u, measurement.ranges.size());
  ExpectRange(measurement.ranges.front(), 0, 0, 0, 0);
}

TEST_F(BoxLayoutAlgorithmTest, ComposeIdentityEntersRangesWithoutAnyPolicy) {
  ComposeMeasurement measurement;
  auto* leaf = CreateComposeChild(&measurement);
  auto* inner = CreateNode();
  inner->SetComposeLayout(true);
  inner->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  inner->AppendChild(leaf);
  auto* root = CreateNode();
  root->SetComposeLayout(true);
  root->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  root->GetCSSMutableStyle()->SetMaxWidth(NLength::MakeUnitNLength(100));
  root->GetCSSMutableStyle()->SetMinHeight(NLength::MakeUnitNLength(30));
  root->GetCSSMutableStyle()->SetMaxHeight(NLength::MakeUnitNLength(80));
  root->AppendChild(inner);

  for (float minimum : {50.f, 60.f}) {
    root->GetCSSMutableStyle()->SetMinWidth(NLength::MakeUnitNLength(minimum));
    for (bool final_measure : {false, true}) {
      measurement.ranges.clear();
      const auto size =
          root->UpdateMeasureByPlatform(Constraints(), final_measure);
      EXPECT_FLOAT_EQ(minimum, size.width_);
      EXPECT_FLOAT_EQ(30, size.height_);
      ASSERT_FALSE(measurement.ranges.empty());
      ExpectRange(measurement.ranges.front(), minimum, 100, 30, 80);
      EXPECT_FLOAT_EQ(minimum, leaf->GetBorderBoundWidth());
      EXPECT_TRUE(leaf->IsComposeLayout());
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeIdentityKeepsNativeAlgorithmLayout) {
  for (DisplayType display : {DisplayType::kXBox, DisplayType::kFlex,
                              DisplayType::kRelative, DisplayType::kGrid}) {
    for (bool compose : {false, true}) {
      SCOPED_TRACE(static_cast<int>(display));
      SCOPED_TRACE(compose);
      const bool uses_compose_ranges = compose && display == DisplayType::kXBox;
      NativeMeasurement inflow_measurement;
      NativeMeasurement absolute_measurement;
      auto* inner = CreateNode();
      inner->SetComposeLayout(uses_compose_ranges);
      inner->AppendChild(CreateNativeChild(&inflow_measurement));
      auto* absolute = CreateNativeChild(&absolute_measurement);
      absolute->GetCSSMutableStyle()->SetPosition(PositionType::kAbsolute);
      auto* root = CreateNode(display);
      root->SetComposeLayout(compose);
      root->GetCSSMutableStyle()->SetMinHeight(NLength::MakeUnitNLength(30));
      root->GetCSSMutableStyle()->SetMaxWidth(NLength::MakeUnitNLength(100));
      root->GetCSSMutableStyle()->SetMaxHeight(NLength::MakeUnitNLength(80));
      root->AppendChild(inner);
      root->AppendChild(absolute);
      for (float minimum : {50.f, 70.f}) {
        root->GetCSSMutableStyle()->SetMinWidth(
            NLength::MakeUnitNLength(minimum));
        for (bool final_measure : {false, true}) {
          // CSS paths may reuse a child measurement between passes.
          if (uses_compose_ranges) inflow_measurement.ranges.clear();
          const auto size =
              root->UpdateMeasureByPlatform(Constraints(), final_measure);
          EXPECT_GE(size.width_, minimum);
          EXPECT_LE(size.width_, 100);
          EXPECT_GE(size.height_, 30);
          EXPECT_LE(size.height_, 80);
          ASSERT_FALSE(inflow_measurement.ranges.empty());
          EXPECT_FLOAT_EQ(20, inflow_measurement.node->GetBorderBoundWidth());
          EXPECT_FLOAT_EQ(15, inflow_measurement.node->GetBorderBoundHeight());
          if (final_measure) {
            root->UpdateAlignment();
            ASSERT_FALSE(absolute_measurement.ranges.empty());
            EXPECT_FLOAT_EQ(20, absolute->GetBorderBoundWidth());
            EXPECT_FLOAT_EQ(15, absolute->GetBorderBoundHeight());
          }
        }
      }
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, NativeLinearKeepsIndefiniteMainAxis) {
  for (bool staggered : {false, true}) {
    SCOPED_TRACE(staggered);
    NativeMeasurement measurement;
    measurement.preferred = {20, 200, 0};
    auto* root = CreateNode(DisplayType::kLinear);
    root->GetCSSMutableStyle()->SetMaxWidth(NLength::MakeUnitNLength(100));
    root->GetCSSMutableStyle()->SetMaxHeight(NLength::MakeUnitNLength(80));
    if (staggered) root->attr_map().setColumnCount(2);
    auto* child = CreateNativeChild(&measurement);
    root->AppendChild(child);
    root->UpdateMeasureByPlatform(Constraints(), true);
    EXPECT_FLOAT_EQ(200, child->GetBorderBoundHeight());
    ASSERT_FALSE(measurement.ranges.empty());
    EXPECT_EQ(SLMeasureModeIndefinite,
              measurement.ranges.front()[kVertical].Mode());
  }
}

TEST_F(BoxLayoutAlgorithmTest, FlexMeasuresComposeBoxChild) {
  for (bool compose_parent : {false, true}) {
    for (auto direction :
         {FlexDirectionType::kRow, FlexDirectionType::kColumn}) {
      SCOPED_TRACE(compose_parent);
      SCOPED_TRACE(static_cast<int>(direction));
      ComposeMeasurement measurement;
      auto* content = CreateComposeChild(&measurement);
      auto* box = CreateNode();
      box->SetComposeLayout(true);
      box->AppendChild(content);
      auto* flex = CreateNode(DisplayType::kFlex);
      flex->SetComposeLayout(compose_parent);
      auto* style = flex->GetCSSMutableStyle();
      style->SetWidth(NLength::MakeUnitNLength(100));
      style->SetHeight(NLength::MakeUnitNLength(80));
      style->SetFlexDirection(direction);
      style->SetAlignItems(FlexAlignType::kFlexStart);
      flex->AppendChild(box);

      flex->ReLayout();

      ASSERT_FALSE(measurement.ranges.empty());
      EXPECT_FLOAT_EQ(20, box->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(15, box->GetBorderBoundHeight());
      EXPECT_FLOAT_EQ(20, content->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(15, content->GetBorderBoundHeight());
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, FlexMeasuresComposePolicyChild) {
  for (bool compose_parent : {false, true}) {
    for (auto direction :
         {FlexDirectionType::kRow, FlexDirectionType::kColumn}) {
      SCOPED_TRACE(compose_parent);
      SCOPED_TRACE(static_cast<int>(direction));
      ComposeMeasurement measurement;
      auto* content = CreateComposeChild(&measurement);
      auto* flex = CreateNode(DisplayType::kFlex);
      flex->SetComposeLayout(compose_parent);
      auto* style = flex->GetCSSMutableStyle();
      style->SetWidth(NLength::MakeUnitNLength(100));
      style->SetHeight(NLength::MakeUnitNLength(80));
      style->SetFlexDirection(direction);
      style->SetAlignItems(FlexAlignType::kFlexStart);
      flex->AppendChild(content);

      flex->ReLayout();

      ASSERT_FALSE(measurement.ranges.empty());
      EXPECT_FLOAT_EQ(20, content->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(15, content->GetBorderBoundHeight());
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, NonBoxExplicitComposeMeasureUsesCssChildren) {
  for (DisplayType display : {DisplayType::kFlex, DisplayType::kLinear,
                              DisplayType::kRelative, DisplayType::kGrid}) {
    SCOPED_TRACE(static_cast<int>(display));
    NativeMeasurement measurement;
    auto* inner = CreateNode();
    inner->AppendChild(CreateNativeChild(&measurement));
    auto* root = CreateNode(display);
    root->SetComposeLayout(true);
    root->AppendChild(inner);
    for (float minimum : {50.f, 70.f}) {
      const auto size = root->UpdateMeasureWithComposeConstraints(
          {minimum, 100, 30, 80}, true);
      EXPECT_GE(size.width_, minimum);
      EXPECT_LE(size.width_, 100);
      EXPECT_GE(size.height_, 30);
      EXPECT_LE(size.height_, 80);
      ASSERT_FALSE(measurement.ranges.empty());
      EXPECT_FLOAT_EQ(20, measurement.node->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(15, measurement.node->GetBorderBoundHeight());
      root->UpdateAlignment();
      root->MarkNotDirty();
      inner->MarkNotDirty();
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxKeepsPercentagePaddingAcrossMeasures) {
  NativeMeasurement measurement;
  auto* inner = CreateNode();
  inner->SetComposeLayout(true);
  inner->GetCSSMutableStyle()->SetPaddingLeft(
      NLength::MakePercentageNLength(10));
  inner->AppendChild(CreateNativeChild(&measurement));
  auto* root = CreateNode();
  root->SetComposeLayout(true);
  root->AppendChild(inner);
  for (float maximum : {100.f, 140.f, 100.f}) {
    root->UpdateMeasureWithComposeConstraints({50, maximum, 30, 80}, true);
    EXPECT_FLOAT_EQ(maximum / 10, inner->GetLayoutPaddingLeft());
    root->UpdateAlignment();
    EXPECT_FLOAT_EQ(maximum / 10, inner->GetLayoutPaddingLeft());
    EXPECT_FLOAT_EQ(
        maximum / 10,
        measurement.node->GetBorderBoundLeftFromParentPaddingBound());
    root->MarkNotDirty();
    inner->MarkNotDirty();
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeBoxMeasuresNativeContentWithoutPolicy) {
  NativeMeasurement measurement;
  auto* content = CreateNode();
  content->SetComposeLayout(true);
  content->AppendChild(CreateNativeChild(&measurement));
  measurement.node->SetComposeLayout(true);
  auto* box = CreateNode();
  box->SetComposeLayout(true);
  box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  box->GetCSSMutableStyle()->SetPaddingLeft(NLength::MakeUnitNLength(10));
  box->GetCSSMutableStyle()->SetPaddingTop(NLength::MakeUnitNLength(4));
  box->GetCSSMutableStyle()->SetMinWidth(NLength::MakeUnitNLength(50));
  box->GetCSSMutableStyle()->SetMaxWidth(NLength::MakeUnitNLength(100));
  box->AppendChild(content);
  for (float preferred : {20.f, 30.f}) {
    measurement.preferred.width_ = preferred;
    measurement.node->MarkDirty();
    measurement.ranges.clear();
    const auto size = box->UpdateMeasureByPlatform(Constraints(), true);
    EXPECT_FLOAT_EQ(50, size.width_);
    EXPECT_FLOAT_EQ(40, content->GetBorderBoundWidth());
    box->UpdateAlignment();
    ASSERT_FALSE(measurement.ranges.empty());
    EXPECT_EQ(SLMeasureModeAtMost,
              measurement.ranges.front()[kHorizontal].Mode());
    EXPECT_FLOAT_EQ(90, measurement.ranges.front()[kHorizontal].Size());
    EXPECT_FLOAT_EQ(10, content->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(4, content->GetBorderBoundTopFromParentPaddingBound());
    EXPECT_FLOAT_EQ(preferred, measurement.node->GetBorderBoundWidth());
    box->MarkNotDirty();
    content->MarkNotDirty();
  }
}

TEST_F(BoxLayoutAlgorithmTest,
       ComposeBoxPropagatesTightConstraintsThroughPadding) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  auto* box = CreatePaddedBox(child, true);
  box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  auto* outer = CreateNode();
  outer->SetComposeLayout(true);
  outer->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  outer->GetCSSMutableStyle()->SetPaddingLeft(NLength::MakeUnitNLength(5));
  outer->GetCSSMutableStyle()->SetPaddingTop(NLength::MakeUnitNLength(7));
  outer->AppendChild(box);

  const auto size =
      outer->UpdateMeasureWithComposeConstraints({100, 100, 80, 80}, true);
  EXPECT_FLOAT_EQ(100, size.width_);
  EXPECT_FLOAT_EQ(80, size.height_);
  ASSERT_EQ(1u, measurement.ranges.size());
  ExpectRange(measurement.ranges.front(), 83, 83, 69, 69);
  EXPECT_FLOAT_EQ(83, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(69, child->GetBorderBoundHeight());
  outer->UpdateAlignment();
  EXPECT_FLOAT_EQ(5, box->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(7, box->GetBorderBoundTopFromParentPaddingBound());
  EXPECT_FLOAT_EQ(12, child->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(4, child->GetBorderBoundTopFromParentPaddingBound());
}

TEST_F(BoxLayoutAlgorithmTest, ComposePaddingCannotExpandPastMaximum) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  auto* box = CreatePaddedBox(child, true);
  box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  const ComposeConstraints range{0, 10, 0, 3};
  for (int pass = 0; pass < 2; ++pass) {
    const auto size = box->UpdateMeasureWithComposeConstraints(range, true);
    EXPECT_FLOAT_EQ(10, size.width_);
    EXPECT_FLOAT_EQ(3, size.height_);
    box->UpdateAlignment();
    EXPECT_FLOAT_EQ(0, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(0, child->GetBorderBoundHeight());
    EXPECT_FLOAT_EQ(12, child->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(4, child->GetBorderBoundTopFromParentPaddingBound());
  }
  ASSERT_EQ(1u, measurement.ranges.size());
  ExpectRange(measurement.ranges.front(), 0, 0, 0, 0);

  auto* css_child = CreateNode();
  auto* css_box = CreatePaddedBox(css_child);
  const auto css_size = css_box->UpdateMeasureByPlatform(
      {OneSideConstraint::AtMost(10), OneSideConstraint::AtMost(3)}, true);
  EXPECT_FLOAT_EQ(12, css_size.width_);
  EXPECT_FLOAT_EQ(4, css_size.height_);
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheMatchesAllFourBoundsExactly) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  constexpr float infinity = ComposeConstraints::kInfinity;
  for (const auto& range :
       {ComposeConstraints{0, 100, 0, 80}, ComposeConstraints{1, 100, 0, 80},
        ComposeConstraints{1, 100, 2, 80},
        ComposeConstraints{1, 100.0001f, 2, 80},
        ComposeConstraints{1, 100.0001f, 2, 80.0001f},
        ComposeConstraints{1, infinity, 2, 80.0001f},
        ComposeConstraints{1, infinity, 2, infinity},
        ComposeConstraints{0, 100, 0, 80}}) {
    const auto previous = measurement.ranges.size();
    child->UpdateMeasureWithComposeConstraints(range, true);
    EXPECT_EQ(previous + 1, measurement.ranges.size());
    child->MarkNotDirty();
    child->UpdateMeasureWithComposeConstraints(range, true);
    EXPECT_EQ(previous + 1, measurement.ranges.size());
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheSeparatesSpeculativeAndFinalPasses) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  const ComposeConstraints range{0, 100, 0, 80};
  child->UpdateMeasureWithComposeConstraints(range, false);
  child->UpdateMeasureWithComposeConstraints(range, false);
  EXPECT_EQ(1u, measurement.ranges.size());
  EXPECT_FALSE(child->GetFinalMeasure());
  child->UpdateMeasureWithComposeConstraints(range, true);
  child->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(2u, measurement.ranges.size());
  EXPECT_TRUE(child->GetFinalMeasure());
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheInvalidatesDirtyAndExplicitClear) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  const ComposeConstraints range{0, 100, 0, 80};
  child->UpdateMeasureWithComposeConstraints(range, true);
  // Geometry already marked this node dirty; explicit dirty still invalidates.
  ASSERT_TRUE(child->IsDirty());
  measurement.preferred.width_ = 30;
  child->MarkDirty();
  child->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(2u, measurement.ranges.size());
  EXPECT_FLOAT_EQ(30, child->GetBorderBoundWidth());
  child->ClearCache();
  child->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(3u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheKeepsNestedBoxPlacementState) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  auto* box = CreatePaddedBox(child, true);
  auto* outer = CreateNode();
  outer->SetComposeLayout(true);
  outer->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  outer->GetCSSMutableStyle()->SetPaddingLeft(NLength::MakeUnitNLength(5));
  outer->GetCSSMutableStyle()->SetPaddingTop(NLength::MakeUnitNLength(7));
  outer->AppendChild(box);
  const ComposeConstraints range{0, 100, 0, 80};
  for (int pass = 0; pass < 2; ++pass) {
    outer->UpdateMeasureWithComposeConstraints(range, true);
    // A cached measure must still allow alignment to restore child positions.
    box->SetBorderBoundLeftFromParentPaddingBound(99);
    child->SetBorderBoundTopFromParentPaddingBound(99);
    outer->UpdateAlignment();
    EXPECT_FLOAT_EQ(5, box->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(7, box->GetBorderBoundTopFromParentPaddingBound());
    EXPECT_FLOAT_EQ(12, child->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(4, child->GetBorderBoundTopFromParentPaddingBound());
    for (auto* node : {outer, box, child}) node->MarkNotDirty();
  }
  EXPECT_EQ(1u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest,
       ComposeCacheInvalidatesParentOnDirtyChildMeasure) {
  for (bool changed_size : {false, true}) {
    SCOPED_TRACE(changed_size);
    ComposeMeasurement measurement;
    auto* child = CreateComposeChild(&measurement);
    auto* box = CreateNode();
    box->SetComposeLayout(true);
    box->AppendChild(child);
    const ComposeConstraints range{0, 100, 0, 80};
    box->UpdateMeasureWithComposeConstraints(range, true);
    box->UpdateMeasureWithComposeConstraints(range, true);
    ASSERT_EQ(1u, measurement.ranges.size());
    if (changed_size) measurement.preferred.width_ = 30;
    child->MarkDirty();
    child->UpdateMeasureWithComposeConstraints({0, 90, 0, 80}, true);
    ASSERT_EQ(2u, measurement.ranges.size());
    box->UpdateMeasureWithComposeConstraints(range, true);
    EXPECT_EQ(3u, measurement.ranges.size());
    EXPECT_FLOAT_EQ(changed_size ? 30 : 20, box->GetBorderBoundWidth());
    child->MarkDirty();
    box->UpdateMeasureWithComposeConstraints(range, true);
    EXPECT_EQ(4u, measurement.ranges.size());
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeChildConstraintChangeKeepsParentCache) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  auto* box = CreateNode();
  box->SetComposeLayout(true);
  box->AppendChild(child);
  const ComposeConstraints range{0, 100, 0, 80};
  box->UpdateMeasureWithComposeConstraints(range, true);
  ASSERT_EQ(1u, measurement.ranges.size());

  child->UpdateMeasureWithComposeConstraints({0, 90, 0, 80}, true);
  ASSERT_EQ(2u, measurement.ranges.size());
  box->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(2u, measurement.ranges.size());
  EXPECT_FLOAT_EQ(20, box->GetBorderBoundWidth());

  box->MarkDirty();
  box->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(3u, measurement.ranges.size());
  ExpectRange(measurement.ranges.back(), 0, 100, 0, 80);
}

TEST_F(BoxLayoutAlgorithmTest, CssRepeatedDirtyKeepsMeasurementCache) {
  NativeMeasurement measurement;
  auto* child = CreateNativeChild(&measurement);
  const Constraints range{OneSideConstraint::AtMost(100),
                          OneSideConstraint::AtMost(80)};
  child->UpdateMeasureByPlatform(range, true);
  ASSERT_EQ(1u, measurement.ranges.size());
  ASSERT_TRUE(child->IsDirty());
  ASSERT_TRUE(child->IsInflowSubTreeInSyncWithLastMeasurement());

  child->MarkDirty();
  child->UpdateMeasureByPlatform(range, true);
  EXPECT_EQ(1u, measurement.ranges.size());
  EXPECT_FLOAT_EQ(20, child->GetBorderBoundWidth());

  child->MarkNotDirty();
  child->MarkDirty();
  child->UpdateMeasureByPlatform(range, true);
  EXPECT_EQ(2u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest, CssDirtyPropagationStopsAtAlreadyDirtyAncestor) {
  NativeMeasurement measurement;
  auto* child = CreateNativeChild(&measurement);
  auto* box = CreateNode();
  box->AppendChild(child);
  const Constraints range{OneSideConstraint::AtMost(100),
                          OneSideConstraint::AtMost(80)};
  box->UpdateMeasureByPlatform(range, true);
  ASSERT_EQ(1u, measurement.ranges.size());
  ASSERT_TRUE(box->IsDirty());
  ASSERT_TRUE(box->IsInflowSubTreeInSyncWithLastMeasurement());

  child->MarkNotDirty();
  child->MarkDirty();
  EXPECT_FALSE(child->IsInflowSubTreeInSyncWithLastMeasurement());
  EXPECT_TRUE(box->IsInflowSubTreeInSyncWithLastMeasurement());
  box->UpdateMeasureByPlatform(range, true);
  EXPECT_EQ(1u, measurement.ranges.size());

  box->MarkNotDirty();
  child->MarkNotDirty();
  child->MarkDirty();
  EXPECT_FALSE(box->IsInflowSubTreeInSyncWithLastMeasurement());
  box->UpdateMeasureByPlatform(range, true);
  EXPECT_EQ(2u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest, CssForceDirtyClearsAlreadyDirtyAncestorCache) {
  NativeMeasurement measurement;
  auto* child = CreateNativeChild(&measurement);
  auto* box = CreateNode();
  box->AppendChild(child);
  const Constraints range{OneSideConstraint::AtMost(100),
                          OneSideConstraint::AtMost(80)};
  box->UpdateMeasureByPlatform(range, true);
  ASSERT_EQ(1u, measurement.ranges.size());
  ASSERT_TRUE(child->IsDirty());
  ASSERT_TRUE(box->IsDirty());

  child->MarkDirtyAndRequestLayout(true);
  EXPECT_FALSE(child->IsInflowSubTreeInSyncWithLastMeasurement());
  EXPECT_FALSE(box->IsInflowSubTreeInSyncWithLastMeasurement());
  box->UpdateMeasureByPlatform(range, true);
  EXPECT_EQ(2u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest,
       ComposeCacheInvalidatesParentOnDirtyCssChildMeasure) {
  NativeMeasurement measurement;
  auto* child = CreateNativeChild(&measurement);
  auto* flex = CreateNode(DisplayType::kFlex);
  flex->AppendChild(child);
  auto* box = CreateNode();
  box->SetComposeLayout(true);
  box->AppendChild(flex);
  const ComposeConstraints range{0, 100, 0, 80};
  box->UpdateMeasureWithComposeConstraints(range, true);
  const auto initial = measurement.ranges.size();
  ASSERT_GT(initial, 0u);
  box->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(initial, measurement.ranges.size());
  // CSS dirty propagation begins after the previous layout has been committed.
  for (auto* node : {box, flex, child}) node->MarkNotDirty();
  child->MarkDirty();
  // Fixed dimensions force a real CSS measure; a tighter AtMost range can
  // legitimately reuse the existing CSS cache.
  child->UpdateMeasure(
      {OneSideConstraint::Definite(30), OneSideConstraint::Definite(20)}, true);
  const auto independent = measurement.ranges.size();
  ASSERT_GT(independent, initial);
  box->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_GT(measurement.ranges.size(), independent);
  EXPECT_FLOAT_EQ(20, box->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(20, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(15, child->GetBorderBoundHeight());
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheIncludesResolvedPercentageInsets) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  child->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(50));
  child->GetCSSMutableStyle()->SetPaddingLeft(
      NLength::MakePercentageNLength(10));
  // Equal fixed border widths still resolve different percentage padding.
  child->UpdateMeasureWithComposeConstraints({0, 100, 0, 80}, true);
  child->UpdateMeasureWithComposeConstraints({0, 200, 0, 80}, true);
  ASSERT_EQ(2u, measurement.ranges.size());
  ExpectRange(measurement.ranges[0], 40, 40, 0, 80);
  ExpectRange(measurement.ranges[1], 30, 30, 0, 80);
  EXPECT_FLOAT_EQ(20, child->GetLayoutPaddingLeft());
  child->UpdateMeasureWithComposeConstraints({0, 200, 0, 80}, true);
  EXPECT_EQ(2u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheIsolatedFromCssMeasurements) {
  NativeMeasurement measurement;
  auto* child = CreateNativeChild(&measurement);
  child->SetComposeLayout(true);
  const ComposeConstraints range{0, 100, 0, 80};
  child->UpdateMeasureWithComposeConstraints(range, true);
  child->UpdateMeasureWithComposeConstraints(range, true);
  ASSERT_EQ(1u, measurement.ranges.size());
  measurement.preferred.width_ = 30;
  child->SetComposeLayout(false);
  child->UpdateMeasureByPlatform(range.ToCssConstraints(), true);
  ASSERT_EQ(2u, measurement.ranges.size());
  EXPECT_FLOAT_EQ(30, child->GetBorderBoundWidth());
  child->UpdateMeasureByPlatform(range.ToCssConstraints(), true);
  EXPECT_EQ(2u, measurement.ranges.size());
  measurement.preferred.width_ = 40;
  child->SetComposeLayout(true);
  child->UpdateMeasureWithComposeConstraints(range, true);
  ASSERT_EQ(3u, measurement.ranges.size());
  EXPECT_FLOAT_EQ(40, child->GetBorderBoundWidth());
  child->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(3u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest, BoxConstraintPathChangesBeforePlacement) {
  ComposeMeasurement regular_measurement;
  ComposeMeasurement match_measurement;
  auto* regular = CreateComposeChild(&regular_measurement);
  auto* match = CreateComposeChild(&match_measurement);
  match->GetCSSMutableStyle()->SetXBoxMatchParentSize(true);
  auto* box = CreatePaddedBox(regular);
  box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
  box->AppendChild(match);
  const auto compose_measure = regular->GetSLComposeMeasureFunc();
  const SLMeasureFunc css_measure = [](void* context,
                                       const Constraints& constraints, bool) {
    auto* measurement = static_cast<ComposeMeasurement*>(context);
    const float width = constraints[kHorizontal].Size();
    const float height = constraints[kVertical].Size();
    measurement->ranges.push_back({width, width, height, height});
    return FloatSize(width, height, 0);
  };

  for (bool compose : {true, false, true}) {
    SCOPED_TRACE(compose);
    for (auto* node : {box, regular, match}) node->SetComposeLayout(compose);
    for (auto* node : {regular, match}) {
      node->SetSLComposeMeasureFunc(compose ? compose_measure : nullptr);
      node->SetSLMeasureFunc(compose ? nullptr : css_measure);
    }
    if (compose) {
      box->GetCSSMutableStyle()->SetWidth(NLength::MakeAutoNLength());
      box->GetCSSMutableStyle()->SetHeight(NLength::MakeAutoNLength());
      box->UpdateMeasureWithComposeConstraints({50, 100, 30, 80}, true);
      ExpectRange(regular_measurement.ranges.back(), 38, 88, 26, 76);
    } else {
      box->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(70));
      box->GetCSSMutableStyle()->SetHeight(NLength::MakeUnitNLength(40));
      box->UpdateMeasureByPlatform(
          {OneSideConstraint::Definite(70), OneSideConstraint::Definite(40)},
          true);
      ExpectRange(regular_measurement.ranges.back(), 58, 58, 36, 36);
    }
    const auto before_alignment = match_measurement.ranges.size();
    box->AlignmentByPlatform(0, 0);
    ASSERT_EQ(before_alignment + 1, match_measurement.ranges.size());
    const float width = compose ? 38 : 58;
    const float height = compose ? 26 : 36;
    ExpectRange(match_measurement.ranges.back(), width, width, height, height);
    EXPECT_FLOAT_EQ(width, match->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(height, match->GetBorderBoundHeight());
    for (auto* node : {box, regular, match}) node->MarkNotDirty();
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeIdentityCachesNativeCallbacksAndLeaves) {
  for (bool with_callback : {false, true}) {
    SCOPED_TRACE(with_callback);
    NativeMeasurement measurement;
    auto* child =
        with_callback ? CreateNativeChild(&measurement) : CreateNode();
    child->SetComposeLayout(true);
    child->GetCSSMutableStyle()->SetMinWidth(NLength::MakeUnitNLength(50));
    child->GetCSSMutableStyle()->SetMaxWidth(NLength::MakeUnitNLength(100));
    child->GetCSSMutableStyle()->SetMinHeight(NLength::MakeUnitNLength(30));
    child->GetCSSMutableStyle()->SetMaxHeight(NLength::MakeUnitNLength(80));
    for (bool final_measure : {false, true}) {
      SCOPED_TRACE(final_measure);
      const auto size =
          child->UpdateMeasureByPlatform(Constraints(), final_measure);
      EXPECT_FLOAT_EQ(50, size.width_);
      EXPECT_FLOAT_EQ(30, size.height_);
      EXPECT_FLOAT_EQ(50, child->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(30, child->GetBorderBoundHeight());
      const auto count = measurement.ranges.size();
      child->UpdateMeasureByPlatform(Constraints(), final_measure);
      EXPECT_EQ(count, measurement.ranges.size());
      // A geometry change invalidates the Compose cache even without a
      // layout algorithm, so the next measurement must restore the bounds.
      child->SetBorderBoundWidth(70);
      child->UpdateMeasureByPlatform(Constraints(), final_measure);
      EXPECT_FLOAT_EQ(50, child->GetBorderBoundWidth());
      if (with_callback) EXPECT_GT(measurement.ranges.size(), count);
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheRejectsExternallyChangedGeometry) {
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  const ComposeConstraints range{0, 100, 0, 80};
  child->UpdateMeasureWithComposeConstraints(range, true);
  child->SetBorderBoundWidth(50);
  child->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(2u, measurement.ranges.size());
  EXPECT_FLOAT_EQ(20, child->GetBorderBoundWidth());
  child->SetBaseline(7);
  child->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_EQ(3u, measurement.ranges.size());
  EXPECT_FLOAT_EQ(15, child->GetBaseline());
}

TEST_F(BoxLayoutAlgorithmTest, ComposeCacheRebuildsRemovedBoxAlgorithm) {
  struct RemovableBox : LayoutObject {
    using LayoutObject::LayoutObject;
    using LayoutObject::RemoveAlgorithm;
  };
  LayoutComputedStyle style(1.f);
  style.SetDisplay(DisplayType::kXBox);
  style.SetBoxSizing(BoxSizingType::kBorderBox);
  style.SetPaddingLeft(NLength::MakeUnitNLength(12));
  ComposeMeasurement measurement;
  auto* child = CreateComposeChild(&measurement);
  RemovableBox box(child->GetLayoutConfigs(), &style);
  box.SetComposeLayout(true);
  box.AppendChild(child);
  const ComposeConstraints range{0, 100, 0, 80};
  box.UpdateMeasureWithComposeConstraints(range, true);
  box.UpdateAlignment();
  box.RemoveAlgorithm();
  child->SetBorderBoundLeftFromParentPaddingBound(99);
  box.UpdateMeasureWithComposeConstraints(range, true);
  box.UpdateAlignment();
  EXPECT_FLOAT_EQ(12, child->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_EQ(1u, measurement.ranges.size());
}

TEST_F(BoxLayoutAlgorithmTest, CacheStoresMeasurementAfterInvalidation) {
  struct Measurement {
    LayoutObject* node;
    int count = 0;
  };
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    auto* child = CreateNode();
    child->SetComposeLayout(compose);
    Measurement measurement{child};
    child->SetContext(&measurement);
    const auto measure = [](void* context, const auto&, bool) {
      auto* measurement = static_cast<Measurement*>(context);
      ++measurement->count;
      measurement->node->ClearCache();
      return FloatSize(20, 15, 0);
    };
    if (compose) {
      child->SetSLComposeMeasureFunc(measure);
    } else {
      child->SetSLMeasureFunc(measure);
    }
    const ComposeConstraints range{0, 100, 0, 80};
    for (int pass = 0; pass < 3; ++pass) {
      if (compose) {
        child->UpdateMeasureWithComposeConstraints(range, true);
      } else {
        child->UpdateMeasureByPlatform(range.ToCssConstraints(), true);
      }
      EXPECT_EQ(1, measurement.count);
      EXPECT_FLOAT_EQ(20, child->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(15, child->GetBorderBoundHeight());
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, CacheStoresMeasurementAfterDescendantDirty) {
  struct Measurement {
    LayoutObject* node;
    int count = 0;
  };
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    auto* child = CreateNode();
    auto* box = CreateNode();
    child->SetComposeLayout(compose);
    box->SetComposeLayout(compose);
    box->AppendChild(child);
    Measurement measurement{child};
    child->SetContext(&measurement);
    const auto measure = [](void* context, const auto&, bool) {
      auto* measurement = static_cast<Measurement*>(context);
      ++measurement->count;
      measurement->node->MarkDirtyAndRequestLayout(true);
      return FloatSize(20, 15, 0);
    };
    if (compose) {
      child->SetSLComposeMeasureFunc(measure);
    } else {
      child->SetSLMeasureFunc(measure);
    }
    const ComposeConstraints range{0, 100, 0, 80};
    for (int pass = 0; pass < 3; ++pass) {
      if (compose) {
        box->UpdateMeasureWithComposeConstraints(range, true);
      } else {
        box->UpdateMeasureByPlatform(range.ToCssConstraints(), true);
      }
      EXPECT_EQ(1, measurement.count);
      EXPECT_FLOAT_EQ(20, box->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(15, box->GetBorderBoundHeight());
    }
  }
}

}  // namespace
}  // namespace starlight
}  // namespace lynx
