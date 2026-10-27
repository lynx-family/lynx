// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_RESOURCE_CACHE_LIMIT_CALCULATOR_H_
#define CLAY_SHELL_COMMON_RESOURCE_CACHE_LIMIT_CALCULATOR_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"

namespace clay {
class ResourceCacheLimitItem {
 public:
  // The expected GPU resource cache limit in bytes. This will be called on the
  // platform thread.
  virtual size_t GetResourceCacheLimit() = 0;

 protected:
  virtual ~ResourceCacheLimitItem() = default;
};

class ResourceCacheLimitCalculator {
 public:
  explicit ResourceCacheLimitCalculator(size_t max_bytes_threshold)
      : max_bytes_threshold_(max_bytes_threshold) {}

  ~ResourceCacheLimitCalculator() = default;

  // This will be called on the platform thread.
  void AddResourceCacheLimitItem(fml::WeakPtr<ResourceCacheLimitItem> item) {
    items_.push_back(item);
  }

  // The maximum GPU resource cache limit in bytes calculated by
  // 'ResourceCacheLimitItem's. This will be called on the platform thread.
  size_t GetResourceCacheMaxBytes();

 private:
  std::vector<fml::WeakPtr<ResourceCacheLimitItem>> items_;
  size_t max_bytes_threshold_;
  BASE_DISALLOW_COPY_AND_ASSIGN(ResourceCacheLimitCalculator);
};
}  // namespace clay

#endif  // CLAY_SHELL_COMMON_RESOURCE_CACHE_LIMIT_CALCULATOR_H_
