// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/resource_cache_limit_calculator.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace clay {

size_t ResourceCacheLimitCalculator::GetResourceCacheMaxBytes() {
  size_t max_bytes = 0;
  size_t max_bytes_threshold = max_bytes_threshold_ > 0
                                   ? max_bytes_threshold_
                                   : std::numeric_limits<size_t>::max();
  std::vector<fml::WeakPtr<ResourceCacheLimitItem>> live_items;
  for (auto item : items_) {
    if (item) {
      live_items.push_back(item);
      max_bytes += item->GetResourceCacheLimit();
    }
  }
  items_ = std::move(live_items);
  return std::min(max_bytes, max_bytes_threshold);
}

}  // namespace clay
