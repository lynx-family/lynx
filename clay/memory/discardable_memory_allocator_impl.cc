// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/memory/discardable_memory_allocator_impl.h"

#include "base/include/no_destructor.h"
#include "build/build_config.h"

#if OS_ANDROID || OS_MAC || OS_WIN
#include "clay/memory/discardable_memory_impl.h"
#else
#include "clay/memory/heap_discardable_memory.h"
#endif

namespace clay {

DiscardableMemoryAllocatorImpl::DiscardableMemoryAllocatorImpl() = default;
DiscardableMemoryAllocatorImpl::~DiscardableMemoryAllocatorImpl() = default;

DiscardableMemoryAllocator& DiscardableMemoryAllocatorImpl::GetInstance() {
  static fml::NoDestructor<DiscardableMemoryAllocatorImpl> instance;
  return *instance;
}

std::unique_ptr<DiscardableMemory>
DiscardableMemoryAllocatorImpl::AllocateLockedDiscardableMemory(size_t size) {
#if OS_ANDROID || OS_MAC || OS_WIN
  return std::make_unique<DiscardableMemoryImpl>(size, &bytes_allocated_);
#else
  return std::make_unique<HeapDiscardableMemory>(size);
#endif
}

size_t DiscardableMemoryAllocatorImpl::GetBytesAllocated() const {
  return bytes_allocated_;
}

}  // namespace clay
