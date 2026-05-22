// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_HEAP_DISCARDABLE_MEMORY_H_
#define CLAY_MEMORY_HEAP_DISCARDABLE_MEMORY_H_

#include <memory>

#include "clay/fml/logging.h"
#include "clay/memory/discardable_memory.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class HeapDiscardableMemory : public DiscardableMemory {
 public:
  explicit HeapDiscardableMemory(size_t size)
      : memory_(new char[size]), size_(size) {}

  ~HeapDiscardableMemory() override = default;

  [[nodiscard]] bool Lock() override {
    // Locking only succeeds when we have not yet discarded the memory (i.e. if
    // we have never called |Unlock()|.)
    return memory_ != nullptr;
  }

  void Unlock() override { Discard(); }

  void* data() const override {
    FML_DCHECK(memory_);
    return static_cast<void*>(memory_.get());
  }

  bool isLocked() const override { return true; }

  size_t GetSize() const override { return size_; }

 private:
  FRIEND_TEST(HeapDiscardableMemoryTest, TestAllocateAndUse);
  FRIEND_TEST(HeapDiscardableMemoryTest, TestLockAndUnlock);
  FRIEND_TEST(DiscardableMemoryAllocatorImplTest, TestAllocateAndUseMemory);

  void Discard() {
    memory_.reset();
    size_ = 0;
  }
  std::unique_ptr<char[]> memory_;
  size_t size_;
};

}  // namespace clay

#endif  // CLAY_MEMORY_HEAP_DISCARDABLE_MEMORY_H_
