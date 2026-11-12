// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/services/replay/layout_tree_testbench.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace starlight {
namespace {

struct MeasureContext {
  FloatSize size;
  int count = 0;
};

FloatSize CountingMeasure(void* context, const Constraints&, bool) {
  auto* measure_context = static_cast<MeasureContext*>(context);
  ++measure_context->count;
  return measure_context->size;
}

class GridGeometryTest : public ::testing::Test {
 protected:
  void SetUp() override { configs_.SetQuirksMode(base::Version(3, 1)); }

  ComputedCSSStyle* CreateStyle() {
    styles_.push_back(std::make_unique<ComputedCSSStyle>(1.f, 1.f));
    return styles_.back().get();
  }

  LayoutObject* CreateNode(ComputedCSSStyle* style) {
    nodes_.push_back(std::make_unique<LayoutObject>(
        configs_, style->GetLayoutComputedStyle()));
    return nodes_.back().get();
  }

  LayoutObject* CreateMeasuredItem(float width, float height,
                                   MeasureContext* context) {
    auto* style = CreateStyle();
    style->GetLayoutComputedStyle()->SetWidth(NLength::MakeUnitNLength(width));
    style->GetLayoutComputedStyle()->SetHeight(
        NLength::MakeUnitNLength(height));
    auto* node = CreateNode(style);
    context->size = FloatSize(width, height);
    node->SetContext(context);
    node->SetSLMeasureFunc(CountingMeasure);
    return node;
  }

  LayoutObject* CreateGrid(float width, float height,
                           const std::vector<float>& columns,
                           const std::vector<float>& rows, float column_gap = 0,
                           float row_gap = 0) {
    auto* style = CreateStyle();
    auto* layout_style = style->GetLayoutComputedStyle();
    layout_style->SetDisplay(DisplayType::kGrid);
    layout_style->SetWidth(NLength::MakeUnitNLength(width));
    layout_style->SetHeight(NLength::MakeUnitNLength(height));
    layout_style->SetColumnGap(NLength::MakeUnitNLength(column_gap));
    layout_style->SetRowGap(NLength::MakeUnitNLength(row_gap));
    for (float column : columns) {
      layout_style->grid_data_.Access()
          ->grid_template_columns_min_track_sizing_function_.push_back(
              NLength::MakeUnitNLength(column));
      layout_style->grid_data_.Access()
          ->grid_template_columns_max_track_sizing_function_.push_back(
              NLength::MakeUnitNLength(column));
    }
    for (float row : rows) {
      layout_style->grid_data_.Access()
          ->grid_template_rows_min_track_sizing_function_.push_back(
              NLength::MakeUnitNLength(row));
      layout_style->grid_data_.Access()
          ->grid_template_rows_max_track_sizing_function_.push_back(
              NLength::MakeUnitNLength(row));
    }
    return CreateNode(style);
  }

  std::string Layout(LayoutObject* root) {
    root->ReLayout();
    return tasm::replay::LayoutTreeTestBench::GetLayoutTree(root);
  }

 private:
  LayoutConfigs configs_;
  std::vector<std::unique_ptr<ComputedCSSStyle>> styles_;
  std::vector<std::unique_ptr<LayoutObject>> nodes_;
};

TEST_F(GridGeometryTest, FixedTracks) {
  MeasureContext first;
  MeasureContext second;
  auto* root = CreateGrid(220, 80, {100, 100}, {80}, 20);
  root->AppendChild(CreateMeasuredItem(60, 40, &first));
  root->AppendChild(CreateMeasuredItem(80, 50, &second));

  EXPECT_EQ(
      R"({"width":220.0,"height":80.0,"offset_top":0.0,"offset_left":0.0,"content":[0.0,0.0,220.0,0.0,220.0,80.0,0.0,80.0],"padding":[0.0,0.0,220.0,0.0,220.0,80.0,0.0,80.0],"border":[0.0,0.0,220.0,0.0,220.0,80.0,0.0,80.0],"margin":[0.0,0.0,220.0,0.0,220.0,80.0,0.0,80.0],"children":[{"width":60.0,"height":40.0,"offset_top":0.0,"offset_left":0.0,"content":[0.0,0.0,60.0,0.0,60.0,40.0,0.0,40.0],"padding":[0.0,0.0,60.0,0.0,60.0,40.0,0.0,40.0],"border":[0.0,0.0,60.0,0.0,60.0,40.0,0.0,40.0],"margin":[0.0,0.0,60.0,0.0,60.0,40.0,0.0,40.0]},{"width":80.0,"height":50.0,"offset_top":0.0,"offset_left":120.0,"content":[120.0,0.0,200.0,0.0,200.0,50.0,120.0,50.0],"padding":[120.0,0.0,200.0,0.0,200.0,50.0,120.0,50.0],"border":[120.0,0.0,200.0,0.0,200.0,50.0,120.0,50.0],"margin":[120.0,0.0,200.0,0.0,200.0,50.0,120.0,50.0]}]})",
      Layout(root));
  EXPECT_EQ(1, first.count);
  EXPECT_EQ(1, second.count);
}

TEST_F(GridGeometryTest, SpanningItem) {
  MeasureContext wide;
  MeasureContext last;
  auto* root = CreateGrid(230, 100, {70, 70, 70}, {40, 50}, 10, 10);
  auto* wide_item = CreateMeasuredItem(150, 40, &wide);
  wide_item->GetCSSMutableStyle()->grid_data_.Access()->grid_column_span_ = 2;
  root->AppendChild(wide_item);
  root->AppendChild(CreateMeasuredItem(70, 50, &last));

  EXPECT_EQ(
      R"({"width":230.0,"height":100.0,"offset_top":0.0,"offset_left":0.0,"content":[0.0,0.0,230.0,0.0,230.0,100.0,0.0,100.0],"padding":[0.0,0.0,230.0,0.0,230.0,100.0,0.0,100.0],"border":[0.0,0.0,230.0,0.0,230.0,100.0,0.0,100.0],"margin":[0.0,0.0,230.0,0.0,230.0,100.0,0.0,100.0],"children":[{"width":150.0,"height":40.0,"offset_top":0.0,"offset_left":0.0,"content":[0.0,0.0,150.0,0.0,150.0,40.0,0.0,40.0],"padding":[0.0,0.0,150.0,0.0,150.0,40.0,0.0,40.0],"border":[0.0,0.0,150.0,0.0,150.0,40.0,0.0,40.0],"margin":[0.0,0.0,150.0,0.0,150.0,40.0,0.0,40.0]},{"width":70.0,"height":50.0,"offset_top":0.0,"offset_left":160.0,"content":[160.0,0.0,230.0,0.0,230.0,50.0,160.0,50.0],"padding":[160.0,0.0,230.0,0.0,230.0,50.0,160.0,50.0],"border":[160.0,0.0,230.0,0.0,230.0,50.0,160.0,50.0],"margin":[160.0,0.0,230.0,0.0,230.0,50.0,160.0,50.0]}]})",
      Layout(root));
  EXPECT_EQ(1, wide.count);
  EXPECT_EQ(1, last.count);
}

TEST_F(GridGeometryTest, RtlPlacement) {
  MeasureContext first;
  MeasureContext second;
  auto* root = CreateGrid(210, 60, {100, 100}, {60}, 10);
  root->GetCSSMutableStyle()->SetDirection(DirectionType::kRtl);
  root->AppendChild(CreateMeasuredItem(40, 30, &first));
  root->AppendChild(CreateMeasuredItem(50, 30, &second));

  EXPECT_EQ(
      R"({"width":210.0,"height":60.0,"offset_top":0.0,"offset_left":0.0,"content":[0.0,0.0,210.0,0.0,210.0,60.0,0.0,60.0],"padding":[0.0,0.0,210.0,0.0,210.0,60.0,0.0,60.0],"border":[0.0,0.0,210.0,0.0,210.0,60.0,0.0,60.0],"margin":[0.0,0.0,210.0,0.0,210.0,60.0,0.0,60.0],"children":[{"width":40.0,"height":30.0,"offset_top":0.0,"offset_left":170.0,"content":[170.0,0.0,210.0,0.0,210.0,30.0,170.0,30.0],"padding":[170.0,0.0,210.0,0.0,210.0,30.0,170.0,30.0],"border":[170.0,0.0,210.0,0.0,210.0,30.0,170.0,30.0],"margin":[170.0,0.0,210.0,0.0,210.0,30.0,170.0,30.0]},{"width":50.0,"height":30.0,"offset_top":0.0,"offset_left":50.0,"content":[50.0,0.0,100.0,0.0,100.0,30.0,50.0,30.0],"padding":[50.0,0.0,100.0,0.0,100.0,30.0,50.0,30.0],"border":[50.0,0.0,100.0,0.0,100.0,30.0,50.0,30.0],"margin":[50.0,0.0,100.0,0.0,100.0,30.0,50.0,30.0]}]})",
      Layout(root));
  EXPECT_EQ(1, first.count);
  EXPECT_EQ(1, second.count);
}

// Zero decoration isolates the Compose-to-CSS algorithm dispatch. The decorated
// companion below checks content origins and percentage bases independently.
TEST_F(GridGeometryTest, ComposeGridUsesNativeTracksAndHonorsOuterRange) {
  MeasureContext a, b;
  auto* grid = CreateGrid(220, 80, {100, 100}, {80}, 20);
  grid->SetComposeLayout(true);
  auto* first = CreateMeasuredItem(60, 40, &a);
  auto* second = CreateMeasuredItem(80, 50, &b);
  first->SetComposeLayout(true);
  second->SetComposeLayout(true);
  grid->AppendChild(first);
  grid->AppendChild(second);
  const auto size =
      grid->UpdateMeasureWithComposeConstraints({220, 220, 80, 80}, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(220, size.width_);
  EXPECT_FLOAT_EQ(80, size.height_);
  EXPECT_FLOAT_EQ(120, second->GetBorderBoundLeftFromParentPaddingBound());
  auto next =
      grid->UpdateMeasureWithComposeConstraints({140, 140, 60, 60}, true);
  EXPECT_FLOAT_EQ(140, next.width_);
  EXPECT_FLOAT_EQ(60, next.height_);
}

TEST_F(GridGeometryTest, ComposeGridAppliesOrderedModifiersWithinCellBounds) {
  auto* grid = CreateGrid(0, 80, {100, 100}, {80}, 20);
  grid->GetCSSMutableStyle()->SetWidth(NLength::MakeAutoNLength());
  grid->SetComposeLayout(true);
  grid->SetComposeLayoutModifiers(std::make_shared<ComposeLayoutModifiers>(
      ComposeLayoutModifiers{{ComposeLayoutModifier::Type::kFill,
                              ComposeLayoutModifier::kWidth, 0.5}}));

  auto* child = CreateNode(CreateStyle());
  child->SetComposeLayout(true);
  child->GetCSSMutableStyle()->grid_data_.Access()->justify_self_ =
      JustifyType::kStretch;
  child->SetComposeLayoutModifiers(
      std::make_shared<ComposeLayoutModifiers>(ComposeLayoutModifiers{
          {ComposeLayoutModifier::Type::kFill, ComposeLayoutModifier::kWidth,
           0.5},
          {ComposeLayoutModifier::Type::kSize, ComposeLayoutModifier::kWidth,
           90},
          {ComposeLayoutModifier::Type::kAspectRatio, 0, 2}}));
  child->SetSLComposeMeasureFunc(
      [](void*, const ComposeConstraints& constraints, bool) {
        return FloatSize(constraints.min_width, constraints.min_height);
      });
  grid->AppendChild(child);

  auto* stretched_child = CreateNode(CreateStyle());
  stretched_child->SetComposeLayout(true);
  stretched_child->GetCSSMutableStyle()->grid_data_.Access()->justify_self_ =
      JustifyType::kStretch;
  stretched_child->SetSLComposeMeasureFunc(child->GetSLComposeMeasureFunc());
  grid->AppendChild(stretched_child);

  const auto size =
      grid->UpdateMeasureWithComposeConstraints({0, 440, 80, 80}, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(220, size.width_);
  EXPECT_FLOAT_EQ(50, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(25, child->GetBorderBoundHeight());
  EXPECT_FLOAT_EQ(100, stretched_child->GetBorderBoundWidth());

  stretched_child->SetComposeLayoutModifiers(
      std::make_shared<ComposeLayoutModifiers>(
          ComposeLayoutModifiers{{ComposeLayoutModifier::Type::kSize,
                                  ComposeLayoutModifier::kWidth, 60}}));
  grid->UpdateMeasureWithComposeConstraints({0, 440, 80, 80}, true);
  EXPECT_FLOAT_EQ(60, stretched_child->GetBorderBoundWidth());
}

TEST_F(GridGeometryTest, ComposePercentagesSubtractGapsButCssIsUnchanged) {
  for (bool compose : {false, true}) {
    MeasureContext a, b;
    auto* grid = CreateGrid(234, 104, {}, {80}, 20);
    grid->SetComposeLayout(compose);
    auto* style = grid->GetCSSMutableStyle();
    style->SetBoxSizing(BoxSizingType::kBorderBox);
    style->SetPaddingLeft(NLength::MakeUnitNLength(3));
    style->SetPaddingRight(NLength::MakeUnitNLength(7));
    style->SetPaddingTop(NLength::MakeUnitNLength(5));
    style->SetPaddingBottom(NLength::MakeUnitNLength(9));
    style->SetBorderLeftWidth(1);
    style->SetBorderRightWidth(3);
    style->SetBorderTopWidth(2);
    style->SetBorderBottomWidth(8);
    auto* data = style->grid_data_.Access();
    for (int i = 0; i < 2; ++i) {
      data->grid_template_columns_min_track_sizing_function_.push_back(
          NLength::MakePercentageNLength(50));
      data->grid_template_columns_max_track_sizing_function_.push_back(
          NLength::MakePercentageNLength(50));
    }
    auto* first = CreateMeasuredItem(10, 10, &a);
    auto* second = CreateMeasuredItem(10, 10, &b);
    second->GetCSSMutableStyle()->SetMarginLeft(NLength::MakeUnitNLength(2));
    grid->AppendChild(first);
    grid->AppendChild(second);
    if (compose) {
      grid->UpdateMeasureWithComposeConstraints({234, 234, 104, 104}, true);
      grid->UpdateAlignment();
    } else {
      Layout(grid);
    }
    // Content width: 234 - (3+7 padding) - (1+3 borders) = 220.
    // Compose: (220-20)*50%=100. CSS: 220*50%=110.
    // This getter's origin is the parent's padding edge, excluding its border.
    EXPECT_FLOAT_EQ((compose ? 100 : 110) + 20 + 3 + 2,
                    second->GetBorderBoundLeftFromParentPaddingBound());
  }
}

TEST_F(GridGeometryTest, ComposeImplicitFlowGrowsInTheSelectedDirection) {
  for (bool row : {false, true}) {
    MeasureContext a, b, c;
    auto* grid = CreateGrid(300, 200, {}, {}, 0, 0);
    grid->SetComposeLayout(true);
    grid->GetCSSMutableStyle()->grid_data_.Access()->grid_auto_flow_ =
        row ? GridAutoFlowType::kRow : GridAutoFlowType::kColumn;
    auto* first = CreateMeasuredItem(20, 10, &a);
    auto* second = CreateMeasuredItem(20, 10, &b);
    auto* third = CreateMeasuredItem(20, 10, &c);
    grid->AppendChild(first);
    grid->AppendChild(second);
    grid->AppendChild(third);
    grid->UpdateMeasureWithComposeConstraints({300, 300, 200, 200}, true);
    grid->UpdateAlignment();
    if (row) {
      EXPECT_FLOAT_EQ(first->GetBorderBoundTopFromParentPaddingBound(),
                      third->GetBorderBoundTopFromParentPaddingBound());
      EXPECT_GT(third->GetBorderBoundLeftFromParentPaddingBound(),
                second->GetBorderBoundLeftFromParentPaddingBound());
    } else {
      EXPECT_FLOAT_EQ(first->GetBorderBoundLeftFromParentPaddingBound(),
                      third->GetBorderBoundLeftFromParentPaddingBound());
      EXPECT_GT(third->GetBorderBoundTopFromParentPaddingBound(),
                second->GetBorderBoundTopFromParentPaddingBound());
    }
  }
}

TEST_F(GridGeometryTest, EmptyComposeGridStillMeasuresExplicitTracks) {
  auto* grid = CreateGrid(0, 0, {80, 100}, {30, 40}, 8, 6);
  auto* style = grid->GetCSSMutableStyle();
  style->SetWidth(NLength::MakeAutoNLength());
  style->SetHeight(NLength::MakeAutoNLength());
  grid->SetComposeLayout(true);
  const auto size = grid->UpdateMeasureWithComposeConstraints(
      {0, ComposeConstraints::kInfinity, 0, ComposeConstraints::kInfinity},
      true);
  EXPECT_FLOAT_EQ(188, size.width_);
  EXPECT_FLOAT_EQ(76, size.height_);
}

TEST_F(GridGeometryTest, ComposeFlowWrapsAtExplicitTracksDespiteImplicitItems) {
  MeasureContext a, b, c, d;
  auto* grid = CreateGrid(400, 100, {80, 100}, {30, 40}, 10, 6);
  grid->SetComposeLayout(true);
  auto* positioned = CreateMeasuredItem(10, 10, &a);
  positioned->GetCSSMutableStyle()->grid_data_.Access()->grid_column_start_ = 4;
  positioned->GetCSSMutableStyle()->grid_data_.Access()->grid_row_start_ = 1;
  grid->AppendChild(positioned);
  auto* first = CreateMeasuredItem(10, 10, &b);
  auto* second = CreateMeasuredItem(10, 10, &c);
  auto* third = CreateMeasuredItem(10, 10, &d);
  grid->AppendChild(first);
  grid->AppendChild(second);
  grid->AppendChild(third);
  grid->UpdateMeasureWithComposeConstraints({400, 400, 100, 100}, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(0, first->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(90, second->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(0, third->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(36, third->GetBorderBoundTopFromParentPaddingBound());
}

TEST_F(GridGeometryTest, ComposePercentageAndFlexTracksUseBoundedMaxSize) {
  for (bool percentage : {false, true}) {
    MeasureContext a, b;
    auto* grid = CreateGrid(0, 20, {}, {20}, 20);
    grid->SetComposeLayout(true);
    auto* style = grid->GetCSSMutableStyle();
    style->SetWidth(NLength::MakeAutoNLength());
    auto* data = style->grid_data_.Access();
    for (int i = 1; i <= 2; ++i) {
      const auto track = percentage ? NLength::MakePercentageNLength(50)
                                    : NLength::MakeFrNLength(i);
      data->grid_template_columns_min_track_sizing_function_.push_back(track);
      data->grid_template_columns_max_track_sizing_function_.push_back(track);
    }
    grid->AppendChild(CreateMeasuredItem(10, 10, &a));
    auto* second = CreateMeasuredItem(10, 10, &b);
    grid->AppendChild(second);
    const auto size =
        grid->UpdateMeasureWithComposeConstraints({0, 220, 20, 20}, true);
    grid->UpdateAlignment();
    EXPECT_FLOAT_EQ(220, size.width_);
    EXPECT_NEAR(percentage ? 120.f : 20.f + 200.f / 3.f,
                second->GetBorderBoundLeftFromParentPaddingBound(), 0.01f);
  }
}

TEST_F(GridGeometryTest, CssFlowUsesImplicitTracksForAutoPlacement) {
  MeasureContext a, b, c, d;
  auto* grid = CreateGrid(400, 100, {80, 100}, {30, 40}, 10, 6);
  auto* positioned = CreateMeasuredItem(10, 10, &a);
  positioned->GetCSSMutableStyle()->grid_data_.Access()->grid_column_start_ = 4;
  positioned->GetCSSMutableStyle()->grid_data_.Access()->grid_row_start_ = 1;
  grid->AppendChild(positioned);
  grid->AppendChild(CreateMeasuredItem(10, 10, &b));
  grid->AppendChild(CreateMeasuredItem(10, 10, &c));
  auto* third = CreateMeasuredItem(10, 10, &d);
  grid->AppendChild(third);
  Layout(grid);
  EXPECT_FLOAT_EQ(200, third->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(0, third->GetBorderBoundTopFromParentPaddingBound());
}

TEST_F(GridGeometryTest, SpanningOccupancySurvivesComposeSearchClipping) {
  for (bool compose : {false, true}) {
    MeasureContext a, b, c, d;
    auto* grid = CreateGrid(400, 100, {80, 100}, {30, 40}, 10, 6);
    grid->SetComposeLayout(compose);
    auto* positioned = CreateMeasuredItem(10, 10, &a);
    auto* data = positioned->GetCSSMutableStyle()->grid_data_.Access();
    data->grid_column_start_ = 2;
    data->grid_column_end_ = 7;
    data->grid_row_start_ = 1;
    grid->AppendChild(positioned);
    auto* first = CreateMeasuredItem(10, 10, &b);
    auto* second = CreateMeasuredItem(10, 10, &c);
    auto* third = CreateMeasuredItem(10, 10, &d);
    grid->AppendChild(first);
    grid->AppendChild(second);
    grid->AppendChild(third);
    if (compose) {
      grid->UpdateMeasureWithComposeConstraints({400, 400, 100, 100}, true);
      grid->UpdateAlignment();
    } else {
      Layout(grid);
    }
    EXPECT_FLOAT_EQ(0, first->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(0, first->GetBorderBoundTopFromParentPaddingBound());
    EXPECT_FLOAT_EQ(0, second->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(36, second->GetBorderBoundTopFromParentPaddingBound());
    EXPECT_FLOAT_EQ(90, third->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(36, third->GetBorderBoundTopFromParentPaddingBound());
  }
}

}  // namespace
}  // namespace starlight
}  // namespace lynx
