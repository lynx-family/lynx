// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_

#include <string>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/memory_monitor/memory_record.h"
#include "core/services/performance/performance_event_sender.h"

namespace lynx {
namespace tasm {
namespace performance {

inline constexpr char kRuntimeId[] = "runtimeId";
inline constexpr char kRuntimeGroupId[] = "groupId";
inline constexpr char kRawRuntimeMemoryInfo[] = "raw_memory_info_json_str";

// @class MemoryMonitor
// @brief A class for monitoring memory usage and managing memory records.
//
// The MemoryMonitor class provides functionality to allocate, deallocate,
// and update memory usage records. It maintains a mapping of categories to
// their respective MemoryRecord instances, allowing for efficient tracking
// of memory utilization.
class MemoryMonitor {
  friend class PerformanceController;

 public:
  // Increments memory usage and sends a PerformanceEntry.
  // This interface will increase the total memory usage for the category found
  // in the record.
  void AllocateMemory(MemoryRecord&& record);

  // Decrements memory usage and sends a PerformanceEntry.
  // This interface will decrease the total memory usage for the category found
  // in the record.
  void DeallocateMemory(MemoryRecord&& record);

  // Overwrites the memory usage and sends a PerformanceEntry.
  // This interface will overwrite the record corresponding to the category in
  // the record, effectively updating the memory usage information.
  void UpdateMemoryUsage(MemoryRecord&& record);

  // Overwrites the memory usage and sends a PerformanceEntry.
  // This interface will overwrite the record corresponding to the category in
  // the record, effectively updating the memory usage information.
  void UpdateScriptingEngineMemoryUsage(
      std::unordered_map<std::string, std::string> info);

  // Checks if memory monitoring is enabled.
  // Modules can call this before collecting data to avoid unnecessary
  // collection.
  static bool Enable();

  // Forces memory monitoring to be enabled or disabled, overriding other
  // settings.
  static void SetForceEnable(bool force_enable);

  // The threshold for memory increase and decrease that triggers collection, in
  // MB. This is configured through Settings.
  static uint32_t MemoryChangeThresholdMb();

  /// @brief Generates a bitmask for scripting engine memory monitoring
  /// configuration This method combines memory monitoring status and memory
  /// increment threshold into a uint32_t bitmask:
  /// - Bits 0-8      : Memory increment threshold in MB (capped at 100MB)
  /// @return uint32_t Combined configuration bitmask
  static uint32_t ScriptingEngineMode();

  explicit MemoryMonitor(PerformanceEventSender* observer,
                         int32_t instance_id = report::kUninitializedInstanceId)
      : instance_id_(instance_id), sender_(observer) {
    LOGI("[memory_monitor.h] new MemoryMonitor, this:"
         << this << ", Enable:" << Enable()
         << ", MemoryChangeThresholdMb:" << MemoryChangeThresholdMb());
  };
  ~MemoryMonitor();
  MemoryMonitor(const MemoryMonitor& timing) = delete;
  MemoryMonitor& operator=(const MemoryMonitor&) = delete;
  MemoryMonitor(MemoryMonitor&& other) = delete;
  MemoryMonitor& operator=(MemoryMonitor&& other) = delete;

 private:
  void ReportMemory();
  int32_t instance_id_ = report::kUninitializedInstanceId;
  PerformanceEventSender* sender_;
  std::unordered_map<MemoryCategory, MemoryRecord> memory_records_;
  int64_t last_reported_size_bytes_ = 0;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_
