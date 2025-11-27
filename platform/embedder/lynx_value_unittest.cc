// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/public/lynx_value.h"

#include <cmath>

#include "base/include/value/base_string.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace pub {

class LynxValueTest : public ::testing::Test {
 protected:
  LynxValueTest() = default;
  ~LynxValueTest() override = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(LynxValueTest, LynxValueMap) {
  auto map_value = LynxValue(LynxValue::kCreateAsMapTag);
  EXPECT_EQ(map_value.Type(), lynx_value_map);
  map_value.SetProperty("key1", LynxValue(true));
  map_value.SetProperty("key2", LynxValue(3.14));
  map_value.SetProperty("key3", LynxValue((uint64_t)100));
  auto key1_ret = map_value.GetProperty("key1");
  ASSERT_TRUE(key1_ret.Bool());
  auto key2_ret = map_value.GetProperty("key2");
  ASSERT_TRUE(std::fabs(key2_ret.Double() - 3.14) < 0.001);
  auto key3_ret = map_value.GetProperty("key3");
  EXPECT_EQ(key3_ret.UInt64(), 100);
  ASSERT_TRUE(map_value.HasProperty("key1"));

  auto str = LynxValue("string_value");
  auto raw_str =
      reinterpret_cast<base::RefCountedStringImpl*>(str.Value().val_ptr);
  ASSERT_TRUE(raw_str->HasOneRef());
  {
    auto map = LynxValue(LynxValue::kCreateAsMapTag);
    map.SetProperty("key", str);
    ASSERT_FALSE(raw_str->HasOneRef());
    auto ret = map.GetProperty("key");
  }
  ASSERT_TRUE(raw_str->HasOneRef());
  map_value.SetProperty("key4", str);
  bool a;
  uint64_t b;
  int c;
  std::string ret_s;
  map_value.IteratorValue([&](const LynxValue& key, const LynxValue& value) {
    if (key.StdString() == "key1") {
      a = value.Bool();
    } else if (key.StdString() == "key3") {
      b = value.UInt64();
    } else if (key.StdString() == "key4") {
      c = raw_str->SubtleRefCountForDebug();
      ret_s = value.StdString();
    }
  });
  EXPECT_EQ(raw_str->SubtleRefCountForDebug(), 2);
  ASSERT_TRUE(a);
  EXPECT_EQ(b, 100);
  EXPECT_EQ(c, 3);
  EXPECT_EQ(ret_s, "string_value");
}

TEST_F(LynxValueTest, LynxValueArray) {
  auto array_value = LynxValue(LynxValue::kCreateAsArrayTag);
  EXPECT_EQ(array_value.Type(), lynx_value_array);
  array_value.InsertValue(0, LynxValue("string"));
  array_value.InsertValue(1, LynxValue(1001));
  array_value.InsertValue(2, LynxValue((uint32_t)100));
  EXPECT_EQ(array_value.ArrayLength(), 3);
  auto v1_ret = array_value.GetValueAt(0);
  EXPECT_EQ(v1_ret.StdString(), "string");
  auto v2_ret = array_value.GetValueAt(1);
  EXPECT_EQ(v2_ret.Int32(), 1001);
  auto v3_ret = array_value.GetValueAt(2);
  EXPECT_EQ(v3_ret.UInt32(), 100);
  EXPECT_EQ(array_value.GetValueAt(4).Type(), lynx_value_null);

  auto str = LynxValue("string_value");
  auto raw_str =
      reinterpret_cast<base::RefCountedStringImpl*>(str.Value().val_ptr);
  ASSERT_TRUE(raw_str->HasOneRef());
  {
    auto array = LynxValue(LynxValue::kCreateAsArrayTag);
    array.InsertValue(0, str);
    ASSERT_FALSE(raw_str->HasOneRef());
    auto ret = array.GetValueAt(0);
  }
  ASSERT_TRUE(raw_str->HasOneRef());
}

}  // namespace pub
}  // namespace lynx
