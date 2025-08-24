// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include <cmath>
#include <memory>

#include "core/animation/basic_animation/basic_animatable_values/color_property_value.h"
#include "core/animation/basic_animation/basic_animatable_values/float_property_value.h"
#include "core/animation/basic_animation/basic_animatable_values/int_property_value.h"
#include "core/animation/basic_animation/property_value.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace testing {

class BasicAnimatableValueTest : public ::testing::Test {
 public:
  BasicAnimatableValueTest() {}
  ~BasicAnimatableValueTest() override {}
};

TEST_F(BasicAnimatableValueTest, PropertyValueTest1) {
  //  cover color value
  auto color_value_1 = std::make_unique<animation::basic::ColorPropertyValue>(
      static_cast<uint32_t>(0x0));
  color_value_1->SetColorValue(static_cast<uint32_t>(4294901760));
  EXPECT_EQ(uint32_t(4294901760), color_value_1->GetColorValue());
  color_value_1->SetColorSRGBSpace();
  color_value_1->SetColorLinearSpace();
  color_value_1->Interpolate(
      0.5, std::make_unique<animation::basic::ColorPropertyValue>(
               static_cast<uint32_t>(0x0)));
  EXPECT_EQ(size_t(animation::basic::BasicPropertyValueType::Color),
            color_value_1->GetType());

  // cover int value
  auto int_value_1 = std::make_unique<animation::basic::IntPropertyValue>(0);
  auto int_value_2 = std::make_unique<animation::basic::IntPropertyValue>(0);
  int_value_1->SetIntValue(1);
  int_value_2->SetIntValue(0);
  EXPECT_EQ(int(1), int_value_1->GetIntValue());
  EXPECT_EQ(int(0), int_value_2->GetIntValue());
  int_value_1->Interpolate(
      0.5, std::make_unique<animation::basic::IntPropertyValue>(0));
  int_value_2->Interpolate(
      0.5, std::make_unique<animation::basic::IntPropertyValue>(1));
  EXPECT_EQ(size_t(animation::basic::BasicPropertyValueType::Int),
            int_value_1->GetType());

  // cover float value
  auto float_value_1 =
      std::make_unique<animation::basic::FloatPropertyValue>(0.0);
  float_value_1->SetFloatValue(1.0);
  EXPECT_EQ(float(1.0), float_value_1->GetFloatValue());
  float_value_1->Interpolate(
      0.5, std::make_unique<animation::basic::FloatPropertyValue>(0.0));
  EXPECT_EQ(size_t(animation::basic::BasicPropertyValueType::Float),
            float_value_1->GetType());
}

}  // namespace testing
}  // namespace lynx
