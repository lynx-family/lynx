// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/box_layout_algorithm.h"

#include <memory>
#include <vector>

#include "core/renderer/starlight/layout/box_info.h"
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

  LayoutObject* CreatePaddedBox(LayoutObject* child) {
    auto* box = CreateNode();
    box->GetCSSMutableStyle()->SetPaddingLeft(NLength::MakeUnitNLength(12));
    box->GetCSSMutableStyle()->SetPaddingTop(NLength::MakeUnitNLength(4));
    box->AppendChild(child);
    return box;
  }

  LayoutObject* CreatePropagationBox(LayoutObject* child) {
    auto* box = CreateNode();
    auto* style = box->GetCSSMutableStyle();
    style->SetPaddingLeft(NLength::MakeUnitNLength(3));
    style->SetPaddingRight(NLength::MakeUnitNLength(7));
    style->SetPaddingTop(NLength::MakeUnitNLength(11));
    style->SetPaddingBottom(NLength::MakeUnitNLength(13));
    style->SetBorderLeftWidth(1);
    style->SetBorderRightWidth(2);
    style->SetBorderTopWidth(3);
    style->SetBorderBottomWidth(4);
    auto* child_style = child->GetCSSMutableStyle();
    child_style->SetMarginLeft(NLength::MakeUnitNLength(5));
    child_style->SetMarginRight(NLength::MakeUnitNLength(9));
    child_style->SetMarginTop(NLength::MakeUnitNLength(6));
    child_style->SetMarginBottom(NLength::MakeUnitNLength(10));
    box->AppendChild(child);
    return box;
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

TEST_F(BoxLayoutAlgorithmTest, PropagatedMinimumWinsOverChildMaximumAndResets) {
  auto* child = CreateNode();
  auto* child_style = child->GetCSSMutableStyle();
  child_style->SetWidth(NLength::MakeUnitNLength(80));
  child_style->SetHeight(NLength::MakeUnitNLength(40));
  child_style->SetMaxWidth(NLength::MakeUnitNLength(100));
  child_style->SetMaxHeight(NLength::MakeUnitNLength(50));
  auto* box = CreatePropagationBox(child);
  box->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(200));
  box->GetCSSMutableStyle()->SetHeight(NLength::MakeUnitNLength(120));
  box->ReLayout();
  EXPECT_FLOAT_EQ(80, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(40, child->GetBorderBoundHeight());
  const auto original_minimum = child->GetBoxInfo()->min_size_;
  const auto original_maximum = child->GetBoxInfo()->max_size_;

  for (bool propagate : {true, false, true}) {
    box->MarkUpdated();
    child->MarkUpdated();
    box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(propagate);
    box->MarkDirty();
    box->ReLayout();
    // Child border box: 200 - (3 + 7 + 1 + 2) - (5 + 9) = 173;
    // height: 120 - (11 + 13 + 3 + 4) - (6 + 10) = 73.
    EXPECT_FLOAT_EQ(propagate ? 173 : 80, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(propagate ? 73 : 40, child->GetBorderBoundHeight());
    for (auto dimension : {kHorizontal, kVertical}) {
      EXPECT_FLOAT_EQ(original_minimum[dimension],
                      child->GetBoxInfo()->min_size_[dimension]);
      EXPECT_FLOAT_EQ(original_maximum[dimension],
                      child->GetBoxInfo()->max_size_[dimension]);
    }
  }
}

TEST_F(BoxLayoutAlgorithmTest, PropagatedMinimumDoesNotFixAnAutoSizedBox) {
  for (bool larger_child : {false, true}) {
    auto* child = CreateNode();
    auto* style = child->GetCSSMutableStyle();
    style->SetWidth(NLength::MakeUnitNLength(larger_child ? 210 : 80));
    style->SetHeight(NLength::MakeUnitNLength(larger_child ? 140 : 40));
    style->SetMaxWidth(NLength::MakeUnitNLength(larger_child ? 250 : 100));
    style->SetMaxHeight(NLength::MakeUnitNLength(larger_child ? 200 : 50));
    auto* box = CreatePropagationBox(child);
    box->GetCSSMutableStyle()->SetMinWidth(NLength::MakeUnitNLength(160));
    box->GetCSSMutableStyle()->SetMinHeight(NLength::MakeUnitNLength(120));
    box->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
    box->ReLayout();
    // Minimum child border box is (160 - 13 - 14, 120 - 31 - 16).
    // Larger content remains free to grow, adding those box-model sums back.
    EXPECT_FLOAT_EQ(larger_child ? 210 : 133, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(larger_child ? 140 : 73, child->GetBorderBoundHeight());
    EXPECT_FLOAT_EQ(larger_child ? 237 : 160, box->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(larger_child ? 187 : 120, box->GetBorderBoundHeight());
  }
}

TEST_F(BoxLayoutAlgorithmTest, NestedBoxControlsFurtherMinimumPropagation) {
  // Zero spacing isolates the nested propagation boundary; decorated cases
  // above cover padding, border and margins on both axes.
  for (bool propagate_inner : {false, true}) {
    auto* outer = CreateNode();
    outer->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(200));
    outer->GetCSSMutableStyle()->SetHeight(NLength::MakeUnitNLength(120));
    outer->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(true);
    auto* inner = CreateNode();
    inner->GetCSSMutableStyle()->SetMaxWidth(NLength::MakeUnitNLength(100));
    inner->GetCSSMutableStyle()->SetMaxHeight(NLength::MakeUnitNLength(60));
    inner->GetCSSMutableStyle()->SetXBoxPropagateMinConstraints(
        propagate_inner);
    auto* content = CreateNode();
    content->GetCSSMutableStyle()->SetWidth(NLength::MakeUnitNLength(30));
    content->GetCSSMutableStyle()->SetHeight(NLength::MakeUnitNLength(20));
    content->GetCSSMutableStyle()->SetMaxWidth(NLength::MakeUnitNLength(50));
    content->GetCSSMutableStyle()->SetMaxHeight(NLength::MakeUnitNLength(40));
    inner->AppendChild(content);
    outer->AppendChild(inner);
    outer->ReLayout();
    EXPECT_FLOAT_EQ(200, inner->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(120, inner->GetBorderBoundHeight());
    EXPECT_FLOAT_EQ(propagate_inner ? 200 : 30, content->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(propagate_inner ? 120 : 20,
                    content->GetBorderBoundHeight());
  }
}

}  // namespace
}  // namespace starlight
}  // namespace lynx
