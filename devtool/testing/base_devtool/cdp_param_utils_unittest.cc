// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/public/cdp_param_utils.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

TEST(CDPParamUtilsTest, ReadIntAcceptsSignedAndUnsignedValuesWithinIntRange) {
  const int values[] = {std::numeric_limits<int>::min(), -1, 0, 1,
                        std::numeric_limits<int>::max()};
  for (int expected : values) {
    int result = 42;
    EXPECT_TRUE(ReadIntParam(Json::Value(expected), result));
    EXPECT_EQ(result, expected);
    if (expected >= 0) {
      EXPECT_TRUE(
          ReadIntParam(Json::Value(static_cast<Json::UInt>(expected)), result));
      EXPECT_EQ(result, expected);
    }
  }
}

TEST(CDPParamUtilsTest, ReadIntRejectsOtherTypesWithoutChangingOutput) {
  const std::vector<Json::Value> values = {Json::Value(),
                                           Json::Value(true),
                                           Json::Value(false),
                                           Json::Value("1"),
                                           Json::Value(1.0),
                                           Json::Value(1.5),
                                           Json::Value(Json::arrayValue),
                                           Json::Value(Json::objectValue)};
  for (const auto& value : values) {
    SCOPED_TRACE(value.toStyledString());
    int result = 42;
    EXPECT_FALSE(ReadIntParam(value, result));
    EXPECT_EQ(result, 42);
  }
}

TEST(CDPParamUtilsTest, ReadIntRejectsOverflowWithoutChangingOutput) {
  const std::vector<Json::Value> values = {
      Json::Value(static_cast<Json::Int64>(std::numeric_limits<int>::min()) -
                  1),
      Json::Value(static_cast<Json::Int64>(std::numeric_limits<int>::max()) +
                  1),
      Json::Value(std::numeric_limits<Json::UInt64>::max()),
      Json::Value(std::numeric_limits<Json::Int64>::min())};
  for (const auto& value : values) {
    int result = 42;
    EXPECT_FALSE(ReadIntParam(value, result));
    EXPECT_EQ(result, 42);
  }
}

TEST(CDPParamUtilsTest, ReadIntRejectsNonFiniteNumbers) {
  const double values[] = {std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN()};
  for (double value : values) {
    int result = 42;
    EXPECT_FALSE(ReadIntParam(Json::Value(value), result));
    EXPECT_EQ(result, 42);
  }
}

TEST(CDPParamUtilsTest, ReadBoolAcceptsOnlyBooleanValues) {
  bool result = false;
  EXPECT_TRUE(ReadBoolParam(Json::Value(true), result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(ReadBoolParam(Json::Value(false), result));
  EXPECT_FALSE(result);

  const std::vector<Json::Value> values = {Json::Value(),
                                           Json::Value(0),
                                           Json::Value(1),
                                           Json::Value("true"),
                                           Json::Value(Json::arrayValue),
                                           Json::Value(Json::objectValue)};
  for (const auto& value : values) {
    result = true;
    EXPECT_FALSE(ReadBoolParam(value, result));
    EXPECT_TRUE(result);
  }
}

TEST(CDPParamUtilsTest, ReadStringAcceptsOnlyStringValues) {
  const std::string expected("value\0with-null", 15);
  std::string result = "unchanged";
  EXPECT_TRUE(ReadStringParam(Json::Value(expected), result));
  EXPECT_EQ(result, expected);

  const std::vector<Json::Value> values = {Json::Value(),
                                           Json::Value(true),
                                           Json::Value(1),
                                           Json::Value(1.0),
                                           Json::Value(Json::arrayValue),
                                           Json::Value(Json::objectValue)};
  for (const auto& value : values) {
    result = "unchanged";
    EXPECT_FALSE(ReadStringParam(value, result));
    EXPECT_EQ(result, "unchanged");
  }
}

TEST(CDPParamUtilsTest, ReadIntStringAcceptsDecimalValuesWithinIntRange) {
  const int values[] = {std::numeric_limits<int>::min(), -1, 0, 1,
                        std::numeric_limits<int>::max()};
  for (int expected : values) {
    int result = 42;
    EXPECT_TRUE(
        ReadIntStringParam(Json::Value(std::to_string(expected)), result));
    EXPECT_EQ(result, expected);
  }
  int result = 42;
  EXPECT_TRUE(ReadIntStringParam(Json::Value("+001"), result));
  EXPECT_EQ(result, 1);
  EXPECT_TRUE(ReadIntStringParam(Json::Value("-0"), result));
  EXPECT_EQ(result, 0);
}

TEST(CDPParamUtilsTest, ReadIntStringRejectsMalformedStrings) {
  const std::vector<std::string> values = {"",
                                           "+",
                                           "-",
                                           " 1",
                                           "\t1",
                                           "1 ",
                                           "1suffix",
                                           "1.0",
                                           "1e2",
                                           "0x10",
                                           std::string("1\0suffix", 8),
                                           std::string("\xff", 1)};
  for (const auto& value : values) {
    int result = 42;
    EXPECT_FALSE(ReadIntStringParam(Json::Value(value), result));
    EXPECT_EQ(result, 42);
  }
}

TEST(CDPParamUtilsTest, ReadIntStringRejectsOverflow) {
  const std::vector<std::string> values = {
      std::to_string(static_cast<int64_t>(std::numeric_limits<int>::min()) - 1),
      std::to_string(static_cast<int64_t>(std::numeric_limits<int>::max()) + 1),
      "999999999999999999999999999999", "-999999999999999999999999999999"};
  for (const auto& value : values) {
    int result = 42;
    EXPECT_FALSE(ReadIntStringParam(Json::Value(value), result));
    EXPECT_EQ(result, 42);
  }
}

TEST(CDPParamUtilsTest, ReadIntStringRejectsNonStringTypes) {
  const std::vector<Json::Value> values = {Json::Value(),
                                           Json::Value(true),
                                           Json::Value(1),
                                           Json::Value(1.0),
                                           Json::Value(Json::arrayValue),
                                           Json::Value(Json::objectValue)};
  for (const auto& value : values) {
    int result = 42;
    EXPECT_FALSE(ReadIntStringParam(value, result));
    EXPECT_EQ(result, 42);
  }
}

TEST(CDPParamUtilsTest, ReadFiniteNumberAcceptsIntegerAndRealValues) {
  const std::vector<std::pair<Json::Value, double>> values = {
      {Json::Value(-1), -1.0},
      {Json::Value(0), 0.0},
      {Json::Value(static_cast<Json::UInt>(1)), 1.0},
      {Json::Value(0.5), 0.5},
      {Json::Value(std::numeric_limits<double>::max()),
       std::numeric_limits<double>::max()}};
  for (const auto& [value, expected] : values) {
    double result = 42.0;
    EXPECT_TRUE(ReadFiniteNumberParam(value, result));
    EXPECT_DOUBLE_EQ(result, expected);
  }
}

TEST(CDPParamUtilsTest, ReadFiniteNumberRejectsOtherTypesAndNonFiniteValues) {
  const std::vector<Json::Value> values = {
      Json::Value(),
      Json::Value(true),
      Json::Value("0.5"),
      Json::Value(Json::arrayValue),
      Json::Value(Json::objectValue),
      Json::Value(std::numeric_limits<double>::infinity()),
      Json::Value(-std::numeric_limits<double>::infinity()),
      Json::Value(std::numeric_limits<double>::quiet_NaN())};
  for (const auto& value : values) {
    double result = 42.0;
    EXPECT_FALSE(ReadFiniteNumberParam(value, result));
    EXPECT_DOUBLE_EQ(result, 42.0);
  }
}

TEST(CDPParamUtilsTest, ReadFiniteFloatAcceptsValuesWithinFloatRange) {
  const std::vector<std::pair<Json::Value, float>> values = {
      {Json::Value(-1), -1.0f},
      {Json::Value(0), 0.0f},
      {Json::Value(static_cast<Json::UInt>(1)), 1.0f},
      {Json::Value(0.5), 0.5f},
      {Json::Value(std::numeric_limits<float>::max()),
       std::numeric_limits<float>::max()},
      {Json::Value(-std::numeric_limits<float>::max()),
       -std::numeric_limits<float>::max()}};
  for (const auto& [value, expected] : values) {
    float result = 42.0f;
    EXPECT_TRUE(ReadFiniteFloatParam(value, result));
    EXPECT_FLOAT_EQ(result, expected);
  }
}

TEST(CDPParamUtilsTest, ReadFiniteFloatRejectsOtherTypesAndOutOfRangeValues) {
  const std::vector<Json::Value> values = {
      Json::Value(),
      Json::Value(true),
      Json::Value("0.5"),
      Json::Value(Json::arrayValue),
      Json::Value(Json::objectValue),
      Json::Value(std::numeric_limits<double>::infinity()),
      Json::Value(-std::numeric_limits<double>::infinity()),
      Json::Value(std::numeric_limits<double>::quiet_NaN()),
      Json::Value(std::numeric_limits<double>::max()),
      Json::Value(-std::numeric_limits<double>::max())};
  for (const auto& value : values) {
    float result = 42.0f;
    EXPECT_FALSE(ReadFiniteFloatParam(value, result));
    EXPECT_FLOAT_EQ(result, 42.0f);
  }
}

TEST(CDPParamUtilsTest, MissingFieldsAndNullParamsFailWithoutChangingOutput) {
  const Json::Value object(Json::objectValue);
  const Json::Value null;
  int result = 42;
  EXPECT_FALSE(ReadIntParam(object["nodeId"], result));
  EXPECT_FALSE(ReadIntParam(null["nodeId"], result));
  EXPECT_FALSE(ReadIntStringParam(object["layerId"], result));
  EXPECT_FALSE(ReadIntStringParam(null["layerId"], result));
  EXPECT_EQ(result, 42);
  double number_result = 42.0;
  EXPECT_FALSE(ReadFiniteNumberParam(object["a"], number_result));
  EXPECT_FALSE(ReadFiniteNumberParam(null["a"], number_result));
  EXPECT_DOUBLE_EQ(number_result, 42.0);
  float float_result = 42.0f;
  EXPECT_FALSE(ReadFiniteFloatParam(object["x"], float_result));
  EXPECT_FALSE(ReadFiniteFloatParam(null["x"], float_result));
  EXPECT_FLOAT_EQ(float_result, 42.0f);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
