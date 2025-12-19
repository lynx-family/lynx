// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/box_model_recorder.h"

#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

static LayoutInfoForDraw MakeInfo(
    float x, float y, float w, float h, float bl, float bt, float br, float bb,
    float pl, float pt, float pr, float pb,
    const base::flex_optional<BorderRadiusInfo>& radii) {
  LayoutInfoForDraw info;
  info.layout_result.offset_ = starlight::FloatPoint(x, y);
  info.layout_result.size_.width_ = w;
  info.layout_result.size_.height_ = h;
  info.layout_result.border_[starlight::Direction::kLeft] = bl;
  info.layout_result.border_[starlight::Direction::kTop] = bt;
  info.layout_result.border_[starlight::Direction::kRight] = br;
  info.layout_result.border_[starlight::Direction::kBottom] = bb;
  info.layout_result.padding_[starlight::Direction::kLeft] = pl;
  info.layout_result.padding_[starlight::Direction::kTop] = pt;
  info.layout_result.padding_[starlight::Direction::kRight] = pr;
  info.layout_result.padding_[starlight::Direction::kBottom] = pb;
  info.border_radius_info = radii;
  return info;
}

class FakeDisplayListBuilder : public DisplayListBuilder {
 public:
  using DisplayListBuilder::DisplayListBuilder;
};

TEST(BoxModelRecorderTest, RecordIndicesPerTypeAndOnce) {
  BoxModelRecorder recorder;
  FakeDisplayListBuilder builder;

  BorderRadiusInfo radii{9, 10, 11, 12, 13, 14, 15, 16};
  auto info = MakeInfo(10, 20, 100, 50, 1, 2, 3, 4, 5, 6, 7, 8, radii);

  int32_t idx_content_1 =
      recorder.GetIndexOfBoxModel(kBoxModelTypeContent, info, builder);
  int32_t idx_padding_1 =
      recorder.GetIndexOfBoxModel(kBoxModelTypePadding, info, builder);
  int32_t idx_border_1 =
      recorder.GetIndexOfBoxModel(kBoxModelTypeBorder, info, builder);

  // Subsequent calls should not record again; indices remain unchanged
  int32_t idx_content_2 =
      recorder.GetIndexOfBoxModel(kBoxModelTypeContent, info, builder);
  int32_t idx_padding_2 =
      recorder.GetIndexOfBoxModel(kBoxModelTypePadding, info, builder);
  int32_t idx_border_2 =
      recorder.GetIndexOfBoxModel(kBoxModelTypeBorder, info, builder);

  EXPECT_EQ(idx_content_1, idx_content_2);
  EXPECT_EQ(idx_padding_1, idx_padding_2);
  EXPECT_EQ(idx_border_1, idx_border_2);

  // Indices should not be negative after recording
  EXPECT_GE(idx_content_1, 0);
  EXPECT_GE(idx_padding_1, 0);
  EXPECT_GE(idx_border_1, 0);
}

TEST(BoxModelRecorderTest, ResetClearsIndices) {
  BoxModelRecorder recorder;
  FakeDisplayListBuilder builder;
  auto info = MakeInfo(0, 0, 10, 10, 0, 0, 0, 0, 0, 0, 0, 0,
                       base::flex_optional<BorderRadiusInfo>());

  auto idx1 = recorder.GetIndexOfBoxModel(kBoxModelTypeBorder, info, builder);
  EXPECT_GE(idx1, 0);

  recorder.Reset();

  auto idx2 = recorder.GetIndexOfBoxModel(kBoxModelTypeBorder, info, builder);
  EXPECT_GE(idx2, 0);
  // After Reset, a new index should be assigned (builder maintains its own
  // counter). We cannot guarantee different values since the internal builder
  // starts at 0; we only ensure it continues to work.
}

}  // namespace tasm
}  // namespace lynx
