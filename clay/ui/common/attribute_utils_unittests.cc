// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/public/value.h"
#include "clay/ui/common/attribute_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(AttributeUtilsTest, GetBool) {
  clay::Value value{true};
  EXPECT_EQ(attribute_utils::GetBool(value), true);

  value = clay::Value{false};
  EXPECT_EQ(attribute_utils::GetBool(value), false);

  value = clay::Value{"true"};
  EXPECT_EQ(attribute_utils::GetBool(value), true);

  value = clay::Value{"false"};
  EXPECT_EQ(attribute_utils::GetBool(value), false);

  value = clay::Value{"anything"};
  EXPECT_EQ(attribute_utils::GetBool(value), false);

  value = clay::Value{ClayPointer{ClayPointer::kClayPointerTypeVoidPtr,
                                  reinterpret_cast<void*>(0x1)}};
  EXPECT_EQ(attribute_utils::GetBool(value), true);

  value =
      clay::Value{ClayPointer{ClayPointer::kClayPointerTypeVoidPtr, nullptr}};
  EXPECT_EQ(attribute_utils::GetBool(value), false);

  value = clay::Value{1};
  EXPECT_EQ(attribute_utils::GetBool(value), true);

  value = clay::Value{0};
  EXPECT_EQ(attribute_utils::GetBool(value), false);

  value = clay::Value{int64_t(1)};
  EXPECT_EQ(attribute_utils::GetBool(value), true);
}

TEST(AttributeUtilsTest, GetCString) {
  clay::Value value{true};
  EXPECT_EQ(attribute_utils::GetCString(value), "true");

  value = clay::Value{false};
  EXPECT_EQ(attribute_utils::GetCString(value), "false");

  value = clay::Value{"true"};
  EXPECT_EQ(attribute_utils::GetCString(value), "true");

  value = clay::Value{"false"};
  EXPECT_EQ(attribute_utils::GetCString(value), "false");

  value = clay::Value{"anything"};
  EXPECT_EQ(attribute_utils::GetCString(value), "anything");

  value = clay::Value{1};
  EXPECT_EQ(attribute_utils::GetCString(value), "1");

  value = clay::Value{0};
  EXPECT_EQ(attribute_utils::GetCString(value), "0");

  value = clay::Value{-1};
  EXPECT_EQ(attribute_utils::GetCString(value), "-1");

  value = clay::Value{int64_t(1)};
  EXPECT_EQ(attribute_utils::GetCString(value), "1");

  value = clay::Value{float(1)};
  EXPECT_EQ(attribute_utils::GetCString(value), "1");

  value = clay::Value{0};
  EXPECT_EQ(attribute_utils::GetCString(value), "0");

  value = clay::Value{-1};
  EXPECT_EQ(attribute_utils::GetCString(value), "-1");

  value = clay::Value{double(1)};
  EXPECT_EQ(attribute_utils::GetCString(value), "1");

  value = clay::Value{0};
  EXPECT_EQ(attribute_utils::GetCString(value), "0");

  value = clay::Value{-1};
  EXPECT_EQ(attribute_utils::GetCString(value), "-1");

  value = clay::Value{3.1000000009};
  EXPECT_EQ(attribute_utils::GetCString(value), "3.1000000009");

  value = clay::Value{3.10000000000009};
  EXPECT_EQ(attribute_utils::GetCString(value), "3.10000000000009");

  value = clay::Value{3.100000000000009};
  EXPECT_EQ(attribute_utils::GetCString(value), "3.100000000000009");

  value = clay::Value{3.10000000000000009};
  EXPECT_EQ(attribute_utils::GetCString(value), "3.1");

  value = clay::Value{3e60};
  EXPECT_EQ(attribute_utils::GetCString(value), "3e+60");

  value = clay::Value{3e-60};
  EXPECT_EQ(attribute_utils::GetCString(value), "3e-60");

  value = clay::Value{NAN};
  EXPECT_EQ(attribute_utils::GetCString(value), "NaN");

  value = clay::Value{INFINITY};
  EXPECT_EQ(attribute_utils::GetCString(value), "Infinity");

  value = clay::Value{-INFINITY};
  EXPECT_EQ(attribute_utils::GetCString(value), "-Infinity");
}

TEST(AttributeUtilsTest, GetArray) {
  clay::Value::Array array_wrapper;
  array_wrapper.emplace_back(double(100));
  array_wrapper.emplace_back(0);
  auto array = clay::Value(std::move(array_wrapper));
  unsigned long size = 2;
  EXPECT_EQ(attribute_utils::GetArray(std::move(array)).size(), size);

  clay::Value value = clay::Value{"anything"};
  EXPECT_TRUE(attribute_utils::GetArray(value).empty());
}

TEST(AttributeUtilsTest, TryGetPlatformLength) {
  auto equals = [](ClayPlatformLength& a, ClayPlatformLength& b) -> bool {
    return a.value == b.value && a.unit == b.unit;
  };
  clay::Value::Array array;
  array.emplace_back(double(100));
  array.emplace_back(0);
  array.emplace_back(4u);
  array.emplace_back(10);
  array.emplace_back(false);
  ClayPlatformLength result;
  ClayPlatformLength default_val{20, ClayPlatformLengthUnit::kPercentage};
  attribute_utils::TryGetPlatformLength(array, 0, result, {});
  ClayPlatformLength expected{100, ClayPlatformLengthUnit::kNumber};

  EXPECT_TRUE(equals(result, expected));
  attribute_utils::TryGetPlatformLength(array, 1, result, default_val);
  EXPECT_TRUE(equals(result, default_val));
  attribute_utils::TryGetPlatformLength(array, 2, result, default_val);
  EXPECT_TRUE(equals(result, default_val));
  attribute_utils::TryGetPlatformLength(array, 4, result, default_val);
  EXPECT_TRUE(equals(result, default_val));
}

}  // namespace clay
