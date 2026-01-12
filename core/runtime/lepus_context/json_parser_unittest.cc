// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus_context/json_parser.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {
namespace test {

class JsonParserTest : public ::testing::Test {
 public:
  JsonParserTest() = default;

  ~JsonParserTest() override = default;
};

TEST_F(JsonParserTest, lepusValueToJSONString) {
  Value value;
  value.SetNumber(123123123123.0);
  std::stringstream ss;

  lepusValueToJSONString(ss, value, false);
  EXPECT_EQ(ss.str(), "1.23123e+11");

  ss.str("");
  lepusValueToJSONString(ss, value, false, true);
  EXPECT_EQ(ss.str(), "123123123123.000000");
}

TEST_F(JsonParserTest, lepusValueToString) {
  Value value;
  value.SetNumber(123123123123.0);
  std::stringstream ss;

  std::string result;
  result = lepusValueToString(value);
  EXPECT_EQ(result, "1.23123e+11");

  result = lepusValueToString(value, false, true);
  EXPECT_EQ(result, "123123123123.000000");
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
