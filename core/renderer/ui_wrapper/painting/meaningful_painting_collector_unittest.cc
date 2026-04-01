// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/meaningful_painting_collector.h"

#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {
namespace {

class FakePlatformRenderer final : public PlatformRendererImpl {
 public:
  explicit FakePlatformRenderer(
      int32_t id, PlatformRendererType type = PlatformRendererType::kView)
      : PlatformRendererImpl(id, type, base::String()) {}

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override {
    display_list_ = std::move(display_list);
  }

  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&, bool) override {}
  void OnAddChild(PlatformRenderer*) override {}
  void OnRemoveFromParent() override {}
  void OnUpdateSubtreeProperties(const DisplayList&) override {}
};

RoundedRectangle MakeRect(float x, float y, float width, float height) {
  RoundedRectangle rect;
  rect.SetX(x);
  rect.SetY(y);
  rect.SetWidth(width);
  rect.SetHeight(height);
  return rect;
}

void ExpectRecord(const MeaningfulPaintingAreaRecord& record, int32_t sign,
                  PlatformRendererType type, int32_t x, int32_t y,
                  int32_t width, int32_t height) {
  EXPECT_EQ(record.sign, sign);
  EXPECT_EQ(record.type, type);
  EXPECT_EQ(record.x, x);
  EXPECT_EQ(record.y, y);
  EXPECT_EQ(record.width, width);
  EXPECT_EQ(record.height, height);
}

TEST(MeaningfulPaintingCollectorTest, CollectsFlattenedTextAndImageAreas) {
  auto root = fml::MakeRefCounted<FakePlatformRenderer>(
      kRootId, PlatformRendererType::kPage);

  DisplayListBuilder builder(0.f, 0.f);
  int32_t root_box_index = -1;
  int32_t nested_box_index = -1;
  builder.Begin(kRootId, 0.f, 0.f, 200.f, 200.f);
  builder.RecordBoxModel(MakeRect(1.f, 2.f, 30.f, 40.f), root_box_index);
  builder.DrawText(101, root_box_index);
  builder.Begin(2001, 5.f, 6.f, 20.f, 20.f);
  builder.RecordBoxModel(MakeRect(2.f, 3.f, 4.f, 5.f), nested_box_index);
  builder.DrawImage(102, nested_box_index);
  builder.End();
  builder.End();
  root->UpdateDisplayList(builder.Build());

  MeaningfulPaintingCollector collector(root);
  auto records = collector.Collect();

  ASSERT_EQ(records.size(), 2u);
  ExpectRecord(records[0], 101, PlatformRendererType::kText, 1, 2, 30, 40);
  ExpectRecord(records[1], 102, PlatformRendererType::kImage, 7, 9, 4, 5);
}

TEST(MeaningfulPaintingCollectorTest,
     CollectsChildRendererAreasUsingDrawViewHierarchy) {
  auto root = fml::MakeRefCounted<FakePlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  auto child = fml::MakeRefCounted<FakePlatformRenderer>(
      2002, PlatformRendererType::kView);
  root->AddChild(child);

  DisplayListBuilder parent_builder(0.f, 0.f);
  parent_builder.Begin(kRootId, 0.f, 0.f, 200.f, 200.f);
  parent_builder.Begin(3001, 7.f, 8.f, 30.f, 30.f);
  parent_builder.DrawView(2002);
  parent_builder.End();
  parent_builder.End();
  root->UpdateDisplayList(parent_builder.Build());

  DisplayListBuilder child_builder(50.f, 60.f);
  int32_t child_box_index = -1;
  child_builder.Begin(2002, 20.f, 30.f, 40.f, 50.f);
  child_builder.RecordBoxModel(MakeRect(1.f, 2.f, 3.f, 4.f), child_box_index);
  child_builder.DrawImage(103, child_box_index);
  child_builder.End();
  child->UpdateDisplayList(child_builder.Build());

  MeaningfulPaintingCollector collector(root);
  auto records = collector.Collect();
  ASSERT_EQ(records.size(), 1u);
  // The host view placement may use render_offset, but content geometry stays
  // aligned to the fragment frame recorded in OP_BEGIN.
  ExpectRecord(records[0], 103, PlatformRendererType::kImage, 28, 40, 3, 4);
}

TEST(MeaningfulPaintingCollectorTest, SerializesRecordsForJniTransport) {
  MeaningfulPaintingAreaRecord text_record;
  text_record.sign = 1;
  text_record.type = PlatformRendererType::kText;
  text_record.x = 2;
  text_record.y = 3;
  text_record.width = 4;
  text_record.height = 5;

  MeaningfulPaintingAreaRecord image_record;
  image_record.sign = 6;
  image_record.type = PlatformRendererType::kImage;
  image_record.x = 7;
  image_record.y = 8;
  image_record.width = 9;
  image_record.height = 10;

  base::Vector<MeaningfulPaintingAreaRecord> records;
  records.push_back(text_record);
  records.push_back(image_record);

  auto serialized = MeaningfulPaintingCollector::Serialize(records);
  ASSERT_EQ(serialized.size(),
            2u * kMeaningfulPaintingAreaSerializedFieldCount);
  EXPECT_EQ(serialized[0], 1);
  EXPECT_EQ(serialized[1], static_cast<int32_t>(PlatformRendererType::kText));
  EXPECT_EQ(serialized[2], 2);
  EXPECT_EQ(serialized[3], 3);
  EXPECT_EQ(serialized[4], 4);
  EXPECT_EQ(serialized[5], 5);
  EXPECT_EQ(serialized[6], 6);
  EXPECT_EQ(serialized[7], static_cast<int32_t>(PlatformRendererType::kImage));
  EXPECT_EQ(serialized[8], 7);
  EXPECT_EQ(serialized[9], 8);
  EXPECT_EQ(serialized[10], 9);
  EXPECT_EQ(serialized[11], 10);
}

}  // namespace
}  // namespace lynx::tasm
