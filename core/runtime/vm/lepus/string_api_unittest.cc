// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/string_api.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

static const std::string test_data1 = "64.0";
static const std::string test_data2 = "640.";
static const std::string need_to_replace = ".";

// ref:
// https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Global_Objects/String/replace#%E4%BD%BF%E7%94%A8%E5%AD%97%E7%AC%A6%E4%B8%B2%E4%BD%9C%E4%B8%BA%E5%8F%82%E6%95%B0
TEST(LepusGetReplaceStrTest1, HasSameStringReturnTrue) {
  const std::string replace_to = "";
  int32_t position = test_data1.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data1, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "");
}

TEST(LepusGetReplaceStrTest2, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-";
  int32_t position = test_data1.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data1, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-");
}

TEST(LepusGetReplaceStrTest3, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$&-";
  int32_t position = test_data1.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data1, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-.-");
}

TEST(LepusGetReplaceStrTest4, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$`-";
  int32_t position = test_data1.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data1, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-64-");
}

TEST(LepusGetReplaceStrTest5, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$'-";
  int32_t position = test_data1.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data1, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-0-");
}

TEST(LepusGetReplaceStrTest6, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$`$'-";
  int32_t position = test_data1.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data1, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-640-");
}

TEST(LepusGetReplaceStrTest7, HasSameStringReturnTrue) {
  const std::string replace_to = "";
  int32_t position = test_data2.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data2, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "");
}

TEST(LepusGetReplaceStrTest8, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-";
  int32_t position = test_data2.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data2, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-");
}

TEST(LepusGetReplaceStrTest9, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$&-";
  int32_t position = test_data2.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data2, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-.-");
}

TEST(LepusGetReplaceStrTest10, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$`-";
  int32_t position = test_data2.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data2, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-640-");
}

TEST(LepusGetReplaceStrTest11, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$'-";
  int32_t position = test_data2.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data2, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$--");
}

TEST(LepusGetReplaceStrTest12, HasSameStringReturnTrue) {
  const std::string replace_to = "-$$-$`$'-";
  int32_t position = test_data2.find(need_to_replace);
  std::string replace_result =
      lepus::GetReplaceStr(test_data2, need_to_replace, replace_to, position);
  ASSERT_TRUE(replace_result == "-$-640-");
}

}  // namespace base
}  // namespace lynx
