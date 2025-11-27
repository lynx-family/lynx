// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/rounded_rectangle.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/include/geometry/rect.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class RoundedRectangleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rounded_rectangle_ = std::make_unique<RoundedRectangle>();
  }

  void TearDown() override { rounded_rectangle_.reset(); }

  std::unique_ptr<RoundedRectangle> rounded_rectangle_;
};

TEST_F(RoundedRectangleTest, RectAccess) {
  base::geometry::FloatRect rect({10.0f, 20.0f}, {30.0f, 40.0f});
  rounded_rectangle_->SetRect(rect);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().X(), 10.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().Y(), 20.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().Width(), 30.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().Height(), 40.0f);
}

TEST_F(RoundedRectangleTest, XYWidthHeightSetters) {
  base::geometry::FloatRect rect({0.0f, 0.0f}, {0.0f, 0.0f});
  rounded_rectangle_->SetRect(rect);
  rounded_rectangle_->SetX(5.0f);
  rounded_rectangle_->SetY(6.0f);
  rounded_rectangle_->SetWidth(7.0f);
  rounded_rectangle_->SetHeight(8.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().X(), 5.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().Y(), 6.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().Width(), 7.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRect().Height(), 8.0f);
}

TEST_F(RoundedRectangleTest, RadiiHasRadius) {
  rounded_rectangle_->SetRadiusXTopLeft(0.0f);
  rounded_rectangle_->SetRadiusXTopRight(0.0f);
  rounded_rectangle_->SetRadiusXBottomRight(0.0f);
  rounded_rectangle_->SetRadiusXBottomLeft(0.0f);
  rounded_rectangle_->SetRadiusYTopLeft(0.0f);
  rounded_rectangle_->SetRadiusYTopRight(0.0f);
  rounded_rectangle_->SetRadiusYBottomRight(0.0f);
  rounded_rectangle_->SetRadiusYBottomLeft(0.0f);
  EXPECT_FALSE(rounded_rectangle_->HasRadius());
  rounded_rectangle_->SetRadiusXTopLeft(2.0f);
  EXPECT_TRUE(rounded_rectangle_->HasRadius());
  rounded_rectangle_->SetRadiusXTopLeft(0.0f);
  EXPECT_FALSE(rounded_rectangle_->HasRadius());
}

TEST_F(RoundedRectangleTest, TestRadiusSetterAndGetter) {
  rounded_rectangle_->SetRadiusXTopLeft(2.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusXTopLeft(), 2.0f);
  rounded_rectangle_->SetRadiusXTopRight(3.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusXTopRight(), 3.0f);
  rounded_rectangle_->SetRadiusXBottomRight(4.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusXBottomRight(), 4.0f);
  rounded_rectangle_->SetRadiusXBottomLeft(5.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusXBottomLeft(), 5.0f);
  rounded_rectangle_->SetRadiusYTopLeft(6.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusYTopLeft(), 6.0f);
  rounded_rectangle_->SetRadiusYTopRight(7.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusYTopRight(), 7.0f);
  rounded_rectangle_->SetRadiusYBottomRight(8.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusYBottomRight(), 8.0f);
  rounded_rectangle_->SetRadiusYBottomLeft(9.0f);
  EXPECT_FLOAT_EQ(rounded_rectangle_->GetRadiusYBottomLeft(), 9.0f);
}

}  // namespace tasm
}  // namespace lynx
