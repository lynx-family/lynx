// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_DISCARDABLE_MEMORY_IMPL_H_
#define CLAY_MEMORY_DISCARDABLE_MEMORY_IMPL_H_

#include <atomic>
#include <vector>

#include "clay/memory/discardable_memory.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {
class DiscardableMemoryImpl : public DiscardableMemory {
 public:
  DiscardableMemoryImpl(size_t size_in_bytes,
                        std::atomic<size_t>* allocator_byte_count);
  ~DiscardableMemoryImpl() override;

  bool Lock() override;
  void Unlock() override;
  void* data() const override;

  bool IsValid() const;
  // Gets whether this instance has been discarded (but not yet unmapped).
  bool IsDiscarded() const;
  // Get whether all pages in this discardable memory instance are resident.
  bool IsResident() const;

  bool isLocked() const override { return is_locked_; }

  size_t GetSize() const override { return size_in_bytes_; }

 private:
  FRIEND_TEST(DiscardableMemoryAllocatorImplTest, TestAllocateAndUseMemory);
  FRIEND_TEST(DiscardableMemoryImplTest, TestAllocateAndUse);
  FRIEND_TEST(DiscardableMemoryImplTest, TestLockAndUnlock);

  BASE_DISALLOW_COPY_AND_ASSIGN(DiscardableMemoryImpl);

  bool LockPage(size_t page_index);
  void UnlockPage(size_t page_index);

  bool Deallocate();

  const size_t size_in_bytes_;
  const size_t allocated_pages_;

  // Data comes from mmap.
  void* data_;
  // data_ is locked initially and can be read and written.
  // However after UnLock, accessing data_ will be and undefined behavior.
  bool is_locked_ = true;

  // Pointer to allocator memory usage metric for updating upon allocation and
  // destruction.
  std::atomic<size_t>* allocator_byte_count_;

  // Stores the first word of a page for use during locking.
  std::vector<std::atomic<intptr_t>> page_first_word_;
};
}  // namespace clay

#endif  // CLAY_MEMORY_DISCARDABLE_MEMORY_IMPL_H_
