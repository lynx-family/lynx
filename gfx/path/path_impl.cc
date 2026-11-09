// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path_impl.h"

#include <atomic>
#include <cstdlib>
#include <utility>
namespace lynx::gfx {
namespace {
uint64_t NextPathKey() {
  // Keep the counter in BSS while publishing keys starting at one.
  static std::atomic<uint64_t> next{0};
  auto key = next.fetch_add(1, std::memory_order_relaxed) + 1;
  if (key == 0) std::abort();
  return key;
}
}  // namespace
PathImpl::PathImpl(PathSource source)
    : key_(NextPathKey()), source_(std::move(source)) {}
PathImpl::~PathImpl() = default;
}  // namespace lynx::gfx
