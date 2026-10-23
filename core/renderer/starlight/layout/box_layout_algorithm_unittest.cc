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

  LayoutObject* CreatePaddedBox(LayoutObject* child) {
    auto* box = CreateNode();
    box->GetCSSMutableStyle()->SetPaddingLeft(NLength::MakeUnitNLength(12));
    box->GetCSSMutableStyle()->SetPaddingTop(NLength::MakeUnitNLength(4));
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

}  // namespace
}  // namespace starlight
}  // namespace lynx
