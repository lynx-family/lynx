// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/android/lepus_message_consumer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
namespace {

class LepusMessageConsumerTest : public ::testing::Test {
 protected:
  std::vector<int8_t> EncodeToBytes(const lepus::Value& value) {
    LepusEncoder encoder;
    return encoder.EncodeMessage(value);
  }
};

TEST_F(LepusMessageConsumerTest, EncodePrimitiveValuesProducesBytes) {
  EXPECT_FALSE(EncodeToBytes(lepus::Value(42)).empty());
  EXPECT_FALSE(
      EncodeToBytes(lepus::Value(static_cast<int64_t>(1) << 40)).empty());
  EXPECT_FALSE(EncodeToBytes(lepus::Value(6.25)).empty());
  EXPECT_FALSE(EncodeToBytes(lepus::Value(std::string("hello"))).empty());
  EXPECT_FALSE(EncodeToBytes(lepus::Value(true)).empty());
}

TEST_F(LepusMessageConsumerTest, EncodeCompositeValuesProducesBytes) {
  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value(7));
  array->push_back(lepus::Value(std::string("world")));
  array->push_back(lepus::Value(false));

  auto dict = lepus::Dictionary::Create();
  dict->SetValue("int_val", lepus::Value(42));
  dict->SetValue("long_val", lepus::Value(static_cast<int64_t>(1) << 40));
  dict->SetValue("double_val", lepus::Value(6.25));
  dict->SetValue("string_val", lepus::Value(std::string("hello")));
  dict->SetValue("list_val", lepus::Value(array));

  std::vector<int8_t> encoded = EncodeToBytes(lepus::Value(dict));
  ASSERT_FALSE(encoded.empty());
  ASSERT_EQ(static_cast<uint8_t>(encoded.front()), 8u);
}

TEST_F(LepusMessageConsumerTest, EncodeByteArrayProducesBytes) {
  auto data = std::make_unique<uint8_t[]>(4);
  data[0] = 0x01;
  data[1] = 0x02;
  data[2] = 0x03;
  data[3] = 0x04;
  auto byte_array = lepus::ByteArray::Create(std::move(data), 4);

  std::vector<int8_t> encoded = EncodeToBytes(lepus::Value(byte_array));
  ASSERT_FALSE(encoded.empty());
  ASSERT_EQ(static_cast<uint8_t>(encoded.front()), 9u);
}

}  // namespace
}  // namespace test
}  // namespace tasm
}  // namespace lynx
