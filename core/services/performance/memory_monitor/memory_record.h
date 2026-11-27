// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_RECORD_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_RECORD_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace lynx {
namespace tasm {
namespace performance {

using MemoryCategory = std::string;

inline constexpr char kMemoryEntryType[] = "memory";
inline constexpr char kCategory[] = "category";
inline constexpr char kSizeBytes[] = "sizeBytes";
inline constexpr char kInstanceCount[] = "instanceCount";
inline constexpr char kDetail[] = "detail";

// Memory types
// Main thread scripting engine. Currently covers only LepusNG.
inline constexpr char kCategoryMTSEngine[] = "mainThreadScriptingEngine";
// Background thread scripting engine. Currently covers only QuickJS.
inline constexpr char kCategoryBTSEngine[] = "backgroundThreadScriptingEngine";
inline constexpr char kCategoryTasmElement[] = "lynxTasmElement";

// Reserved categories framing a complete UI snapshot. Submit both sentinels
// and all records in one reporter-thread task. Keep in sync with Android's
// PerformanceController.
inline constexpr char kUIMemorySnapshotBegin[] =
    "__lynx_ui_memory_snapshot_begin__";
inline constexpr char kUIMemorySnapshotEnd[] =
    "__lynx_ui_memory_snapshot_end__";

// Record memory information for a specific module
struct MemoryRecord {
  // Memory type, required attribute.
  // For example, kMemoryCategoryMTSEngine, kMemoryCategoryBTSEngine, and
  // kMemoryCategoryTasmElement, etc.
  MemoryCategory category_;

  // Memory size in bytes.
  int64_t size_bytes_ = 0;

  // The number of instances of the category, default is one.
  int32_t instance_count_ = 1;

  // Set by the platform UI collector when constructing the record.
  bool is_ui_ = false;

  // Detailed description of the memory information, optional attribute.
  // For example, when category_ is kMemoryCategoryImage, detail_ can include
  // image URL information.
  std::unique_ptr<std::unordered_map<std::string, std::string>> detail_;

  // @brief Adds `other` to this MemoryRecord, updating `size_bytes_` and
  // merging `detail_`. `size_bytes_` is always incremented. `detail_` is
  // merged; `other`'s values overwrite existing keys.
  MemoryRecord& operator+=(const MemoryRecord& other);

  // Subtracts `other` from this MemoryRecord, updating `size_bytes_` and
  // removing keys from `detail_`. `size_bytes_` is decremented. Keys in
  // `other.detail_` are removed from `detail_`. Null `detail_` are handled.
  MemoryRecord& operator-=(const MemoryRecord& other);

  explicit MemoryRecord(MemoryCategory category, int64_t size_bytes = 0);
  MemoryRecord(
      MemoryCategory category, int64_t size_bytes,
      std::unique_ptr<std::unordered_map<std::string, std::string>> detail);
  MemoryRecord(
      MemoryCategory category, int64_t size_bytes, int32_t instance_count,
      std::unique_ptr<std::unordered_map<std::string, std::string>> detail,
      bool is_ui = false);
  MemoryRecord() = default;
  ~MemoryRecord() = default;

  MemoryRecord(MemoryRecord&) = delete;
  MemoryRecord(const MemoryRecord&) = delete;
  MemoryRecord& operator=(const MemoryRecord&) = delete;
  MemoryRecord(MemoryRecord&&) = default;
  MemoryRecord& operator=(MemoryRecord&&) = default;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_RECORD_H_
