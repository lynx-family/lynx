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

TEST_F(GridGeometryTest, ComposeGridUsesNativeTracksAndHonorsOuterRange) {
  MeasureContext a, b;
  auto* grid = CreateGrid(220, 80, {100, 100}, {80});
  grid->GetCSSMutableStyle()->SetColumnGap(NLength::MakePercentageNLength(10));
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
  EXPECT_FLOAT_EQ(122, second->GetBorderBoundLeftFromParentPaddingBound());
  const auto next =
      grid->UpdateMeasureWithComposeConstraints({140, 140, 60, 60}, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(140, next.width_);
  EXPECT_FLOAT_EQ(60, next.height_);
  EXPECT_FLOAT_EQ(0, first->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(114, second->GetBorderBoundLeftFromParentPaddingBound());
}

TEST_F(GridGeometryTest, ComposeGridMeasuresWithReplacedLayoutStyle) {
  MeasureContext first, second;
  auto* grid = CreateGrid(200, 80, {40, 40}, {30}, 10);
  grid->SetComposeLayout(true);
  grid->AppendChild(CreateMeasuredItem(10, 10, &first));
  auto* child = CreateMeasuredItem(10, 10, &second);
  grid->AppendChild(child);
  const ComposeConstraints bounds{200, 200, 80, 80};
  grid->UpdateMeasureWithComposeConstraints(bounds, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(50, child->GetBorderBoundLeftFromParentPaddingBound());

  auto* updated_style = CreateStyle()->GetLayoutComputedStyle();
  updated_style->CopyFrom(*grid->GetCSSStyle());
  updated_style->SetColumnGap(NLength::MakeUnitNLength(25));
  grid->SetLayoutStyle(updated_style);
  grid->UpdateMeasureWithComposeConstraints(bounds, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(65, child->GetBorderBoundLeftFromParentPaddingBound());
}

TEST_F(GridGeometryTest, ComposeGridPassesTrackBoundsToChildOnRemeasure) {
  auto* grid = CreateGrid(0, 0, {}, {40}, 10);
  grid->SetComposeLayout(true);
  auto* style = grid->GetCSSMutableStyle();
  style->SetWidth(NLength::MakeAutoNLength());
  style->SetHeight(NLength::MakeAutoNLength());
  style->SetPaddingLeft(NLength::MakeUnitNLength(8));
  style->SetPaddingRight(NLength::MakeUnitNLength(12));
  style->SetPaddingTop(NLength::MakeUnitNLength(6));
  auto* tracks = style->grid_data_.Access();
  const auto half = NLength::MakePercentageNLength(50);
  tracks->grid_template_columns_min_track_sizing_function_ = {half, half};
  tracks->grid_template_columns_max_track_sizing_function_ = {half, half};

  ComposeConstraints final_child_bounds;
  auto* child = CreateNode(CreateStyle());
  child->SetComposeLayout(true);
  child->GetCSSMutableStyle()->grid_data_.Access()->grid_column_start_ = 2;
  child->SetContext(&final_child_bounds);
  child->SetSLComposeMeasureFunc(
      [](void* context, const ComposeConstraints& bounds, bool final_measure) {
        if (final_measure) {
          *static_cast<ComposeConstraints*>(context) = bounds;
        }
        return FloatSize(bounds.ConstrainWidth(25), bounds.ConstrainHeight(12));
      });
  grid->AppendChild(child);

  for (float max_width : {150.f, 230.f}) {
    SCOPED_TRACE(max_width);
    const auto size = grid->UpdateMeasureWithComposeConstraints(
        {110, max_width, 46, 46}, true);
    grid->UpdateAlignment();

    const float track_width = (max_width - 8 - 12 - 10) / 2;
    const ComposeConstraints expected_child_bounds{0, track_width, 0, 40};
    EXPECT_EQ(expected_child_bounds, final_child_bounds);
    EXPECT_FLOAT_EQ(max_width, size.width_);
    EXPECT_FLOAT_EQ(track_width + 10 + 8,
                    child->GetBorderBoundLeftFromParentPaddingBound());
  }
}

TEST_F(GridGeometryTest, GridContainerSelectsChildMeasurementMode) {
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    auto* grid = CreateGrid(100, 80, {100}, {80});
    grid->SetComposeLayout(compose);
    MeasureContext measurement;
    auto* child = CreateMeasuredItem(60, 40, &measurement);
    child->SetComposeLayout(compose);
    child->SetComposeLayoutModifiers(std::make_shared<ComposeLayoutModifiers>(
        ComposeLayoutModifiers{{ComposeLayoutModifier::Type::kFill,
                                ComposeLayoutModifier::kWidth, 0.5}}));
    grid->AppendChild(child);

    if (compose) {
      grid->UpdateMeasureWithComposeConstraints({100, 100, 80, 80}, true);
    } else {
      Layout(grid);
    }
    EXPECT_FLOAT_EQ(compose ? 50.f : 60.f, child->GetBorderBoundWidth());
  }
}

TEST_F(GridGeometryTest, OnlyCssGridStretchesAutoSizedChildren) {
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    auto* grid = CreateGrid(100, 80, {100}, {80});
    grid->SetComposeLayout(compose);
    auto* child = CreateNode(CreateStyle());
    auto* style = child->GetCSSMutableStyle();
    style->SetAlignSelf(FlexAlignType::kStretch);
    style->grid_data_.Access()->justify_self_ = JustifyType::kStretch;
    MeasureContext measurement{FloatSize(60, 40)};
    child->SetContext(&measurement);
    child->SetSLMeasureFunc(CountingMeasure);
    grid->AppendChild(child);

    if (compose) {
      grid->UpdateMeasureWithComposeConstraints({100, 100, 80, 80}, true);
    } else {
      Layout(grid);
    }
    EXPECT_FLOAT_EQ(compose ? 60.f : 100.f, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(compose ? 40.f : 80.f, child->GetBorderBoundHeight());
  }
}

TEST_F(GridGeometryTest, CssGridMeasuresOutOfFlowChildren) {
  MeasureContext absolute, in_flow;
  auto* grid = CreateGrid(220, 80, {100, 100}, {80}, 20);
  auto* positioned = CreateMeasuredItem(60, 40, &absolute);
  positioned->GetCSSMutableStyle()->SetPosition(PositionType::kAbsolute);
  grid->AppendChild(positioned);
  auto* child = CreateMeasuredItem(60, 40, &in_flow);
  grid->AppendChild(child);

  Layout(grid);

  EXPECT_GT(absolute.count, 0);
  EXPECT_FLOAT_EQ(60, positioned->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(0, child->GetBorderBoundLeftFromParentPaddingBound());
}

TEST_F(GridGeometryTest, ComposeGridPlacesChildrenInSourceOrder) {
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    MeasureContext first_measure, second_measure;
    auto* grid = CreateGrid(220, 80, {100, 100}, {80}, 20);
    grid->SetComposeLayout(compose);
    auto* first = CreateMeasuredItem(60, 40, &first_measure);
    auto* second = CreateMeasuredItem(60, 40, &second_measure);
    first->SetComposeLayout(compose);
    second->SetComposeLayout(compose);
    first->GetCSSMutableStyle()->flex_data_.Access()->order_ = 1;
    grid->AppendChild(first);
    grid->AppendChild(second);

    if (compose) {
      grid->UpdateMeasureWithComposeConstraints({220, 220, 80, 80}, true);
      grid->UpdateAlignment();
    } else {
      Layout(grid);
    }

    EXPECT_FLOAT_EQ(compose ? 0.f : 120.f,
                    first->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(compose ? 120.f : 0.f,
                    second->GetBorderBoundLeftFromParentPaddingBound());
  }
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

  auto* filled_child = CreateNode(CreateStyle());
  filled_child->SetComposeLayout(true);
  filled_child->SetComposeLayoutModifiers(
      std::make_shared<ComposeLayoutModifiers>(
          ComposeLayoutModifiers{{ComposeLayoutModifier::Type::kFill,
                                  ComposeLayoutModifier::kWidth, 1},
                                 {ComposeLayoutModifier::Type::kFill,
                                  ComposeLayoutModifier::kHeight, 1}}));
  filled_child->SetSLComposeMeasureFunc(child->GetSLComposeMeasureFunc());
  grid->AppendChild(filled_child);

  const auto size =
      grid->UpdateMeasureWithComposeConstraints({0, 440, 80, 80}, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(220, size.width_);
  EXPECT_FLOAT_EQ(50, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(25, child->GetBorderBoundHeight());
  EXPECT_FLOAT_EQ(100, filled_child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(80, filled_child->GetBorderBoundHeight());

  filled_child->SetComposeLayoutModifiers(
      std::make_shared<ComposeLayoutModifiers>(
          ComposeLayoutModifiers{{ComposeLayoutModifier::Type::kSize,
                                  ComposeLayoutModifier::kWidth, 60}}));
  grid->UpdateMeasureWithComposeConstraints({0, 440, 80, 80}, true);
  EXPECT_FLOAT_EQ(60, filled_child->GetBorderBoundWidth());
}

TEST_F(GridGeometryTest, ComposeGridWrapsWithinWidthRange) {
  auto* grid = CreateGrid(200, 80, {100, 100}, {80});
  grid->SetComposeLayout(true);
  float widths[] = {10, 90};
  for (auto& width : widths) {
    auto* child = CreateNode(CreateStyle());
    child->SetComposeLayout(true);
    auto* style = child->GetCSSMutableStyle();
    style->SetWidth(NLength::MakeFitContentNLength());
    style->SetMinWidth(NLength::MakeUnitNLength(30));
    style->SetMaxWidth(NLength::MakeUnitNLength(70));
    child->SetContext(&width);
    child->SetSLComposeMeasureFunc(
        [](void* context, const ComposeConstraints& constraints, bool) {
          return FloatSize(*static_cast<float*>(context),
                           constraints.min_height);
        });
    grid->AppendChild(child);
  }
  grid->UpdateMeasureWithComposeConstraints({200, 200, 80, 80}, true);
  auto* first = static_cast<LayoutObject*>(grid->FirstChild());
  auto* second = static_cast<LayoutObject*>(first->Next());
  EXPECT_FLOAT_EQ(30, first->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(70, second->GetBorderBoundWidth());
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

TEST_F(GridGeometryTest, EmptyComposeGridUsesLeafMeasurement) {
  auto* grid = CreateGrid(0, 0, {80, 100}, {30, 40}, 8, 6);
  auto* style = grid->GetCSSMutableStyle();
  style->SetWidth(NLength::MakeAutoNLength());
  style->SetHeight(NLength::MakeAutoNLength());
  grid->SetComposeLayout(true);
  const ComposeConstraints constraints;
  const auto empty =
      grid->UpdateMeasureWithComposeConstraints(constraints, true);
  EXPECT_FLOAT_EQ(0, empty.width_);
  EXPECT_FLOAT_EQ(0, empty.height_);

  MeasureContext measurement;
  auto* child = CreateMeasuredItem(20, 10, &measurement);
  child->SetComposeLayout(true);
  grid->AppendChild(child);
  grid->MarkDirty();
  const auto populated =
      grid->UpdateMeasureWithComposeConstraints(constraints, true);
  ASSERT_FLOAT_EQ(188, populated.width_);
  ASSERT_FLOAT_EQ(76, populated.height_);

  grid->RemoveChild(child);
  grid->MarkDirty();
  const auto after_removal =
      grid->UpdateMeasureWithComposeConstraints(constraints, true);
  EXPECT_FLOAT_EQ(0, after_removal.width_);
  EXPECT_FLOAT_EQ(0, after_removal.height_);
  child->SetBorderBoundLeftFromParentPaddingBound(50);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(50, child->GetBorderBoundLeftFromParentPaddingBound());
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
    EXPECT_FLOAT_EQ(percentage ? 120.f : 87.f,
                    second->GetBorderBoundLeftFromParentPaddingBound());
  }
}

TEST_F(GridGeometryTest, BoundedFlexTracksUseComposeOrCssDistribution) {
  for (bool compose : {false, true}) {
    for (float weight : {0.25f, 1.f}) {
      SCOPED_TRACE(compose);
      SCOPED_TRACE(weight);
      auto* grid = CreateGrid(131, 20, {}, {20}, 10);
      grid->SetComposeLayout(compose);
      auto* data = grid->GetCSSMutableStyle()->grid_data_.Access();
      data->grid_template_columns_min_track_sizing_function_ = {
          NLength::MakeUnitNLength(30), NLength::MakeUnitNLength(50)};
      data->grid_template_columns_max_track_sizing_function_ = {
          NLength::MakeFrNLength(weight), NLength::MakeFrNLength(weight)};

      MeasureContext a, b;
      grid->AppendChild(CreateMeasuredItem(10, 10, &a));
      auto* second = CreateMeasuredItem(10, 10, &b);
      grid->AppendChild(second);
      if (compose) {
        grid->UpdateMeasureWithComposeConstraints({131, 131, 20, 20}, true);
        grid->UpdateAlignment();
      } else {
        Layout(grid);
      }

      // Compose adds 21 and 20 to the two bases. CSS uses its original
      // partial-fill and flex sizing rules.
      EXPECT_FLOAT_EQ(compose ? 61.f : (weight < 1.f ? 40.f : 70.5f),
                      second->GetBorderBoundLeftFromParentPaddingBound());
    }
  }
}

TEST_F(GridGeometryTest, UnboundedComposeFlexTracksKeepTheirBaseSizes) {
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    auto* grid = CreateGrid(0, 20, {}, {20}, 10);
    grid->SetComposeLayout(compose);
    auto* style = grid->GetCSSMutableStyle();
    style->SetWidth(NLength::MakeAutoNLength());
    auto* data = style->grid_data_.Access();
    data->grid_template_columns_min_track_sizing_function_ = {
        NLength::MakeUnitNLength(30), NLength::MakeUnitNLength(50)};
    data->grid_template_columns_max_track_sizing_function_ = {
        NLength::MakeFrNLength(1), NLength::MakeFrNLength(2)};

    MeasureContext a, b;
    grid->AppendChild(CreateMeasuredItem(10, 10, &a));
    auto* second = CreateMeasuredItem(10, 10, &b);
    grid->AppendChild(second);

    if (compose) {
      grid->UpdateMeasureWithComposeConstraints(
          {310, ComposeConstraints::kInfinity, 20, 20}, true);
      grid->UpdateAlignment();
    } else {
      style->SetMinWidth(NLength::MakeUnitNLength(310));
      Layout(grid);
    }
    EXPECT_FLOAT_EQ(310, grid->GetBorderBoundWidth());
    // Compose keeps the first column at 30 plus the 10-unit gap. CSS
    // redistributes 300 across the two flexible columns.
    EXPECT_FLOAT_EQ(compose ? 40.f : 110.f,
                    second->GetBorderBoundLeftFromParentPaddingBound());
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

TEST_F(GridGeometryTest, ComposeAutoColumnAndCssLeadingImplicitColumn) {
  for (bool compose : {false, true}) {
    SCOPED_TRACE(compose);
    MeasureContext a, b;
    auto* grid = CreateGrid(200, 60, {50, 60}, {40}, 10);
    grid->SetComposeLayout(compose);
    auto* tracks = grid->GetCSSMutableStyle()->grid_data_.Access();
    const auto implicit_width = NLength::MakeUnitNLength(25);
    tracks->grid_auto_columns_min_track_sizing_function_ = {implicit_width};
    tracks->grid_auto_columns_max_track_sizing_function_ = {implicit_width};
    auto* backward = CreateMeasuredItem(20, 10, &a);
    auto* first_track = CreateMeasuredItem(10, 10, &b);
    backward->SetComposeLayout(compose);
    first_track->SetComposeLayout(compose);
    auto* backward_style = backward->GetCSSMutableStyle()->grid_data_.Access();
    if (!compose) {
      backward_style->grid_column_start_ = -4;
    }
    backward_style->grid_row_start_ = 1;
    auto* first_style = first_track->GetCSSMutableStyle()->grid_data_.Access();
    first_style->grid_column_start_ = 1;
    first_style->grid_row_start_ = 1;
    grid->AppendChild(first_track);
    grid->AppendChild(backward);
    if (compose) {
      grid->UpdateMeasureWithComposeConstraints({200, 200, 60, 60}, true);
      grid->UpdateAlignment();
    } else {
      Layout(grid);
    }
    EXPECT_FLOAT_EQ(compose ? 60.f : 0.f,
                    backward->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(0, backward->GetBorderBoundTopFromParentPaddingBound());
    EXPECT_FLOAT_EQ(compose ? 0.f : 35.f,
                    first_track->GetBorderBoundLeftFromParentPaddingBound());
  }
}

TEST_F(GridGeometryTest, ComposeGridResolvesNegativeColumnAfterResize) {
  MeasureContext first, second;
  auto* grid = CreateGrid(200, 80, {40, 40, 40}, {30, 30});
  grid->SetComposeLayout(true);
  auto* occupied = CreateMeasuredItem(10, 10, &first);
  auto* child = CreateMeasuredItem(10, 10, &second);
  occupied->SetComposeLayout(true);
  child->SetComposeLayout(true);
  auto* occupied_style = occupied->GetCSSMutableStyle()->grid_data_.Access();
  occupied_style->grid_row_start_ = 1;
  occupied_style->grid_column_start_ = 1;
  child->GetCSSMutableStyle()->grid_data_.Access()->grid_column_start_ = -3;
  grid->AppendChild(occupied);
  grid->AppendChild(child);

  auto* wide = grid->GetCSSMutableStyle();
  auto* narrow = CreateStyle()->GetLayoutComputedStyle();
  narrow->CopyFrom(*wide);
  auto* tracks = narrow->grid_data_.Access();
  tracks->grid_template_columns_min_track_sizing_function_ = {
      NLength::MakeUnitNLength(40)};
  tracks->grid_template_columns_max_track_sizing_function_ = {
      NLength::MakeUnitNLength(40)};

  const auto check_position = [grid, child](float left, float top) {
    grid->UpdateMeasureWithComposeConstraints({200, 200, 80, 80}, true);
    grid->UpdateAlignment();
    EXPECT_FLOAT_EQ(left, child->GetBorderBoundLeftFromParentPaddingBound());
    EXPECT_FLOAT_EQ(top, child->GetBorderBoundTopFromParentPaddingBound());
    EXPECT_EQ(-3, child->GetCSSStyle()->GetGridColumnStart());
  };
  check_position(40, 0);
  grid->SetLayoutStyle(narrow);
  check_position(0, 30);
  grid->SetLayoutStyle(wide);
  check_position(40, 0);
}

TEST_F(GridGeometryTest, ComposeGridClampsOnlyAuthoredAutoSpansOnFlowAxis) {
  for (bool row_flow : {false, true}) {
    for (int start : {0, -4, 1}) {
      SCOPED_TRACE(row_flow);
      SCOPED_TRACE(start);
      auto* grid = CreateGrid(200, 200, {40, 40}, {40, 40});
      grid->SetComposeLayout(true);
      auto* tracks = grid->GetCSSMutableStyle()->grid_data_.Access();
      tracks->grid_auto_flow_ =
          row_flow ? GridAutoFlowType::kRow : GridAutoFlowType::kColumn;
      const auto implicit = NLength::MakeUnitNLength(20);
      tracks->grid_auto_columns_min_track_sizing_function_ = {implicit};
      tracks->grid_auto_columns_max_track_sizing_function_ = {implicit};
      tracks->grid_auto_rows_min_track_sizing_function_ = {implicit};
      tracks->grid_auto_rows_max_track_sizing_function_ = {implicit};
      auto* child = CreateNode(CreateStyle());
      child->SetComposeLayout(true);
      auto* placement = child->GetCSSMutableStyle()->grid_data_.Access();
      placement->grid_row_start_ = row_flow ? 0 : start;
      placement->grid_column_start_ = row_flow ? start : 0;
      placement->grid_row_span_ = 3;
      placement->grid_column_span_ = 3;
      child->SetComposeLayoutModifiers(std::make_shared<ComposeLayoutModifiers>(
          ComposeLayoutModifiers{{ComposeLayoutModifier::Type::kFill,
                                  ComposeLayoutModifier::kWidth, 1},
                                 {ComposeLayoutModifier::Type::kFill,
                                  ComposeLayoutModifier::kHeight, 1}}));
      child->SetSLComposeMeasureFunc(
          [](void*, const ComposeConstraints& constraints, bool) {
            return FloatSize(constraints.min_width, constraints.min_height);
          });
      grid->AppendChild(child);

      grid->UpdateMeasureWithComposeConstraints({200, 200, 200, 200}, true);
      grid->UpdateAlignment();
      const float flow_size = start == 0 ? 80 : 100;
      EXPECT_FLOAT_EQ(row_flow ? flow_size : 100, child->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(row_flow ? 100 : flow_size,
                      child->GetBorderBoundHeight());
    }
  }
}

TEST_F(GridGeometryTest, ComposeAutoRowSearchesInFixedColumn) {
  MeasureContext a, b;
  auto* grid = CreateGrid(200, 100, {50, 60}, {30, 40}, 10, 6);
  grid->SetComposeLayout(true);
  auto* first_cell = CreateMeasuredItem(10, 10, &a);
  first_cell->SetComposeLayout(true);
  auto* first_style = first_cell->GetCSSMutableStyle()->grid_data_.Access();
  first_style->grid_row_start_ = 1;
  first_style->grid_column_start_ = 1;
  grid->AppendChild(first_cell);

  auto* backward = CreateMeasuredItem(10, 10, &b);
  backward->SetComposeLayout(true);
  auto* backward_style = backward->GetCSSMutableStyle()->grid_data_.Access();
  backward_style->grid_column_start_ = 1;
  grid->AppendChild(backward);

  grid->UpdateMeasureWithComposeConstraints({200, 200, 100, 100}, true);
  grid->UpdateAlignment();
  EXPECT_FLOAT_EQ(0, backward->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(36, backward->GetBorderBoundTopFromParentPaddingBound());
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
