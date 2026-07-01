// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list.h"

#include <cstring>
#include <memory>
#include <utility>

#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/display_list_reader.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

namespace {

template <typename T>
T ReadField(const uint8_t* data, size_t offset) {
  T value;
  std::memcpy(&value, data + offset, sizeof(T));
  return value;
}

size_t SerializedItemOffset(size_t index) {
  return kDisplayListItemBufferHeaderSize +
         index * kSerializedDisplayListItemSize;
}

}  // namespace

class DisplayListTest : public ::testing::Test {
 protected:
  void SetUp() override { display_list_ = std::make_unique<DisplayList>(); }

  void TearDown() override { display_list_.reset(); }

  std::unique_ptr<DisplayList> display_list_;
};

TEST_F(DisplayListTest, EmptyDisplayList) {
  EXPECT_EQ(display_list_->GetContentItemsSize(), 0u);
  EXPECT_EQ(display_list_->GetContentDataSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertiesSize(), 0u);
}

TEST_F(DisplayListTest, AddSingleItem) {
  DisplayListItem item;
  item.type = DisplayListOpType::kFill;
  item.payload.fill.color = 0xFF0000FF;
  item.payload.fill.clip_index = -1;
  display_list_->AppendItem(item);

  EXPECT_EQ(display_list_->GetContentItemsSize(), 1u);
  EXPECT_EQ(display_list_->GetContentDataSize(), 0u);

  DisplayListReader reader(*display_list_);
  EXPECT_TRUE(reader.HasNext());
  const auto& read_item = reader.Next();
  EXPECT_EQ(read_item.type, DisplayListOpType::kFill);
  EXPECT_EQ(read_item.payload.fill.color, 0xFF0000FF);
  EXPECT_EQ(read_item.payload.fill.clip_index, -1);
  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListTest, AddMultipleItems) {
  DisplayListItem item1;
  item1.type = DisplayListOpType::kBegin;
  item1.payload.begin.id = 1;
  item1.payload.begin.type = static_cast<int32_t>(PlatformRendererType::kView);
  item1.payload.begin.x = 10.0f;
  item1.payload.begin.y = 20.0f;
  item1.payload.begin.w = 100.0f;
  item1.payload.begin.h = 200.0f;
  display_list_->AppendItem(item1);

  DisplayListItem item2;
  item2.type = DisplayListOpType::kFill;
  item2.payload.fill.color = 0xFF00FF00;
  item2.payload.fill.clip_index = 0;
  display_list_->AppendItem(item2);

  DisplayListItem item3;
  item3.type = DisplayListOpType::kEnd;
  display_list_->AppendItem(item3);

  EXPECT_EQ(display_list_->GetContentItemsSize(), 3u);

  DisplayListReader reader(*display_list_);
  EXPECT_TRUE(reader.HasNext());
  const auto& read1 = reader.Next();
  EXPECT_EQ(read1.type, DisplayListOpType::kBegin);
  EXPECT_EQ(read1.payload.begin.id, 1);
  EXPECT_EQ(read1.payload.begin.type,
            static_cast<int32_t>(PlatformRendererType::kView));
  EXPECT_FLOAT_EQ(read1.payload.begin.x, 10.0f);
  EXPECT_FLOAT_EQ(read1.payload.begin.y, 20.0f);
  EXPECT_FLOAT_EQ(read1.payload.begin.w, 100.0f);
  EXPECT_FLOAT_EQ(read1.payload.begin.h, 200.0f);

  EXPECT_TRUE(reader.HasNext());
  const auto& read2 = reader.Next();
  EXPECT_EQ(read2.type, DisplayListOpType::kFill);
  EXPECT_EQ(read2.payload.fill.color, 0xFF00FF00);
  EXPECT_EQ(read2.payload.fill.clip_index, 0);

  EXPECT_TRUE(reader.HasNext());
  const auto& read3 = reader.Next();
  EXPECT_EQ(read3.type, DisplayListOpType::kEnd);

  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListTest, ClearDisplayList) {
  DisplayListItem item;
  item.type = DisplayListOpType::kFill;
  item.payload.fill.color = 0xFF0000FF;
  item.payload.fill.clip_index = -1;
  display_list_->AppendItem(item);

  EXPECT_EQ(display_list_->GetContentItemsSize(), 1u);

  display_list_->Clear();

  EXPECT_EQ(display_list_->GetContentItemsSize(), 0u);
  EXPECT_EQ(display_list_->GetContentDataSize(), 0u);
}

TEST_F(DisplayListTest, AddLinearGradient) {
  base::Vector<uint32_t> colors;
  colors.push_back(0xFFFF0000);
  colors.push_back(0xFF00FF00);
  colors.push_back(0xFF0000FF);

  base::Vector<float> stops;
  stops.push_back(0.0f);
  stops.push_back(0.5f);
  stops.push_back(1.0f);

  display_list_->AddLinearGradient(45.0f, colors, stops, 0, 1, 1, 1);

  EXPECT_EQ(display_list_->GetContentItemsSize(), 1u);
  EXPECT_EQ(display_list_->GetContentDataSize(),
            colors.size() * sizeof(uint32_t) + stops.size() * sizeof(float));

  DisplayListReader reader(*display_list_);
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
}

TEST_F(DisplayListTest, AddLinearGradientEmpty) {
  base::Vector<uint32_t> colors;
  base::Vector<float> stops;

  display_list_->AddLinearGradient(0.0f, colors, stops, -1, -1, 0, 0);

  EXPECT_EQ(display_list_->GetContentItemsSize(), 1u);
  EXPECT_EQ(display_list_->GetContentDataSize(), 0u);

  DisplayListReader reader(*display_list_);
  EXPECT_TRUE(reader.HasNext());
  const auto& item = reader.Next();
  EXPECT_EQ(item.type, DisplayListOpType::kLinearGradient);
  EXPECT_EQ(item.payload.linear_gradient.color_count, 0u);
  EXPECT_EQ(item.payload.linear_gradient.stop_count, 0u);
  EXPECT_EQ(reader.Colors(item), nullptr);
  EXPECT_EQ(reader.Stops(item), nullptr);
}

TEST_F(DisplayListTest, MoveSemantics) {
  DisplayListItem item;
  item.type = DisplayListOpType::kFill;
  item.payload.fill.color = 0xFF0000FF;
  item.payload.fill.clip_index = -1;
  display_list_->AppendItem(item);

  DisplayList moved_list = std::move(*display_list_);

  EXPECT_EQ(moved_list.GetContentItemsSize(), 1u);
  EXPECT_EQ(display_list_->GetContentItemsSize(), 0u);
}

TEST_F(DisplayListTest, SubtreePropertySeparation) {
  SubtreeProperty prop;
  prop.type = DisplayListSubtreePropertyOpType::kTransform;
  for (int i = 0; i < 16; ++i) {
    prop.data.transform[i] = static_cast<float>(i);
  }
  display_list_->AddSubtreeProperty(prop);

  EXPECT_EQ(display_list_->GetSubtreePropertiesSize(), 1u);
  EXPECT_EQ(display_list_->GetContentItemsSize(), 0u);

  const SubtreeProperty* data = display_list_->GetSubtreePropertiesData();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data[0].type, DisplayListSubtreePropertyOpType::kTransform);
  for (int i = 0; i < 16; ++i) {
    EXPECT_FLOAT_EQ(data[0].data.transform[i], static_cast<float>(i));
  }
}

TEST_F(DisplayListTest, ClearSubtreeProperties) {
  SubtreeProperty prop;
  prop.type = DisplayListSubtreePropertyOpType::kOpacity;
  prop.data.opacity = 0.5f;
  display_list_->AddSubtreeProperty(prop);

  EXPECT_EQ(display_list_->GetSubtreePropertiesSize(), 1u);

  display_list_->ClearSubtreeProperties();

  EXPECT_EQ(display_list_->GetSubtreePropertiesSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertiesData(), nullptr);
}

TEST_F(DisplayListTest, SerializedContentItemsBuffer) {
  DisplayListItem begin{};
  begin.type = DisplayListOpType::kBegin;
  begin.payload.begin.id = 42;
  begin.payload.begin.type = static_cast<int32_t>(PlatformRendererType::kView);
  begin.payload.begin.x = 10.0f;
  begin.payload.begin.y = 20.0f;
  begin.payload.begin.w = 100.0f;
  begin.payload.begin.h = 200.0f;
  display_list_->AppendItem(begin);

  DisplayListItem fill{};
  fill.type = DisplayListOpType::kFill;
  fill.payload.fill.color = 0xFF00FF00;
  fill.payload.fill.clip_index = 0;
  display_list_->AppendItem(fill);

  DisplayListItem draw_view{};
  draw_view.type = DisplayListOpType::kDrawView;
  draw_view.payload.draw_view.view_id = 99;
  display_list_->AppendItem(draw_view);

  DisplayListItem text{};
  text.type = DisplayListOpType::kText;
  text.payload.text.text_id = 101;
  text.payload.text.box_index = 1;
  display_list_->AppendItem(text);

  DisplayListItem image{};
  image.type = DisplayListOpType::kImage;
  image.payload.image.image_id = 202;
  image.payload.image.box_index = 2;
  display_list_->AppendItem(image);

  DisplayListItem border{};
  border.type = DisplayListOpType::kBorder;
  border.payload.border.out_index = 3;
  border.payload.border.inner_index = 4;
  border.payload.border.colors[0] = 0xFFFF0000;
  border.payload.border.colors[1] = 0xFF00FF00;
  border.payload.border.colors[2] = 0xFF0000FF;
  border.payload.border.colors[3] = 0xFFFFFF00;
  border.payload.border.styles[0] = 1;
  border.payload.border.styles[1] = 2;
  border.payload.border.styles[2] = 3;
  border.payload.border.styles[3] = 4;
  display_list_->AppendItem(border);

  DisplayListItem clip{};
  clip.type = DisplayListOpType::kClipRect;
  clip.payload.clip_rect.x = 1.0f;
  clip.payload.clip_rect.y = 2.0f;
  clip.payload.clip_rect.w = 3.0f;
  clip.payload.clip_rect.h = 4.0f;
  for (int i = 0; i < 8; ++i) {
    clip.payload.clip_rect.radii[i] = static_cast<float>(i + 1);
  }
  clip.payload.clip_rect.has_radii = 1;
  display_list_->AppendItem(clip);

  DisplayListItem record{};
  record.type = DisplayListOpType::kRecordBox;
  record.payload.record_box.x = 5.0f;
  record.payload.record_box.y = 6.0f;
  record.payload.record_box.w = 7.0f;
  record.payload.record_box.h = 8.0f;
  for (int i = 0; i < 8; ++i) {
    record.payload.record_box.radii[i] = static_cast<float>(i + 9);
  }
  record.payload.record_box.has_radii = 1;
  display_list_->AppendItem(record);

  base::Vector<uint32_t> colors;
  colors.push_back(0xFF111111);
  colors.push_back(0xFF222222);
  base::Vector<float> stops;
  stops.push_back(0.25f);
  stops.push_back(0.75f);
  display_list_->AddLinearGradient(45.0f, colors, stops, 8, 9, 1, 0);

  DisplayListItem box_shadow{};
  box_shadow.type = DisplayListOpType::kBoxShadow;
  box_shadow.payload.box_shadow.shadow_box_index = 10;
  box_shadow.payload.box_shadow.clip_box_index = 11;
  box_shadow.payload.box_shadow.color = 0x80000000;
  box_shadow.payload.box_shadow.blur_radius = 12.0f;
  box_shadow.payload.box_shadow.clip_mode = 1;
  display_list_->AppendItem(box_shadow);

  DisplayListItem end{};
  end.type = DisplayListOpType::kEnd;
  display_list_->AppendItem(end);

  const uint8_t* data = display_list_->GetSerializedContentItemsData();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(display_list_->GetSerializedContentItemsSize(),
            static_cast<size_t>(kDisplayListItemBufferHeaderSize +
                                11 * kSerializedDisplayListItemSize));
  EXPECT_EQ(ReadField<int32_t>(data, 0), kSerializedDisplayListItemSize);

  const size_t begin_offset = SerializedItemOffset(0);
  EXPECT_EQ(ReadField<int32_t>(data, begin_offset),
            static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(ReadField<int32_t>(data, begin_offset + 4), 42);
  EXPECT_FLOAT_EQ(ReadField<float>(data, begin_offset + 12), 10.0f);
  EXPECT_FLOAT_EQ(ReadField<float>(data, begin_offset + 24), 200.0f);

  const size_t fill_offset = SerializedItemOffset(1);
  EXPECT_EQ(ReadField<int32_t>(data, fill_offset),
            static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(ReadField<uint32_t>(data, fill_offset + 4), 0xFF00FF00u);
  EXPECT_EQ(ReadField<int32_t>(data, fill_offset + 8), 0);

  const size_t draw_view_offset = SerializedItemOffset(2);
  EXPECT_EQ(ReadField<int32_t>(data, draw_view_offset + 4), 99);

  const size_t text_offset = SerializedItemOffset(3);
  EXPECT_EQ(ReadField<int32_t>(data, text_offset + 4), 101);
  EXPECT_EQ(ReadField<int32_t>(data, text_offset + 8), 1);

  const size_t image_offset = SerializedItemOffset(4);
  EXPECT_EQ(ReadField<int32_t>(data, image_offset + 4), 202);
  EXPECT_EQ(ReadField<int32_t>(data, image_offset + 8), 2);

  const size_t border_offset = SerializedItemOffset(5);
  EXPECT_EQ(ReadField<int32_t>(data, border_offset + 4), 3);
  EXPECT_EQ(ReadField<int32_t>(data, border_offset + 8), 4);
  EXPECT_EQ(ReadField<uint32_t>(data, border_offset + 12), 0xFFFF0000u);
  EXPECT_EQ(ReadField<uint32_t>(data, border_offset + 24), 0xFFFFFF00u);
  EXPECT_EQ(ReadField<int32_t>(data, border_offset + 28), 1);
  EXPECT_EQ(ReadField<int32_t>(data, border_offset + 40), 4);

  const size_t clip_offset = SerializedItemOffset(6);
  EXPECT_FLOAT_EQ(ReadField<float>(data, clip_offset + 4), 1.0f);
  EXPECT_FLOAT_EQ(ReadField<float>(data, clip_offset + 16), 4.0f);
  EXPECT_FLOAT_EQ(ReadField<float>(data, clip_offset + 48), 8.0f);
  EXPECT_EQ(ReadField<uint32_t>(data, clip_offset + 52), 1u);

  const size_t record_offset = SerializedItemOffset(7);
  EXPECT_FLOAT_EQ(ReadField<float>(data, record_offset + 4), 5.0f);
  EXPECT_FLOAT_EQ(ReadField<float>(data, record_offset + 16), 8.0f);
  EXPECT_FLOAT_EQ(ReadField<float>(data, record_offset + 48), 16.0f);
  EXPECT_EQ(ReadField<uint32_t>(data, record_offset + 52), 1u);

  const size_t gradient_offset = SerializedItemOffset(8);
  EXPECT_EQ(ReadField<uint32_t>(data, gradient_offset + 4), 0u);
  EXPECT_EQ(ReadField<uint32_t>(data, gradient_offset + 8), 2u);
  EXPECT_EQ(ReadField<uint32_t>(data, gradient_offset + 12),
            2 * sizeof(uint32_t));
  EXPECT_EQ(ReadField<uint32_t>(data, gradient_offset + 16), 2u);
  EXPECT_EQ(ReadField<int32_t>(data, gradient_offset + 20), 8);
  EXPECT_EQ(ReadField<int32_t>(data, gradient_offset + 24), 9);
  EXPECT_EQ(ReadField<int32_t>(data, gradient_offset + 28), 1);
  EXPECT_EQ(ReadField<int32_t>(data, gradient_offset + 32), 0);
  EXPECT_FLOAT_EQ(ReadField<float>(data, gradient_offset + 36), 45.0f);

  const size_t box_shadow_offset = SerializedItemOffset(9);
  EXPECT_EQ(ReadField<int32_t>(data, box_shadow_offset + 4), 10);
  EXPECT_EQ(ReadField<int32_t>(data, box_shadow_offset + 8), 11);
  EXPECT_EQ(ReadField<uint32_t>(data, box_shadow_offset + 12), 0x80000000u);
  EXPECT_FLOAT_EQ(ReadField<float>(data, box_shadow_offset + 16), 12.0f);
  EXPECT_EQ(ReadField<int32_t>(data, box_shadow_offset + 20), 1);

  const size_t end_offset = SerializedItemOffset(10);
  EXPECT_EQ(ReadField<int32_t>(data, end_offset),
            static_cast<int32_t>(DisplayListOpType::kEnd));
}

TEST_F(DisplayListTest, DisplayListReaderRoundTrip) {
  // Build a display list with all op types
  DisplayListItem begin;
  begin.type = DisplayListOpType::kBegin;
  begin.payload.begin.id = 42;
  begin.payload.begin.type = static_cast<int32_t>(PlatformRendererType::kView);
  begin.payload.begin.x = 10.0f;
  begin.payload.begin.y = 20.0f;
  begin.payload.begin.w = 100.0f;
  begin.payload.begin.h = 200.0f;
  display_list_->AppendItem(begin);

  DisplayListItem fill;
  fill.type = DisplayListOpType::kFill;
  fill.payload.fill.color = 0xFF00FF00;
  fill.payload.fill.clip_index = 0;
  display_list_->AppendItem(fill);

  DisplayListItem draw_view;
  draw_view.type = DisplayListOpType::kDrawView;
  draw_view.payload.draw_view.view_id = 99;
  display_list_->AppendItem(draw_view);

  DisplayListItem text;
  text.type = DisplayListOpType::kText;
  text.payload.text.text_id = 101;
  text.payload.text.box_index = 1;
  display_list_->AppendItem(text);

  DisplayListItem image;
  image.type = DisplayListOpType::kImage;
  image.payload.image.image_id = 202;
  image.payload.image.box_index = 2;
  display_list_->AppendItem(image);

  DisplayListItem border;
  border.type = DisplayListOpType::kBorder;
  border.payload.border.out_index = 0;
  border.payload.border.inner_index = 1;
  border.payload.border.colors[0] = 0xFFFF0000;
  border.payload.border.colors[1] = 0xFF00FF00;
  border.payload.border.colors[2] = 0xFF0000FF;
  border.payload.border.colors[3] = 0xFFFFFF00;
  border.payload.border.styles[0] = 1;
  border.payload.border.styles[1] = 1;
  border.payload.border.styles[2] = 1;
  border.payload.border.styles[3] = 1;
  display_list_->AppendItem(border);

  DisplayListItem clip;
  clip.type = DisplayListOpType::kClipRect;
  clip.payload.clip_rect.x = 0.0f;
  clip.payload.clip_rect.y = 0.0f;
  clip.payload.clip_rect.w = 50.0f;
  clip.payload.clip_rect.h = 50.0f;
  clip.payload.clip_rect.has_radii = 1;
  clip.payload.clip_rect.radii[0] = 5.0f;
  clip.payload.clip_rect.radii[1] = 5.0f;
  clip.payload.clip_rect.radii[2] = 5.0f;
  clip.payload.clip_rect.radii[3] = 5.0f;
  clip.payload.clip_rect.radii[4] = 5.0f;
  clip.payload.clip_rect.radii[5] = 5.0f;
  clip.payload.clip_rect.radii[6] = 5.0f;
  clip.payload.clip_rect.radii[7] = 5.0f;
  display_list_->AppendItem(clip);

  DisplayListItem record;
  record.type = DisplayListOpType::kRecordBox;
  record.payload.record_box.x = 0.0f;
  record.payload.record_box.y = 0.0f;
  record.payload.record_box.w = 100.0f;
  record.payload.record_box.h = 100.0f;
  record.payload.record_box.has_radii = 0;
  display_list_->AppendItem(record);

  DisplayListItem box_shadow;
  box_shadow.type = DisplayListOpType::kBoxShadow;
  box_shadow.payload.box_shadow.shadow_box_index = 0;
  box_shadow.payload.box_shadow.clip_box_index = 1;
  box_shadow.payload.box_shadow.color = 0x80000000;
  box_shadow.payload.box_shadow.blur_radius = 10.0f;
  box_shadow.payload.box_shadow.clip_mode = 0;
  display_list_->AppendItem(box_shadow);

  DisplayListItem end;
  end.type = DisplayListOpType::kEnd;
  display_list_->AppendItem(end);

  // Verify with reader
  DisplayListReader reader(*display_list_);
  EXPECT_EQ(reader.Remaining(), 10u);

  const auto& r1 = reader.Next();
  EXPECT_EQ(r1.type, DisplayListOpType::kBegin);
  EXPECT_EQ(r1.payload.begin.id, 42);

  const auto& r2 = reader.Next();
  EXPECT_EQ(r2.type, DisplayListOpType::kFill);
  EXPECT_EQ(r2.payload.fill.color, 0xFF00FF00);

  const auto& r3 = reader.Next();
  EXPECT_EQ(r3.type, DisplayListOpType::kDrawView);
  EXPECT_EQ(r3.payload.draw_view.view_id, 99);

  const auto& r4 = reader.Next();
  EXPECT_EQ(r4.type, DisplayListOpType::kText);
  EXPECT_EQ(r4.payload.text.text_id, 101);
  EXPECT_EQ(r4.payload.text.box_index, 1);

  const auto& r5 = reader.Next();
  EXPECT_EQ(r5.type, DisplayListOpType::kImage);
  EXPECT_EQ(r5.payload.image.image_id, 202);
  EXPECT_EQ(r5.payload.image.box_index, 2);

  const auto& r6 = reader.Next();
  EXPECT_EQ(r6.type, DisplayListOpType::kBorder);
  EXPECT_EQ(r6.payload.border.out_index, 0);
  EXPECT_EQ(r6.payload.border.inner_index, 1);
  EXPECT_EQ(r6.payload.border.colors[0], 0xFFFF0000);
  EXPECT_EQ(r6.payload.border.styles[0], 1);

  const auto& r7 = reader.Next();
  EXPECT_EQ(r7.type, DisplayListOpType::kClipRect);
  EXPECT_EQ(r7.payload.clip_rect.has_radii, 1u);
  EXPECT_FLOAT_EQ(r7.payload.clip_rect.radii[0], 5.0f);

  const auto& r8 = reader.Next();
  EXPECT_EQ(r8.type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(r8.payload.record_box.has_radii, 0u);
  EXPECT_FLOAT_EQ(r8.payload.record_box.w, 100.0f);

  const auto& r9 = reader.Next();
  EXPECT_EQ(r9.type, DisplayListOpType::kBoxShadow);
  EXPECT_EQ(r9.payload.box_shadow.shadow_box_index, 0);
  EXPECT_EQ(r9.payload.box_shadow.clip_box_index, 1);
  EXPECT_EQ(r9.payload.box_shadow.color, 0x80000000);
  EXPECT_FLOAT_EQ(r9.payload.box_shadow.blur_radius, 10.0f);
  EXPECT_EQ(r9.payload.box_shadow.clip_mode, 0);

  const auto& r10 = reader.Next();
  EXPECT_EQ(r10.type, DisplayListOpType::kEnd);

  EXPECT_FALSE(reader.HasNext());
}

TEST_F(DisplayListTest, ReserveAndClear) {
  display_list_->Reserve(100);
  EXPECT_EQ(display_list_->GetContentItemsSize(), 0u);

  DisplayListItem item;
  item.type = DisplayListOpType::kFill;
  item.payload.fill.color = 0xFF0000FF;
  item.payload.fill.clip_index = -1;
  display_list_->AppendItem(item);

  EXPECT_EQ(display_list_->GetContentItemsSize(), 1u);

  display_list_->Clear();
  EXPECT_EQ(display_list_->GetContentItemsSize(), 0u);
  EXPECT_EQ(display_list_->GetSerializedContentItemsSize(), 0u);
  EXPECT_EQ(display_list_->GetContentDataSize(), 0u);
}

TEST_F(DisplayListTest, MarkRootNeedClipBounds) {
  EXPECT_FALSE(display_list_->RootNeedClipBounds());
  display_list_->MarkRootNeedClipBounds();
  EXPECT_TRUE(display_list_->RootNeedClipBounds());
}

TEST_F(DisplayListTest, RenderOffset) {
  DisplayList list(5.0f, 10.0f);
  const float* offset = list.GetRenderOffset();
  EXPECT_FLOAT_EQ(offset[0], 5.0f);
  EXPECT_FLOAT_EQ(offset[1], 10.0f);
}

}  // namespace tasm
}  // namespace lynx
