// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <sys/mman.h>
#include <unistd.h>

#include "clay/fml/logging.h"
#include "clay/memory/discardable_memory_impl.h"

#if OS_ANDROID
#include <sys/prctl.h>
#endif

namespace {

constexpr intptr_t kPageMagicCookie = 1;

size_t GetPageSize() {
#if defined(_SC_PAGESIZE)
  return sysconf(_SC_PAGESIZE);
#else
  return getpagesize();
#endif
}

void* AllocatePages(size_t size_in_pages) {
  const size_t length = size_in_pages * GetPageSize();
  void* data = mmap(nullptr, length, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  FML_DCHECK(data != MAP_FAILED);

#if OS_ANDROID
  prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, data, length,
        "madv-free-discardable");
#endif
  return data;
}
}  // namespace

namespace clay {

DiscardableMemoryImpl::DiscardableMemoryImpl(
    size_t size_in_bytes, std::atomic<size_t>* allocator_byte_count)
    : size_in_bytes_(size_in_bytes),
      allocated_pages_((size_in_bytes_ + GetPageSize() - 1) / GetPageSize()),
      allocator_byte_count_(allocator_byte_count),
      page_first_word_((size_in_bytes_ + GetPageSize() - 1) / GetPageSize()) {
  data_ = AllocatePages(allocated_pages_);
  (*allocator_byte_count_) += size_in_bytes_;
}
DiscardableMemoryImpl::~DiscardableMemoryImpl() { Deallocate(); }

bool DiscardableMemoryImpl::Lock() {
  FML_DCHECK(!is_locked_);
  // Locking fails if the memory has been deallocated.
  if (!data_) {
    return false;
  }

  size_t page_index;
  for (page_index = 0; page_index < allocated_pages_; ++page_index) {
    if (!LockPage(page_index)) break;
  }

  if (page_index < allocated_pages_) {
    FML_LOG(ERROR) << "Region eviction discovered during lock with "
                   << allocated_pages_ << " pages";
    Deallocate();
    return false;
  }
  FML_DCHECK(IsResident());

  is_locked_ = true;
  return true;
}

void DiscardableMemoryImpl::Unlock() {
  if (!is_locked_) {
    return;
  }
  FML_DCHECK(data_ != nullptr);

  for (size_t page_index = 0; page_index < allocated_pages_; ++page_index) {
    UnlockPage(page_index);
  }

#ifdef MADV_FREE
#define MADV_ARGUMENT MADV_FREE
#else
#define MADV_ARGUMENT MADV_DONTNEED
#endif
  int retval = madvise(data_, allocated_pages_ * GetPageSize(), MADV_ARGUMENT);
  FML_DCHECK(!retval);
  is_locked_ = false;
}

void* DiscardableMemoryImpl::data() const {
  FML_DCHECK(is_locked_);
  FML_DCHECK(data_ != nullptr);

  return data_;
}

bool DiscardableMemoryImpl::LockPage(size_t page_index) {
  // We require the byte-level representation of std::atomic<intptr_t> to be
  // equivalent to that of an intptr_t. Since std::atomic<intptr_t> has standard
  // layout, having equal size is sufficient but not necessary for them to have
  // the same byte-level representation.
  static_assert(sizeof(intptr_t) == sizeof(std::atomic<intptr_t>),
                "Incompatible layout of std::atomic.");
  FML_DCHECK(std::atomic<intptr_t>{}.is_lock_free());
  std::atomic<intptr_t>* page_as_atomic =
      reinterpret_cast<std::atomic<intptr_t>*>(static_cast<uint8_t*>(data_) +
                                               page_index * GetPageSize());

  intptr_t expected = kPageMagicCookie;

  // Recall that we set the first word of the page to |kPageMagicCookie|
  // (non-zero) during unlocking. Thus, if the value has changed, the page has
  // been discarded. Restore the page's original first word from before
  // unlocking only if the page has not been discarded.
  if (!std::atomic_compare_exchange_strong_explicit(
          page_as_atomic, &expected,
          static_cast<intptr_t>(page_first_word_[page_index]),
          std::memory_order_relaxed, std::memory_order_relaxed)) {
    return false;
  }

  return true;
}

void DiscardableMemoryImpl::UnlockPage(size_t page_index) {
  FML_DCHECK(std::atomic<intptr_t>{}.is_lock_free());

  std::atomic<intptr_t>* page_as_atomic =
      reinterpret_cast<std::atomic<intptr_t>*>(static_cast<uint8_t*>(data_) +
                                               page_index * GetPageSize());

  // Store the first word of the page for use during unlocking.
  page_first_word_[page_index].store(*page_as_atomic,
                                     std::memory_order_relaxed);
  // Store a non-zero value into the first word of the page, so we can tell when
  // the page is discarded during locking.
  page_as_atomic->store(kPageMagicCookie, std::memory_order_relaxed);
}

bool DiscardableMemoryImpl::IsValid() const { return data_ != nullptr; }

bool DiscardableMemoryImpl::IsResident() const {
#if OS_MAC
  std::vector<char> vec(allocated_pages_);
#elif OS_ANDROID
  std::vector<unsigned char> vec(allocated_pages_);
#endif

  int retval = mincore(data_, allocated_pages_ * GetPageSize(), vec.data());
  FML_DCHECK(retval == 0 || errno == EAGAIN);

  for (size_t i = 0; i < allocated_pages_; ++i) {
    if (!(vec[i] & 1)) return false;
  }
  return true;
}

bool DiscardableMemoryImpl::IsDiscarded() const {
  return !is_locked_ && !IsResident();
}

bool DiscardableMemoryImpl::Deallocate() {
  if (data_) {
    int retval = munmap(data_, allocated_pages_ * GetPageSize());
    FML_DCHECK(!retval);
    data_ = nullptr;
    (*allocator_byte_count_) -= size_in_bytes_;
    return true;
  }
  return false;
}

}  // namespace clay
