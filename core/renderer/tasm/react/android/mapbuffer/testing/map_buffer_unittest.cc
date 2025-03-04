
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "core/renderer/tasm/react/android/mapbuffer/map_buffer.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core/renderer/tasm/react/android/mapbuffer/map_buffer_builder.h"

namespace lynx {
namespace tasm {

TEST(MapBufferTest, TestSimpleIntMap) {
  auto builder = base::android::MapBufferBuilder();

  builder.putInt(0, 1234);
  builder.putInt(1, 4321);

  auto map = builder.build();

  EXPECT_EQ(map.count(), 2);
  EXPECT_EQ(map.getInt(0), 1234);
  EXPECT_EQ(map.getInt(1), 4321);
}

TEST(MapBufferTest, TestBoolEntries) {
  auto buffer = base::android::MapBufferBuilder();

  buffer.putBool(0, true);
  buffer.putBool(1, false);

  auto map = buffer.build();

  EXPECT_EQ(map.count(), 2);
  EXPECT_EQ(map.getBool(0), true);
  EXPECT_EQ(map.getBool(1), false);
}

TEST(MapBufferTest, TestDoubleEntries) {
  auto buffer = base::android::MapBufferBuilder();

  buffer.putDouble(0, 123.4);
  buffer.putDouble(1, 432.1);

  auto map = buffer.build();

  EXPECT_EQ(map.count(), 2);

  EXPECT_EQ(map.getDouble(0), 123.4);
  EXPECT_EQ(map.getDouble(1), 432.1);
}

TEST(MapBufferTest, TestStringEntries) {
  auto buffer = base::android::MapBufferBuilder();

  buffer.putString(0, "This is a test");
  auto map = buffer.build();

  EXPECT_EQ(map.getString(0), "This is a test");
}

TEST(MapBufferTest, TestEmojiStringEntry) {
  auto builder = base::android::MapBufferBuilder();

  builder.putString(
      0, "Let's count: 1️⃣, 2️⃣, 3️⃣, 🤦🏿‍♀️");
  auto map = builder.build();

  EXPECT_EQ(map.getString(0),
            "Let's count: 1️⃣, 2️⃣, 3️⃣, 🤦🏿‍♀️");
}

TEST(MapBufferTest, TestUTFStringEntries) {
  auto builder = base::android::MapBufferBuilder();

  builder.putString(0, "Let's count: こんにちは");
  builder.putString(1, "This is a test");
  auto map = builder.build();

  EXPECT_EQ(map.getString(0), "Let's count: こんにちは");
  EXPECT_EQ(map.getString(1), "This is a test");
}

TEST(MapBufferTest, TestEmptyMap) {
  auto builder = base::android::MapBufferBuilder();
  auto map = builder.build();
  EXPECT_EQ(map.count(), 0);
}

TEST(MapBufferTest, TestEmptyMapConstant) {
  auto map = base::android::MapBufferBuilder::EMPTY();
  EXPECT_EQ(map.count(), 0);
}

TEST(MapBufferTest, TestMapEntries) {
  auto builder = base::android::MapBufferBuilder();
  builder.putString(0, "This is a test");
  builder.putInt(1, 1234);
  auto map = builder.build();

  EXPECT_EQ(map.count(), 2);
  EXPECT_EQ(map.getString(0), "This is a test");
  EXPECT_EQ(map.getInt(1), 1234);

  auto builder_2 = base::android::MapBufferBuilder();
  builder_2.putInt(0, 4321);
  builder_2.putMapBuffer(1, map);
  auto map_2 = builder_2.build();

  EXPECT_EQ(map_2.count(), 2);
  EXPECT_EQ(map_2.getInt(0), 4321);

  base::android::MapBuffer read_map_2 = map_2.getMapBuffer(1);

  EXPECT_EQ(read_map_2.count(), 2);
  EXPECT_EQ(read_map_2.getString(0), "This is a test");
  EXPECT_EQ(read_map_2.getInt(1), 1234);
}

TEST(MapBufferTest, TestMapRandomAccess) {
  auto builder = base::android::MapBufferBuilder();
  builder.putInt(1234, 4321);
  builder.putString(0, "This is a test");
  builder.putDouble(8, 908.1);
  auto map = builder.build();

  EXPECT_EQ(map.count(), 3);
  EXPECT_EQ(map.getString(0), "This is a test");
  EXPECT_EQ(map.getDouble(8), 908.1);
  EXPECT_EQ(map.getInt(1234), 4321);
}

}  // namespace tasm
}  // namespace lynx
