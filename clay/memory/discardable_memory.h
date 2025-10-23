// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_DISCARDABLE_MEMORY_H_
#define CLAY_MEMORY_DISCARDABLE_MEMORY_H_

#include <cstddef>

#include "base/include/fml/macros.h"
#include "base/include/fml/time/time_point.h"

namespace clay {

class DiscardableMemory {
 public:
  DiscardableMemory() = default;
  virtual ~DiscardableMemory() = default;

  // Locks the memory so that it will not be purged by the system. Returns
  // true on success. If the return value is false then this object should be
  // destroyed and a new one should be created.
  [[nodiscard]] virtual bool Lock() = 0;

  // Unlocks the memory so that it can be purged by the system. Must be called
  // after every successful lock call.
  virtual void Unlock() = 0;

  // Returns the memory address held by this object. The object must be locked
  // before calling this.
  virtual void* data() const = 0;

  virtual bool isLocked() const = 0;

  virtual size_t GetSize() const = 0;

  // Handy method to simplify calling data() with a reinterpret_cast.
  template <typename T>
  T* data_as() const {
    return reinterpret_cast<T*>(data());
  }

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(DiscardableMemory);
};

}  // namespace clay
#endif  // CLAY_MEMORY_DISCARDABLE_MEMORY_H_
