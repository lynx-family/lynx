// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/logging.h"
#include "clay/gfx/style/color.h"
#include "clay/gfx/style/color_unittests_helper.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(ColorTest, CreateColorTest) {
  Color colorRed1(0xFFFF0000);
  Color colorRed2 = Color::ARGBColor(255, 255, 0, 0);
  Color colorRed3 = Color::RGBOColor(255, 0, 0, 1.0);

  EXPECT_EQ(colorRed1, colorRed2);
  EXPECT_EQ(colorRed2, colorRed3);
  EXPECT_EQ(colorRed3, 0xFFFF0000);
  EXPECT_EQ(colorRed3.Red(), 0xFF);

  EXPECT_EQ(colorRed1, Color::Lerp(colorRed2, colorRed3, 0.2));

  EXPECT_EQ(colorRed1, Color::AlphaBlend(colorRed2, colorRed3));
}

TEST(ColorTest, ParseColor) {
  Color to_parse;

  // Named color
  EXPECT_TRUE(Color::Parse(std::string("red"), &to_parse));
  EXPECT_EQ_RGBO(to_parse, 255, 0, 0, 1.0f);

  EXPECT_TRUE(Color::Parse(std::string("BLUE"), &to_parse));
  EXPECT_EQ_RGBO(to_parse, 0, 0, 255, 1.0f);

  // rgb(0, 255, 0)
  EXPECT_TRUE(Color::Parse(std::string("rgb(0, 255, 0)"), &to_parse));
  EXPECT_EQ_RGBO(to_parse, 0, 255, 0, 1.0f);

  // rgba(0, 0, 255, 0.5)
  EXPECT_TRUE(Color::Parse(std::string("rgba(0, 0, 255, 1.0)"), &to_parse));
  EXPECT_EQ_RGBO(to_parse, 0, 0, 255, 1.0f);

  // rgba(50%, 50%, 50%, 0.5)
  EXPECT_TRUE(Color::Parse(std::string("rgba(50%, 50%, 50%, 1.0)"), &to_parse));
  EXPECT_EQ_RGBO(to_parse, 128, 128, 128, 1.0f);

  // Wrong Case
  EXPECT_FALSE(Color::Parse(std::string("rad"), &to_parse));

  EXPECT_FALSE(Color::Parse(std::string("rgb(0,)"), &to_parse));
  EXPECT_FALSE(Color::Parse(std::string("rgb(0,0.0.0)"), &to_parse));

  // #0f38
  // TODO(Xietong): doesn't support yet.
  // EXPECT_TRUE(Color::Parse(std::string("#0f38"), to_parse));
  // EXPECT_EQ_RGBA(to_parse, 0u, (unsigned int)0xFF, (unsigned int)0x33,
  //                (unsigned int)0x88);

  // #0f3
  EXPECT_TRUE(Color::Parse(std::string("#0f3"), &to_parse));
  EXPECT_EQ_RGB(to_parse, 0, (int)0xFF, (int)0x33);

  // #00ff33
  EXPECT_TRUE(Color::Parse(std::string("#00ff33"), &to_parse));
  EXPECT_EQ_RGB(to_parse, 0, (int)0xFF, (int)0x33);

  // #00ff3388
  EXPECT_TRUE(Color::Parse(std::string("#00ff3388"), &to_parse));
  EXPECT_EQ_RGBA(to_parse, 0, (int)0xFF, (int)0x33, (int)0x88);
}

TEST(ColorTest, CompareColor) {
  const auto color1 = Color::ARGBColor(0x7F, 0x32, 0x41, 0xFF);
  const auto color2 = Color::RGBOColor(0x32, 0x41, 0xFF, 0.499f);
  const auto color3 = Color::RGBOColor(0x32, 0x41, 0xFF, 0.5f);

  EXPECT_EQ(color1, color2);
  EXPECT_EQ(color2, color3);
}

TEST(ColorTest, LerpColor) {
  const Color red = Color(0xFFFF0000u);
  const Color green = Color(0xFF008000u);
  const Color yellow = Color(0xFFFFFF00u);
  const Color blue = Color(0xFF0000FFu);

  EXPECT_EQ(Color::Lerp(red, green, 0.0f), red);
  EXPECT_EQ(Color::Lerp(red, green, 0.5f), 0xFF7F4000u);
  EXPECT_EQ(Color::Lerp(red, green, 1.0f), green);

  EXPECT_EQ(Color::Lerp(yellow, blue, 0.0f), yellow);
  EXPECT_EQ(Color::Lerp(yellow, blue, 0.5f), 0xFF7F7F7Fu);
  EXPECT_EQ(Color::Lerp(yellow, blue, 1.0f), blue);
}

}  // namespace clay
