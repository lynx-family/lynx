// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <vector>

#include "core/renderer/dom/fragment/display_list_reader.h"
#include "core/renderer/starlight/style/borders_data.h"
#include "core/style/transform/matrix44.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class DisplayListBuilderTest : public ::testing::Test {
 protected:
  void SetUp() override { builder_ = std::make_unique<DisplayListBuilder>(); }

  void TearDown() override { builder_.reset(); }

  std::unique_ptr<DisplayListBuilder> builder_;
};

TEST_F(DisplayListBuilderTest, EmptyBuilder) {}

TEST_F(DisplayListBuilderTest, BeginOperation) {
  auto& result = builder_->Begin(0, PlatformRendererType::kView, 10.0f, 20.0f,
                                 100.0f, 200.0f);

  EXPECT_EQ(&result, builder_.get());  // Method chaining

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kBegin);
  EXPECT_EQ(item.payload.begin.id, 0);
  EXPECT_EQ(item.payload.begin.type,
            static_cast<int32_t>(PlatformRendererType::kView));
  EXPECT_FLOAT_EQ(item.payload.begin.x, 10.0f);
  EXPECT_FLOAT_EQ(item.payload.begin.y, 20.0f);
  EXPECT_FLOAT_EQ(item.payload.begin.w, 100.0f);
  EXPECT_FLOAT_EQ(item.payload.begin.h, 200.0f);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, EndOperation) {
  builder_->End();

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kEnd);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, FillOperation) {
  uint32_t color = 0xFF00FF00;
  builder_->Fill(color);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kFill);
  EXPECT_EQ(item.payload.fill.color, color);
  EXPECT_EQ(item.payload.fill.clip_index, -1);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, FillOperationWithClipIndex) {
  uint32_t color = 0xFF00FF00;
  int32_t clip_index = 5;
  builder_->Fill(color, clip_index);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kFill);
  EXPECT_EQ(item.payload.fill.color, color);
  EXPECT_EQ(item.payload.fill.clip_index, clip_index);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, DrawViewOperation) {
  int view_id = 42;
  builder_->DrawView(view_id);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kDrawView);
  EXPECT_EQ(item.payload.draw_view.view_id, view_id);
  EXPECT_FALSE(reader.HasNext());

  // Verify sub_layers_ tracking
  EXPECT_EQ(display_list.SubLayers().size(), 1u);
  EXPECT_EQ(display_list.SubLayers()[0], view_id);
}

TEST_F(DisplayListBuilderTest, DrawImageOperation) {
  int image_id = 123;
  int32_t box_index = 5;
  builder_->DrawImage(image_id, box_index);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kImage);
  EXPECT_EQ(item.payload.image.image_id, image_id);
  EXPECT_EQ(item.payload.image.box_index, box_index);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, DrawTextOperation) {
  int text_id = 456;
  int32_t box_index = 3;
  builder_->DrawText(text_id, box_index);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kText);
  EXPECT_EQ(item.payload.text.text_id, text_id);
  EXPECT_EQ(item.payload.text.box_index, box_index);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, TransformOperation) {
  transforms::Matrix44 matrix;
  matrix.preTranslate(50.0f, 100.0f, 0.0f);
  builder_->Transform(matrix);

  DisplayList display_list = builder_->Build();

  const SubtreeProperty* subtree_properties_data =
      display_list.GetSubtreePropertiesData();

  EXPECT_NE(subtree_properties_data, nullptr);
  EXPECT_EQ(display_list.GetSubtreePropertiesSize(), 1u);
  EXPECT_EQ(subtree_properties_data[0].type,
            DisplayListSubtreePropertyOpType::kTransform);

  // Check transform matrix values (column-major order)
  const float* matrix_data = matrix.Data();
  for (int i = 0; i < 16; ++i) {
    EXPECT_FLOAT_EQ(subtree_properties_data[0].data.transform[i],
                    matrix_data[i]);
  }
}

TEST_F(DisplayListBuilderTest, MethodChaining) {
  builder_->Begin(0, PlatformRendererType::kView, 0.0f, 0.0f, 100.0f, 100.0f)
      .Fill(0xFF0000FF)
      .DrawView(123)
      .DrawImage(456, -1)
      .DrawText(789, -1)
      .Transform(transforms::Matrix44())
      .End();

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kBegin);
  EXPECT_TRUE(reader.HasNext());
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kFill);
  EXPECT_TRUE(reader.HasNext());
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kDrawView);
  EXPECT_TRUE(reader.HasNext());
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kImage);
  EXPECT_TRUE(reader.HasNext());
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kText);
  EXPECT_TRUE(reader.HasNext());
  EXPECT_EQ(reader.Next().type, DisplayListOpType::kEnd);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, BorderOperation) {
  starlight::BordersData border;
  border.color_top = 0xFFFF0000;
  border.color_right = 0xFF00FF00;
  border.color_bottom = 0xFF0000FF;
  border.color_left = 0xFFFFFF00;
  border.style_top = static_cast<starlight::BorderStyleType>(1);
  border.style_right = static_cast<starlight::BorderStyleType>(2);
  border.style_bottom = static_cast<starlight::BorderStyleType>(3);
  border.style_left = static_cast<starlight::BorderStyleType>(4);

  builder_->Border(0, 1, border);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kBorder);
  EXPECT_EQ(item.payload.border.out_index, 0);
  EXPECT_EQ(item.payload.border.inner_index, 1);
  EXPECT_EQ(item.payload.border.colors[0], 0xFFFF0000);
  EXPECT_EQ(item.payload.border.colors[1], 0xFF00FF00);
  EXPECT_EQ(item.payload.border.colors[2], 0xFF0000FF);
  EXPECT_EQ(item.payload.border.colors[3], 0xFFFFFF00);
  EXPECT_EQ(item.payload.border.styles[0], 1);
  EXPECT_EQ(item.payload.border.styles[1], 2);
  EXPECT_EQ(item.payload.border.styles[2], 3);
  EXPECT_EQ(item.payload.border.styles[3], 4);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, ClipRectOperationWithoutRadius) {
  RoundedRectangle rect;
  rect.SetX(0.0f);
  rect.SetY(0.0f);
  rect.SetWidth(100.0f);
  rect.SetHeight(100.0f);

  builder_->ClipRect(rect);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kClipRect);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.x, 0.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.y, 0.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.w, 100.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.h, 100.0f);
  EXPECT_EQ(item.payload.clip_rect.has_radii, 0u);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, ClipRectOperationWithRadius) {
  RoundedRectangle rect;
  rect.SetX(0.0f);
  rect.SetY(0.0f);
  rect.SetWidth(100.0f);
  rect.SetHeight(100.0f);
  rect.SetRadiusXTopLeft(5.0f);
  rect.SetRadiusYTopLeft(6.0f);
  rect.SetRadiusXTopRight(7.0f);
  rect.SetRadiusYTopRight(8.0f);
  rect.SetRadiusXBottomRight(9.0f);
  rect.SetRadiusYBottomRight(10.0f);
  rect.SetRadiusXBottomLeft(11.0f);
  rect.SetRadiusYBottomLeft(12.0f);

  builder_->ClipRect(rect);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kClipRect);
  EXPECT_EQ(item.payload.clip_rect.has_radii, 1u);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[0], 5.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[1], 6.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[2], 7.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[3], 8.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[4], 9.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[5], 10.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[6], 11.0f);
  EXPECT_FLOAT_EQ(item.payload.clip_rect.radii[7], 12.0f);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, RecordBoxModelOperation) {
  RoundedRectangle rect;
  rect.SetX(10.0f);
  rect.SetY(20.0f);
  rect.SetWidth(100.0f);
  rect.SetHeight(200.0f);

  int32_t index = -1;
  builder_->RecordBoxModel(rect, index);

  EXPECT_EQ(index, 0);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kRecordBox);
  EXPECT_FLOAT_EQ(item.payload.record_box.x, 10.0f);
  EXPECT_FLOAT_EQ(item.payload.record_box.y, 20.0f);
  EXPECT_FLOAT_EQ(item.payload.record_box.w, 100.0f);
  EXPECT_FLOAT_EQ(item.payload.record_box.h, 200.0f);
  EXPECT_EQ(item.payload.record_box.has_radii, 0u);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, RecordBoxModelWithRadius) {
  RoundedRectangle rect;
  rect.SetX(0.0f);
  rect.SetY(0.0f);
  rect.SetWidth(100.0f);
  rect.SetHeight(100.0f);
  rect.SetRadiusXTopLeft(5.0f);
  rect.SetRadiusYTopLeft(5.0f);
  rect.SetRadiusXTopRight(5.0f);
  rect.SetRadiusYTopRight(5.0f);
  rect.SetRadiusXBottomRight(5.0f);
  rect.SetRadiusYBottomRight(5.0f);
  rect.SetRadiusXBottomLeft(5.0f);
  rect.SetRadiusYBottomLeft(5.0f);

  int32_t index = -1;
  builder_->RecordBoxModel(rect, index);

  EXPECT_EQ(index, 0);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(item.payload.record_box.has_radii, 1u);
  EXPECT_FLOAT_EQ(item.payload.record_box.radii[0], 5.0f);
  EXPECT_FLOAT_EQ(item.payload.record_box.radii[7], 5.0f);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, LinearGradientOperation) {
  base::Vector<uint32_t> colors;
  colors.push_back(0xFFFF0000);
  colors.push_back(0xFF00FF00);
  colors.push_back(0xFF0000FF);

  base::Vector<float> stops;
  stops.push_back(0.0f);
  stops.push_back(0.5f);
  stops.push_back(1.0f);

  builder_->LinearGradient(45.0f, colors, stops, 0, 1, 1, 1);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kLinearGradient);
  EXPECT_EQ(item.payload.linear_gradient.color_count, 3u);
  EXPECT_EQ(item.payload.linear_gradient.stop_count, 3u);
  EXPECT_FLOAT_EQ(item.payload.linear_gradient.angle, 45.0f);
  EXPECT_EQ(item.payload.linear_gradient.tiling_index, 0);
  EXPECT_EQ(item.payload.linear_gradient.clip_index, 1);
  EXPECT_EQ(item.payload.linear_gradient.repeat_x, 1);
  EXPECT_EQ(item.payload.linear_gradient.repeat_y, 1);

  const uint32_t* read_colors = reader.Colors(item);
  ASSERT_NE(read_colors, nullptr);
  EXPECT_EQ(read_colors[0], 0xFFFF0000);
  EXPECT_EQ(read_colors[1], 0xFF00FF00);
  EXPECT_EQ(read_colors[2], 0xFF0000FF);

  const float* read_stops = reader.Stops(item);
  ASSERT_NE(read_stops, nullptr);
  EXPECT_FLOAT_EQ(read_stops[0], 0.0f);
  EXPECT_FLOAT_EQ(read_stops[1], 0.5f);
  EXPECT_FLOAT_EQ(read_stops[2], 1.0f);

  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, LinearGradientOperationEmpty) {
  base::Vector<uint32_t> colors;
  base::Vector<float> stops;

  builder_->LinearGradient(0.0f, colors, stops, -1, -1, 0, 0);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kLinearGradient);
  EXPECT_EQ(item.payload.linear_gradient.color_count, 0u);
  EXPECT_EQ(item.payload.linear_gradient.stop_count, 0u);
  EXPECT_EQ(reader.Colors(item), nullptr);
  EXPECT_EQ(reader.Stops(item), nullptr);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, BoxShadowOperation) {
  builder_->BoxShadow(0, 1, 0x80000000, 10.0f,
                      DisplayListBuilder::BoxShadowClipMode::kOutset);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kBoxShadow);
  EXPECT_EQ(item.payload.box_shadow.shadow_box_index, 0);
  EXPECT_EQ(item.payload.box_shadow.clip_box_index, 1);
  EXPECT_EQ(item.payload.box_shadow.color, 0x80000000);
  EXPECT_FLOAT_EQ(item.payload.box_shadow.blur_radius, 10.0f);
  EXPECT_EQ(item.payload.box_shadow.clip_mode, 0);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, BoxShadowInsetOperation) {
  builder_->BoxShadow(0, 1, 0x80000000, 10.0f,
                      DisplayListBuilder::BoxShadowClipMode::kInset);

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kBoxShadow);
  EXPECT_EQ(item.payload.box_shadow.clip_mode, 1);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, OpacityOperation) {
  builder_->Opacity(0.5f);

  DisplayList display_list = builder_->Build();

  const SubtreeProperty* subtree_properties_data =
      display_list.GetSubtreePropertiesData();

  EXPECT_NE(subtree_properties_data, nullptr);
  EXPECT_EQ(display_list.GetSubtreePropertiesSize(), 1u);
  EXPECT_EQ(subtree_properties_data[0].type,
            DisplayListSubtreePropertyOpType::kOpacity);
  EXPECT_FLOAT_EQ(subtree_properties_data[0].data.opacity, 0.5f);
}

TEST_F(DisplayListBuilderTest, MarkRootNeedClipBounds) {
  builder_->MarkRootNeedClipBounds();

  DisplayList display_list = builder_->Build();

  EXPECT_TRUE(display_list.RootNeedClipBounds());
}

TEST_F(DisplayListBuilderTest, ClearBuilder) {
  builder_->Begin(0, PlatformRendererType::kView, 0.0f, 0.0f, 100.0f, 100.0f)
      .Fill(0xFF0000FF)
      .End();

  DisplayList display_list = builder_->Build();
  EXPECT_EQ(display_list.GetContentItemsSize(), 3u);

  builder_->Clear();

  DisplayList cleared_list = builder_->Build();
  EXPECT_EQ(cleared_list.GetContentItemsSize(), 0u);
}

TEST_F(DisplayListBuilderTest, ReserveBuilder) {
  builder_->Reserve(100);

  DisplayList display_list = builder_->Build();
  EXPECT_EQ(display_list.GetContentItemsSize(), 0u);
}

TEST_F(DisplayListBuilderTest, ComplexScene) {
  builder_->Begin(0, PlatformRendererType::kView, 0.0f, 0.0f, 100.0f, 100.0f)
      .Fill(0xFF0000FF)
      .End();

  DisplayList display_list = builder_->Build();

  DisplayListReader reader(display_list);
  EXPECT_TRUE(reader.HasNext());
  const auto& begin = reader.Next();
  EXPECT_EQ(begin.type, DisplayListOpType::kBegin);
  EXPECT_EQ(begin.payload.begin.id, 0);
  EXPECT_EQ(begin.payload.begin.type,
            static_cast<int32_t>(PlatformRendererType::kView));

  EXPECT_TRUE(reader.HasNext());
  const auto& fill = reader.Next();
  EXPECT_EQ(fill.type, DisplayListOpType::kFill);
  EXPECT_EQ(fill.payload.fill.color, 0xFF0000FF);

  EXPECT_TRUE(reader.HasNext());
  const auto& end = reader.Next();
  EXPECT_EQ(end.type, DisplayListOpType::kEnd);

  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListBuilderTest, RenderOffset) {
  DisplayListBuilder offset_builder(5.0f, 10.0f);
  DisplayList display_list = offset_builder.Build();

  const float* offset = display_list.GetRenderOffset();
  EXPECT_FLOAT_EQ(offset[0], 5.0f);
  EXPECT_FLOAT_EQ(offset[1], 10.0f);
}

TEST_F(DisplayListBuilderTest, MoveSemantics) {
  auto builder1 = std::make_unique<DisplayListBuilder>();
  builder1->Fill(0xFF0000FF);

  DisplayListBuilder builder2 = std::move(*builder1);

  DisplayList display_list = builder2.Build();
  EXPECT_EQ(display_list.GetContentItemsSize(), 1u);
}

TEST_F(DisplayListBuilderTest, DisplayListItemABI) {
  // Verify ABI invariants that cross-platform consumers depend on
  static_assert(sizeof(DisplayListItem) == 56,
                "DisplayListItem size must be 56 bytes");
  static_assert(offsetof(DisplayListItem, type) == 0,
                "type field must be at offset 0");
  static_assert(offsetof(DisplayListItem, payload.begin.id) == 4,
                "begin.id must be at offset 4");
  static_assert(offsetof(DisplayListItem, payload.fill.color) == 4,
                "fill.color must be at offset 4");
  static_assert(offsetof(DisplayListItem, payload.linear_gradient.angle) == 36,
                "linear_gradient.angle must be at offset 36");
  static_assert(std::is_standard_layout<DisplayListItem>::value,
                "DisplayListItem must be standard layout");
  static_assert(std::is_trivially_copyable<DisplayListItem>::value,
                "DisplayListItem must be trivially copyable");

  SUCCEED();
}

}  // namespace tasm
}  // namespace lynx
