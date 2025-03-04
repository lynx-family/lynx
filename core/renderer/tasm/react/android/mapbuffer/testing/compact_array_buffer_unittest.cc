// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core/renderer/tasm/react/android/mapbuffer/compact_array_buffer_builder.h"

namespace lynx {
namespace tasm {

TEST(ArrayBufferTest, TestSimpleCompactArrayBuffer) {
  auto builder = base::android::CompactArrayBufferBuilder();
  builder.putInt(1234);
  builder.putInt(4321);
  builder.putString("Hello");
  builder.putString("");
  builder.putLong(180388626432);
  builder.putDouble(1.23456789);

  std::vector<int> vec = {1, 2, 3, 4, 5};
  builder.putIntArray(std::move(vec));

  auto array = builder.build();
  EXPECT_EQ(array.count(), 12);
  EXPECT_EQ(array.getInt(0), 1234);
  EXPECT_EQ(array.getInt(1), 4321);
  EXPECT_EQ(array.getString(2), "Hello");
  EXPECT_EQ(array.getString(3), "");
  EXPECT_EQ(array.getLong(4), 180388626432);
  EXPECT_EQ(array.getDouble(5), 1.23456789);
  EXPECT_EQ(array.getInt(6), 5);
  EXPECT_EQ(array.getInt(7), 1);
  EXPECT_EQ(array.getInt(8), 2);
  EXPECT_EQ(array.getInt(9), 3);
  EXPECT_EQ(array.getInt(10), 4);
  EXPECT_EQ(array.getInt(11), 5);
}

}  // namespace tasm
}  // namespace lynx
