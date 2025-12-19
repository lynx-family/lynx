// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/layout_info.h"

#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class LayoutInfoForDrawTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

TEST_F(LayoutInfoForDrawTest, BorderPaddingContentBoxesAndRadii) {
  LayoutInfoForDraw info;

  // Border box position and size
  info.layout_result.offset_ = starlight::FloatPoint(10.0f, 20.0f);
  info.layout_result.size_.width_ = 100.0f;
  info.layout_result.size_.height_ = 50.0f;

  // Border widths
  info.layout_result.border_[starlight::Direction::kLeft] = 1.0f;
  info.layout_result.border_[starlight::Direction::kTop] = 2.0f;
  info.layout_result.border_[starlight::Direction::kRight] = 3.0f;
  info.layout_result.border_[starlight::Direction::kBottom] = 4.0f;

  // Padding widths
  info.layout_result.padding_[starlight::Direction::kLeft] = 5.0f;
  info.layout_result.padding_[starlight::Direction::kTop] = 6.0f;
  info.layout_result.padding_[starlight::Direction::kRight] = 7.0f;
  info.layout_result.padding_[starlight::Direction::kBottom] = 8.0f;

  // Border radius
  BorderRadiusInfo radii;
  radii.x_top_left = 9.0f;
  radii.y_top_left = 10.0f;
  radii.x_top_right = 11.0f;
  radii.y_top_right = 12.0f;
  radii.x_bottom_right = 13.0f;
  radii.y_bottom_right = 14.0f;
  radii.x_bottom_left = 15.0f;
  radii.y_bottom_left = 16.0f;
  info.border_radius_info = radii;

  // Border box
  auto border_rect = info.GenerateBorderRectangle();
  EXPECT_FLOAT_EQ(border_rect.GetX(), 10.0f);
  EXPECT_FLOAT_EQ(border_rect.GetY(), 20.0f);
  EXPECT_FLOAT_EQ(border_rect.GetWidth(), 100.0f);
  EXPECT_FLOAT_EQ(border_rect.GetHeight(), 50.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusXTopLeft(), 9.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusYTopLeft(), 10.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusXTopRight(), 11.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusYTopRight(), 12.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusXBottomRight(), 13.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusYBottomRight(), 14.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusXBottomLeft(), 15.0f);
  EXPECT_FLOAT_EQ(border_rect.GetRadiusYBottomLeft(), 16.0f);

  // Padding box
  auto padding_rect = info.GeneratePaddingRectangle();
  EXPECT_FLOAT_EQ(padding_rect.GetX(), 10.0f + 1.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetY(), 20.0f + 2.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetWidth(), 100.0f - 1.0f - 3.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetHeight(), 50.0f - 2.0f - 4.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusXTopLeft(), 9.0f - 1.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusYTopLeft(), 10.0f - 2.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusXTopRight(), 11.0f - 3.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusYTopRight(), 12.0f - 2.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusXBottomRight(), 13.0f - 3.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusYBottomRight(), 14.0f - 4.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusXBottomLeft(), 15.0f - 1.0f);
  EXPECT_FLOAT_EQ(padding_rect.GetRadiusYBottomLeft(), 16.0f - 4.0f);

  // Content box
  auto content_rect = info.GenerateContentRectangle();
  EXPECT_FLOAT_EQ(content_rect.GetX(), 10.0f + 1.0f + 5.0f);
  EXPECT_FLOAT_EQ(content_rect.GetY(), 20.0f + 2.0f + 6.0f);
  EXPECT_FLOAT_EQ(content_rect.GetWidth(),
                  (100.0f - 1.0f - 3.0f) - 5.0f - 7.0f);
  EXPECT_FLOAT_EQ(content_rect.GetHeight(),
                  (50.0f - 2.0f - 4.0f) - 6.0f - 8.0f);
  EXPECT_FLOAT_EQ(content_rect.GetRadiusXTopLeft(),
                  std::max(9.0f - 1.0f - 5.0f, 0.0f));
  EXPECT_FLOAT_EQ(content_rect.GetRadiusYTopLeft(),
                  std::max(10.0f - 2.0f - 6.0f, 0.0f));
  EXPECT_FLOAT_EQ(content_rect.GetRadiusXTopRight(),
                  std::max(11.0f - 3.0f - 7.0f, 0.0f));
  EXPECT_FLOAT_EQ(content_rect.GetRadiusYTopRight(),
                  std::max(12.0f - 2.0f - 6.0f, 0.0f));
  EXPECT_FLOAT_EQ(content_rect.GetRadiusXBottomRight(),
                  std::max(13.0f - 3.0f - 7.0f, 0.0f));
  EXPECT_FLOAT_EQ(content_rect.GetRadiusYBottomRight(),
                  std::max(14.0f - 4.0f - 8.0f, 0.0f));
  EXPECT_FLOAT_EQ(content_rect.GetRadiusXBottomLeft(),
                  std::max(15.0f - 1.0f - 5.0f, 0.0f));
  EXPECT_FLOAT_EQ(content_rect.GetRadiusYBottomLeft(),
                  std::max(16.0f - 4.0f - 8.0f, 0.0f));
}

TEST_F(LayoutInfoForDrawTest, RadiiClampToZeroWhenNegative) {
  LayoutInfoForDraw info;
  info.layout_result.offset_ = starlight::FloatPoint(0.0f, 0.0f);
  info.layout_result.size_.width_ = 10.0f;
  info.layout_result.size_.height_ = 10.0f;
  info.layout_result.border_[starlight::Direction::kLeft] = 4.0f;
  info.layout_result.padding_[starlight::Direction::kLeft] = 3.0f;  // sum = 7

  BorderRadiusInfo radii;
  radii.x_top_left = 5.0f;  // smaller than border+padding => clamp to 0
  info.border_radius_info = radii;

  auto content_rect = info.GenerateContentRectangle();
  EXPECT_FLOAT_EQ(content_rect.GetRadiusXTopLeft(), 0.0f);
}

TEST_F(LayoutInfoForDrawTest, NoBorderRadiusInfo) {
  LayoutInfoForDraw info;
  info.layout_result.offset_ = starlight::FloatPoint(10.0f, 20.0f);
  info.layout_result.size_.width_ = 100.0f;
  info.layout_result.size_.height_ = 50.0f;
  // Set some padding/border just in case logic depends on it, though without
  // radii it only affects x/y/w/h calculation logic, not radii.
  info.layout_result.border_[starlight::Direction::kLeft] = 1.0f;
  info.layout_result.border_[starlight::Direction::kTop] = 2.0f;
  info.layout_result.border_[starlight::Direction::kRight] = 3.0f;
  info.layout_result.border_[starlight::Direction::kBottom] = 4.0f;

  info.layout_result.padding_[starlight::Direction::kLeft] = 5.0f;
  info.layout_result.padding_[starlight::Direction::kTop] = 6.0f;
  info.layout_result.padding_[starlight::Direction::kRight] = 7.0f;
  info.layout_result.padding_[starlight::Direction::kBottom] = 8.0f;

  // No border_radius_info set

  auto content_rect = info.GenerateContentRectangle();
  EXPECT_FALSE(content_rect.HasRadius());
  // Verify content rect dimensions are correct (100 - 1 - 3 - 5 - 7 = 84 width)
  EXPECT_FLOAT_EQ(content_rect.GetWidth(), 84.0f);

  auto padding_rect = info.GeneratePaddingRectangle();
  EXPECT_FALSE(padding_rect.HasRadius());
  // Verify padding rect dimensions (100 - 1 - 3 = 96 width)
  EXPECT_FLOAT_EQ(padding_rect.GetWidth(), 96.0f);

  auto border_rect = info.GenerateBorderRectangle();
  EXPECT_FALSE(border_rect.HasRadius());
  EXPECT_FLOAT_EQ(border_rect.GetWidth(), 100.0f);
}

}  // namespace tasm
}  // namespace lynx
