// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_DISCARDABLE_MEMORY_ALLOCATOR_IMPL_H_
#define CLAY_MEMORY_DISCARDABLE_MEMORY_ALLOCATOR_IMPL_H_

#include <atomic>
#include <memory>

#include "clay/memory/discardable_memory_allocator.h"

namespace clay {
class DiscardableMemoryAllocatorImpl : public DiscardableMemoryAllocator {
 public:
  DiscardableMemoryAllocatorImpl();
  ~DiscardableMemoryAllocatorImpl() override;

  static DiscardableMemoryAllocator& GetInstance();

  std::unique_ptr<DiscardableMemory> AllocateLockedDiscardableMemory(
      size_t size) override;

  size_t GetBytesAllocated() const override;

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(DiscardableMemoryAllocatorImpl);

  std::atomic<size_t> bytes_allocated_{0};
};
}  // namespace clay

#endif  // CLAY_MEMORY_DISCARDABLE_MEMORY_ALLOCATOR_IMPL_H_
