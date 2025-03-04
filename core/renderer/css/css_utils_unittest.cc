// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_utils.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

struct RadialGradientRadiusTestCase {
  RadialGradientShapeType shape;
  RadialGradientSizeType shape_size;
  float cx;
  float cy;
  float sx;
  float sy;

  std::pair<float, float> expected;
};

class RadialGradientRadiusTest
    : public ::testing::TestWithParam<RadialGradientRadiusTestCase> {};

TEST_P(RadialGradientRadiusTest, Radius) {
  auto param = GetParam();
  auto radius = GetRadialGradientRadius(param.shape, param.shape_size, param.cx,
                                        param.cy, param.sx, param.sy);
  EXPECT_EQ(static_cast<int>(radius.first), param.expected.first);
  EXPECT_EQ(static_cast<int>(radius.second), param.expected.second);
}

static const RadialGradientRadiusTestCase data[] = {
    {RadialGradientShapeType::kEllipse,
     RadialGradientSizeType::kClosestSide,
     10,
     0,
     200,
     100,
     {10, 0}},

    {RadialGradientShapeType::kEllipse,
     RadialGradientSizeType::kClosestCorner,
     0,
     0,
     200,
     100,
     {0, 0}},

    {RadialGradientShapeType::kEllipse,
     RadialGradientSizeType::kFarthestCorner,
     100,
     50,
     200,
     100,
     {141, 70}},

    {RadialGradientShapeType::kEllipse,
     RadialGradientSizeType::kFarthestCorner,
     0,
     0,
     200,
     100,
     {282, 141}},

    {RadialGradientShapeType::kEllipse,
     RadialGradientSizeType::kFarthestSide,
     0,
     0,
     200,
     100,
     {200, 100}},

    {RadialGradientShapeType::kCircle,
     RadialGradientSizeType::kFarthestCorner,
     100,
     50,
     200,
     100,
     {111, 111}},

    {RadialGradientShapeType::kCircle,
     RadialGradientSizeType::kFarthestSide,
     0,
     0,
     200,
     100,
     {200, 200}},

    {RadialGradientShapeType::kCircle,
     RadialGradientSizeType::kClosestSide,
     0,
     0,
     200,
     100,
     {0, 0}},

    {RadialGradientShapeType::kCircle,
     RadialGradientSizeType::kFarthestCorner,
     0,
     0,
     200,
     100,
     {223, 223}},

    {RadialGradientShapeType::kCircle,
     RadialGradientSizeType::kClosestCorner,
     0,
     0,
     200,
     100,
     {0, 0}},
};

INSTANTIATE_TEST_SUITE_P(Radius, RadialGradientRadiusTest,
                         ::testing::ValuesIn(data));

TEST(CSSUtils, ParseStyleDeclarationList) {
  const char* input = "background : url('xxx;);;;border:1px solid red ";
  std::unordered_map<std::string, std::string> values;
  auto ret = ParseStyleDeclarationList(
      input, static_cast<uint32_t>(strlen(input)),
      [&values](const char* key_start, uint32_t key_len,
                const char* value_start, uint32_t value_len) {
        auto key = std::string(key_start, key_start + key_len);
        auto value = std::string(value_start, value_start + value_len);
        values.insert(std::make_pair(key, value));
      });

  EXPECT_TRUE(ret);

  EXPECT_TRUE(values["background"] == "url('xxx;)");
  EXPECT_TRUE(values["border"] == "1px solid red ");

  const char* input2 = "background ; url('xxx;');;;border:1px solid red ";

  std::unordered_map<std::string, std::string> values2;
  ret = ParseStyleDeclarationList(
      input2, static_cast<uint32_t>(strlen(input2)),
      [&values2](const char* key_start, uint32_t key_len,
                 const char* value_start, uint32_t value_len) {
        auto key = std::string(key_start, key_start + key_len);
        auto value = std::string(value_start, value_start + value_len);
        values2.insert(std::make_pair(key, value));
      });

  EXPECT_TRUE(ret);
  EXPECT_TRUE(values2["border"] == "1px solid red ");

  const char* input3 = "height : 3px ; width:2px;border:1px solid red";

  std::unordered_map<std::string, std::string> values3;
  ret = ParseStyleDeclarationList(
      input3, static_cast<uint32_t>(strlen(input3)),
      [&values3](const char* key_start, uint32_t key_len,
                 const char* value_start, uint32_t value_len) {
        auto key = std::string(key_start, key_start + key_len);
        auto value = std::string(value_start, value_start + value_len);
        values3.insert(std::make_pair(key, value));
      });

  EXPECT_TRUE(ret);
  EXPECT_TRUE(values3["border"] == "1px solid red");
  EXPECT_TRUE(values3["width"] == "2px");
  EXPECT_TRUE(values3["height"] == "3px ");

  const char* input4 = "background;url('xxx;');width:1px";
  std::unordered_map<std::string, std::string> values4;
  ret = ParseStyleDeclarationList(
      input4, static_cast<uint32_t>(strlen(input4)),
      [&values4](const char* key_start, uint32_t key_len,
                 const char* value_start, uint32_t value_len) {
        auto key = std::string(key_start, key_start + key_len);
        auto value = std::string(value_start, value_start + value_len);
        values4.insert(std::make_pair(key, value));
      });

  EXPECT_TRUE(ret);
  EXPECT_TRUE(values4["width"] == "1px");

  const char* input5 = "background : url('xxx;';;;border:1px solid red ";

  std::unordered_map<std::string, std::string> values5;
  ret = ParseStyleDeclarationList(
      input5, static_cast<uint32_t>(strlen(input5)),
      [&values5](const char* key_start, uint32_t key_len,
                 const char* value_start, uint32_t value_len) {
        auto key = std::string(key_start, key_start + key_len);
        auto value = std::string(value_start, value_start + value_len);
        values5.insert(std::make_pair(key, value));
      });

  EXPECT_TRUE(!ret);

  const char* input6 = "background : 'xxxxx;;;border:1px solid red ";

  std::unordered_map<std::string, std::string> values6;
  ret = ParseStyleDeclarationList(
      input6, static_cast<uint32_t>(strlen(input6)),
      [&values6](const char* key_start, uint32_t key_len,
                 const char* value_start, uint32_t value_len) {
        auto key = std::string(key_start, key_start + key_len);
        auto value = std::string(value_start, value_start + value_len);
        values6.insert(std::make_pair(key, value));
      });

  EXPECT_TRUE(!ret);
}

TEST(CSSUtils, SplitClasses) {
  const char* input0 = "dark blue";

  ClassList out0 = SplitClasses(input0, strlen(input0));

  EXPECT_TRUE(out0.size() == 2);
  EXPECT_TRUE(out0[0] == "dark");
  EXPECT_TRUE(out0[1] == "blue");

  const char* input1 = "   dark  blue  black   ";

  ClassList out1 = SplitClasses(input1, strlen(input1));

  EXPECT_TRUE(out1.size() == 3);
  EXPECT_TRUE(out1[0] == "dark");
  EXPECT_TRUE(out1[1] == "blue");
  EXPECT_TRUE(out1[2] == "black");

  const char* input2 = "   dark  ";
  ClassList out2 = SplitClasses(input2, strlen(input2));

  EXPECT_TRUE(out2.size() == 1);
  EXPECT_TRUE(out2[0] == "dark");

  const char* input3 = "dark ";
  ClassList out3 = SplitClasses(input3, strlen(input3));

  EXPECT_TRUE(out3.size() == 1);
  EXPECT_TRUE(out3[0] == "dark");
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
