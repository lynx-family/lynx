// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstring>

#include "clay/memory/heap_discardable_memory.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

const size_t kTestPageSize = 1024;
const char test_pattern[] =
    "fhjasfhasjkdfhasdkgasdsajfksaljrw4i5j23krkasfk234123ntklesfasKASflkSFKLSD";

TEST(HeapDiscardableMemoryTest, TestAllocateAndUse) {
  auto mem = std::make_unique<HeapDiscardableMemory>(1 * kTestPageSize);
  EXPECT_EQ(mem->size_, 1 * kTestPageSize);
  EXPECT_NE(mem->data(), nullptr);

  char buffer[sizeof(test_pattern)];
  void* data = mem->data();
  memcpy(data, test_pattern, sizeof(test_pattern));
  memcpy(buffer, mem->data_as<char>(), sizeof(test_pattern));

  EXPECT_EQ(memcmp(test_pattern, buffer, sizeof(test_pattern)), 0);
}

TEST(HeapDiscardableMemoryTest, TestLockAndUnlock) {
  auto mem = std::make_unique<HeapDiscardableMemory>(10 * kTestPageSize);
  EXPECT_EQ(mem->size_, 10 * kTestPageSize);
  EXPECT_NE(mem->data(), nullptr);

  memset(mem->data(), 0xE7, 10 * kTestPageSize);
  EXPECT_EQ(mem->Lock(), true);
  mem->Unlock();
  EXPECT_EQ(mem->size_, 0u);
  EXPECT_EQ(mem->memory_, nullptr);
}

}  // namespace clay
