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

}  // namespace
}  // namespace starlight
}  // namespace lynx
