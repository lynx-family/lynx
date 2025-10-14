// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "build/build_config.h"
#include "clay/memory/discardable_memory_impl.h"

#if OS_ANDROID || OS_MAC
#include <sys/mman.h>
#elif OS_WIN
#include <windows.h>
#endif

#include <cstring>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
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
#error "Unsupported platform for discardable_memory_unittests";
#endif
}

const size_t kPageSize = GetPageSize();
const char test_pattern[] =
    "fhjasfhasjkdfhasdkgasdsajfksaljrw4i5j23krkasfk234123ntklesfasKASflkSFKLSD";
}  // namespace

TEST(DiscardableMemoryImplTest, TestAllocateAndUse) {
  std::atomic<size_t> allocator_byte_count{};
  std::unique_ptr<DiscardableMemoryImpl> mem =
      std::make_unique<DiscardableMemoryImpl>(1 * kPageSize,
                                              &allocator_byte_count);
  EXPECT_TRUE(mem->IsValid());
  EXPECT_TRUE(mem->is_locked_);

  char buffer[sizeof(test_pattern)];
  void* data = mem->data();
  memcpy(data, test_pattern, sizeof(test_pattern));
  memcpy(buffer, mem->data_as<char>(), sizeof(test_pattern));

  EXPECT_EQ(memcmp(test_pattern, buffer, sizeof(test_pattern)), 0);
}

TEST(DiscardableMemoryImplTest, TestLockAndUnlock) {
  std::atomic<size_t> allocator_byte_count{};
  std::unique_ptr<DiscardableMemoryImpl> mem =
      std::make_unique<DiscardableMemoryImpl>(10 * kPageSize,
                                              &allocator_byte_count);
  EXPECT_TRUE(mem->IsValid());
  EXPECT_TRUE(mem->is_locked_);

  memset(mem->data(), 0xE7, 10 * kPageSize);

  mem->Unlock();
  EXPECT_FALSE(mem->is_locked_);
  bool result = mem->Lock();
  // If Lock() succeeded, the memory region should be valid. If Lock() failed,
  // the memory region should be invalid.
  EXPECT_EQ(result, mem->IsValid());
}

}  // namespace clay
