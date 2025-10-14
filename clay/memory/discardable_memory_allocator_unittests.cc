// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "build/build_config.h"
#include "clay/memory/discardable_memory_allocator_impl.h"

#if OS_ANDROID || OS_MAC
#include <sys/mman.h>
#elif OS_WIN
#include <windows.h>
#endif

#if OS_ANDROID || OS_MAC || OS_WIN
#include "clay/memory/discardable_memory_impl.h"
#else
#include "clay/memory/heap_discardable_memory.h"
#endif

#include <cstring>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

#if OS_ANDROID || OS_MAC || OS_WIN
namespace {

size_t GetPageSize() {
#if OS_ANDROID || OS_MAC
#if defined(_SC_PAGESIZE)
  return sysconf(_SC_PAGESIZE);
#else
  return getpagesize();
#endif
#elif OS_WIN
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwPageSize;
#else
#error "discardable memory is not supported on this platform."
#endif
}

const size_t kPageSize = GetPageSize();

}  // namespace

TEST(DiscardableMemoryAllocatorImplTest, TestAllocateAndUseMemory) {
  DiscardableMemoryAllocatorImpl allocator;

  // Allocate 4 pages.
  std::unique_ptr<DiscardableMemory> mem1 =
      allocator.AllocateLockedDiscardableMemory(kPageSize * 3 + 1);

  EXPECT_TRUE(static_cast<DiscardableMemoryImpl*>(mem1.get())->is_locked_);
  EXPECT_EQ(allocator.GetBytesAllocated(), kPageSize * 3 + 1);

  // Allocate 3 pages.
  std::unique_ptr<DiscardableMemory> mem2 =
      allocator.AllocateLockedDiscardableMemory(kPageSize * 3);

  EXPECT_TRUE(static_cast<DiscardableMemoryImpl*>(mem2.get())->is_locked_);
  EXPECT_EQ(allocator.GetBytesAllocated(), kPageSize * 6 + 1);

  mem1.reset();
  EXPECT_EQ(allocator.GetBytesAllocated(), kPageSize * 3);

  // Test writing and reading of discardable memory.
  const char test_pattern[] = "TestDiscardableMemory";
  char buffer[sizeof(test_pattern)];

  void* data = mem2->data();
  memcpy(data, test_pattern, sizeof(test_pattern));
  memcpy(buffer, mem2->data_as<char>(), sizeof(test_pattern));

  EXPECT_EQ(memcmp(test_pattern, buffer, sizeof(test_pattern)), 0);
}
#else
// Test allocating HeapDiscardableMemory.
const size_t kPageSize = 1024;
TEST(DiscardableMemoryAllocatorImplTest, TestAllocateAndUseMemory) {
  DiscardableMemoryAllocatorImpl allocator;
  auto mem1 = allocator.AllocateLockedDiscardableMemory(kPageSize * 3 + 1);
  EXPECT_TRUE(static_cast<HeapDiscardableMemory*>(mem1.get()));
  EXPECT_NE(static_cast<HeapDiscardableMemory*>(mem1.get())->memory_, nullptr);
  EXPECT_EQ(static_cast<HeapDiscardableMemory*>(mem1.get())->size_,
            kPageSize * 3 + 1);
}
#endif  // OS_ANDROID || OS_MAC || OS_WIN

}  // namespace clay
