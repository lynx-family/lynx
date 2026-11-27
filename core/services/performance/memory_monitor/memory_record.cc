// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/memory_record.h"

namespace lynx {
namespace tasm {
namespace performance {

MemoryRecord::MemoryRecord(MemoryCategory category, int64_t size_bytes)
    : category_(std::move(category)), size_bytes_(size_bytes) {}

MemoryRecord::MemoryRecord(
    MemoryCategory category, int64_t size_bytes,
    std::unique_ptr<std::unordered_map<std::string, std::string>> detail)
    : category_(std::move(category)),
      size_bytes_(size_bytes),
      detail_(std::move(detail)) {}

MemoryRecord::MemoryRecord(
    MemoryCategory category, int64_t size_bytes, int32_t instance_count,
    std::unique_ptr<std::unordered_map<std::string, std::string>> detail,
    bool is_ui)
    : category_(std::move(category)),
      size_bytes_(size_bytes),
      instance_count_(instance_count),
      is_ui_(is_ui),
      detail_(std::move(detail)) {}

MemoryRecord& MemoryRecord::operator+=(const MemoryRecord& other) {
  size_bytes_ += other.size_bytes_;
  instance_count_ += other.instance_count_;
  // Merge detail_
  if (!other.detail_) {
    return *this;
  }
  if (!detail_) {
    // If we have no detail, create a copy of other's
    detail_ = std::make_unique<std::unordered_map<std::string, std::string>>(
        *other.detail_);
    return *this;
  }
  for (const auto& [key, value] : *other.detail_) {
    (*detail_)[key] = value;
  }
  return *this;
}

MemoryRecord& MemoryRecord::operator-=(const MemoryRecord& other) {
  size_bytes_ -= other.size_bytes_;
  instance_count_ -= other.instance_count_;
  // Deduplicate detail_
  if (!other.detail_ || !detail_) {
    return *this;
  }
  for (const auto& [key, value] : *other.detail_) {
    detail_->erase(key);
  }
  return *this;
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
