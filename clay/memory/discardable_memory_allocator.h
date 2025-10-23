// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_DISCARDABLE_MEMORY_ALLOCATOR_H_
#define CLAY_MEMORY_DISCARDABLE_MEMORY_ALLOCATOR_H_

#include <functional>
#include <memory>

#include "clay/memory/discardable_memory.h"

namespace clay {
class DiscardableMemoryAllocator {
 public:
  using OnNoMemCallback = std::function<void()>;
  DiscardableMemoryAllocator() = default;
  virtual ~DiscardableMemoryAllocator() = default;

  // Creates an initially-locked instance of discardable memory.
  // If the platform supports madvise(MADV_FREE) like Android,
  // platform-specific techniques will be used to discard memory under pressure.
  // Otherwise, discardable memory is emulated and manually discarded
  // heuristically (via memory pressure notifications).
  virtual std::unique_ptr<DiscardableMemory> AllocateLockedDiscardableMemory(
      size_t size) = 0;

  // Gets the total number of bytes allocated by this allocator which have not
  // been discarded.
  virtual size_t GetBytesAllocated() const = 0;

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(DiscardableMemoryAllocator);
};
}  // namespace clay

#endif  // CLAY_MEMORY_DISCARDABLE_MEMORY_ALLOCATOR_H_
