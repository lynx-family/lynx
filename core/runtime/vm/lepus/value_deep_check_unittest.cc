// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/value/base_value.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

constexpr char foo[] = "foo";
constexpr char bar[] = "bar";
constexpr char bar_other[] = "bar_other";

namespace lynx {
namespace tasm {
namespace test {

class LepusValueDeepCheckTest : public ::testing::Test {
 protected:
  void SetUp() override {
    lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
        lynx::tasm::LynxEnv::kLynxEnableTableDeepCheck, true);
  }

  void TearDown() override {
    lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
        lynx::tasm::LynxEnv::kLynxEnableTableDeepCheck, false);
  }
};

TEST_F(LepusValueDeepCheckTest, DifferentTypeCompare) {
  // compare 1 vs "bar", {} vs "bar"
  auto empty_map = lepus::Value(lepus::Dictionary::Create());
  auto bar_value = lepus::Value(bar);
  auto number_one = lepus::Value(1);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(empty_map),
                                            lepus::Value(bar_value)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(bar_value),
                                            lepus::Value(empty_map)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(bar_value),
                                            lepus::Value(number_one)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(number_one),
                                            lepus::Value(bar_value)));
}

TEST_F(LepusValueDeepCheckTest, StringTypeCompare) {
  // compare "bar" vs "bar", "bar" vs "foo"
  auto bar_value_1 = lepus::Value(bar);
  auto bar_value_2 = lepus::Value(bar);
  auto foo_value = lepus::Value(foo);

  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(bar_value_1),
                                             lepus::Value(bar_value_2)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(bar_value_2),
                                             lepus::Value(bar_value_1)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(bar_value_1),
                                            lepus::Value(foo_value)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(foo_value),
                                            lepus::Value(bar_value_1)));
}

TEST_F(LepusValueDeepCheckTest, SameIntTypeCompare) {
  // compare 1 vs 1, 1 vs 2
  auto number_value_one = lepus::Value(1);
  auto number_value_one_again = lepus::Value(1);
  auto number_value_two = lepus::Value(2);

  ASSERT_FALSE(tasm::CheckTableShadowUpdated(
      lepus::Value(number_value_one), lepus::Value(number_value_one_again)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(
      lepus::Value(number_value_one_again), lepus::Value(number_value_one)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(number_value_one),
                                            lepus::Value(number_value_two)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(number_value_two),
                                            lepus::Value(number_value_one)));
}

TEST_F(LepusValueDeepCheckTest, EmptyTableCompare) {
  // compare {} vs "bar", {} vs {}
  auto update_map = lepus::Dictionary::Create();
  auto target_string = lynx::base::String(bar);
  auto target_map = lepus::Dictionary::Create();

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(target_string),
                                            lepus::Value(update_map)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                             lepus::Value(update_map)));

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                            lepus::Value(target_string)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                             lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, SameKeySameStringValueTable) {
  // compare {"foo": "bar"} vs {"foo": "bar"}
  lepus::Value v(bar);

  auto target_map = lepus::Dictionary::Create();
  target_map.get()->SetValue(base::String(foo), v);
  auto update_map = lepus::Dictionary::Create();
  update_map.get()->SetValue(base::String(foo), v);

  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                             lepus::Value(update_map)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                             lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, SameKeySameTableValueTable) {
  // compare {"foo": {"bar": "bar_other"}} vs {"foo": {"bar": "bar_other"}}
  lepus::Value v(bar_other);
  auto target_map = lepus::Dictionary::Create();
  auto target_child_map = lepus::Dictionary::Create();
  target_child_map.get()->SetValue(base::String(bar), v);
  target_map.get()->SetValue(base::String(foo), lepus::Value(target_child_map));

  auto update_map = lepus::Dictionary::Create();
  auto update_child_map = lepus::Dictionary::Create();
  update_child_map.get()->SetValue(base::String(bar), v);
  update_map.get()->SetValue(base::String(foo), lepus::Value(update_child_map));

  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                             lepus::Value(update_map)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                             lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, SameKeySameArrayValueTable) {
  // compare {"foo": ["bar", "bar_other"]} vs {"foo": ["bar", "bar_other"]}
  lepus::Value v1(bar_other);
  lepus::Value v2(bar);

  auto target_map = lepus::Dictionary::Create();
  auto target_child_array = lepus::CArray::Create();
  target_child_array->push_back(v1);
  target_child_array->push_back(v2);
  target_map.get()->SetValue(base::String(foo),
                             lepus::Value(target_child_array));

  auto update_map = lepus::Dictionary::Create();
  auto update_child_array = lepus::CArray::Create();
  update_child_array->push_back(v1);
  update_child_array->push_back(v2);
  update_map.get()->SetValue(base::String(foo),
                             lepus::Value(update_child_array));

  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                             lepus::Value(update_map)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                             lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, PartSameKeySameStringValueTable) {
  // compare {"foo": "bar"} vs {"foo": "bar", "bar": "bar_other"}
  lepus::Value v1(bar);
  lepus::Value v2(bar_other);

  auto target_map = lepus::Dictionary::Create();
  target_map.get()->SetValue(base::String(foo), v1);
  auto update_map = lepus::Dictionary::Create();
  update_map.get()->SetValue(base::String(foo), v1);
  update_map.get()->SetValue(base::String(bar), v2);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                            lepus::Value(update_map)));
  // First layer table, do not check table size.
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                             lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, PartSameKeySameTableValueTable) {
  // compare {"foo": {"bar": "bar_other"}} vs {"foo": {"bar": "bar_other"},
  // "bar": "bar_other"}
  lepus::Value v(bar_other);
  auto target_map = lepus::Dictionary::Create();
  auto target_child_map = lepus::Dictionary::Create();
  target_child_map.get()->SetValue(base::String(bar), v);
  target_map.get()->SetValue(base::String(foo), lepus::Value(target_child_map));

  auto update_map = lepus::Dictionary::Create();
  auto update_child_map = lepus::Dictionary::Create();
  update_child_map.get()->SetValue(base::String(bar), v);
  update_map.get()->SetValue(base::String(foo), lepus::Value(update_child_map));
  update_map.get()->SetValue(base::String(bar), v);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                            lepus::Value(update_map)));
  // First layer table, do not check table size.
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                             lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, PartSameKeySameArrayValueTable) {
  // compare {"foo": ["bar", "bar_other"]} vs {"foo": ["bar", "bar_other"],
  // "bar": "bar_other"}
  lepus::Value v1(bar_other);
  lepus::Value v2(bar);

  auto target_map = lepus::Dictionary::Create();
  auto target_child_array = lepus::CArray::Create();
  target_child_array->push_back(v1);
  target_child_array->push_back(v2);
  target_map.get()->SetValue(base::String(foo),
                             lepus::Value(target_child_array));

  auto update_map = lepus::Dictionary::Create();
  auto update_child_array = lepus::CArray::Create();
  update_child_array->push_back(v1);
  update_child_array->push_back(v2);
  update_map.get()->SetValue(base::String(foo),
                             lepus::Value(update_child_array));
  update_map.get()->SetValue(base::String(bar), v1);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                            lepus::Value(update_map)));
  ASSERT_FALSE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                             lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, SameStringKeyDifferentStringValueTable) {
  // compare {"foo": "bar"} vs {"foo": "bar_other"}
  auto target_map = lepus::Dictionary::Create();
  lepus::Value bar_other_value(bar_other);
  target_map.get()->SetValue(base::String(foo), bar_other_value);

  auto update_map = lepus::Dictionary::Create();
  lepus::Value bar_value(bar);
  update_map.get()->SetValue(base::String(foo), bar_value);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                            lepus::Value(update_map)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                            lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, SameStringKeyDifferentTableValueTable) {
  // compare {"foo": {"bar": "bar_other"}} vs {"foo": {"bar": "bar"}},  {"foo":
  // {"bar": "bar_other"}} vs {"foo": {"foo": "bar_other"}}
  lepus::Value bar_other_value(bar_other);
  lepus::Value bar_value(bar);

  auto map_one = lepus::Dictionary::Create();
  auto map_one_child = lepus::Dictionary::Create();
  map_one_child.get()->SetValue(base::String(bar), bar_other_value);
  map_one.get()->SetValue(base::String(foo), lepus::Value(map_one_child));

  auto map_two = lepus::Dictionary::Create();
  auto map_two_child = lepus::Dictionary::Create();
  map_two_child.get()->SetValue(base::String(bar), bar_value);
  map_two.get()->SetValue(base::String(foo), lepus::Value(map_two_child));

  auto map_three = lepus::Dictionary::Create();
  auto map_three_child = lepus::Dictionary::Create();
  map_three_child.get()->SetValue(base::String(foo), bar_other_value);
  map_three.get()->SetValue(base::String(foo), lepus::Value(map_three_child));

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(map_one),
                                            lepus::Value(map_two)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(map_two),
                                            lepus::Value(map_one)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(map_one),
                                            lepus::Value(map_three)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(map_three),
                                            lepus::Value(map_one)));
}

TEST_F(LepusValueDeepCheckTest, SameKeyDifferentArrayValueTable) {
  // compare {"foo": ["bar_other", "bar"]} vs {"foo": ["bar", "bar_other"]}
  lepus::Value v1(bar_other);
  lepus::Value v2(bar);

  auto target_map = lepus::Dictionary::Create();
  auto target_child_array = lepus::CArray::Create();
  target_child_array->push_back(v2);
  target_child_array->push_back(v1);
  target_map.get()->SetValue(base::String(foo),
                             lepus::Value(target_child_array));

  auto update_map = lepus::Dictionary::Create();
  auto update_child_array = lepus::CArray::Create();
  update_child_array->push_back(v1);
  update_child_array->push_back(v2);
  update_map.get()->SetValue(base::String(foo),
                             lepus::Value(update_child_array));

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                            lepus::Value(update_map)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                            lepus::Value(target_map)));
}

TEST_F(LepusValueDeepCheckTest, DifferentKeyTable) {
  // compare {"foo", "bar"} vs {"bar_other": "bar"}
  auto update_map = lepus::Dictionary::Create();
  lepus::Value bar_value(bar);
  update_map.get()->SetValue(base::String(foo), bar_value);

  auto target_map = lepus::Dictionary::Create();
  target_map.get()->SetValue(base::String(bar_other), bar_value);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(update_map),
                                            lepus::Value(target_map)));
  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(target_map),
                                            lepus::Value(update_map)));
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
