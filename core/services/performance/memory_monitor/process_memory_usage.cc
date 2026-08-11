// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/process_memory_usage.h"

#include <limits>
#include <sstream>
#include <string>

#if defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__OHOS_FAMILY__)
#include <hidebug/hidebug.h>
#elif defined(__linux__)
#include <fstream>
#elif defined(_WIN32)
// psapi.h requires Windows types before its declarations.
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#endif

namespace lynx {
namespace tasm {
namespace performance {

namespace internal {

// Sum Pss lines; missing/invalid data or arithmetic overflow returns -1.
int64_t ReadPssBytes(std::istream& input) {
  int64_t kilobytes = 0;
  bool found = false;
  std::string line;
  while (std::getline(input, line)) {
    // Pss_Anon/Pss_File/Pss_Shmem are breakdowns, not additional PSS.
    if (line.compare(0, 4, "Pss:") != 0) continue;
    std::istringstream fields(line.substr(4));
    int64_t value;
    std::string unit, extra;
    if (!(fields >> value >> unit) || unit != "kB" || value < 0 ||
        (fields >> extra) ||
        value > (std::numeric_limits<int64_t>::max)() / 1024 - kilobytes) {
      return -1;
    }
    kilobytes += value;
    found = true;
  }
  return found && input.eof() && !input.bad() ? kilobytes * 1024 : -1;
}

}  // namespace internal

int64_t ReadProcessPssBytes() {
#if defined(__APPLE__)
  task_vm_info_data_t info{};
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_VM_INFO,
                reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS &&
      count >= TASK_VM_INFO_REV1_COUNT) {
    if (info.phys_footprint <=
        static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
      return static_cast<int64_t>(info.phys_footprint);
    }
  }
#elif defined(__OHOS_FAMILY__)
  // API 12, libohhidebug. Keep this before __linux__: OHOS defines both.
  // No 5-minute cache: a stale denominator cannot represent this event.
  // HiDebug returns an empty struct on failure and reports values in KiB.
  HiDebug_NativeMemInfo info{};
  OH_HiDebug_GetAppNativeMemInfo(&info);
  if (info.pss > 0) return static_cast<int64_t>(info.pss) * 1024;
#elif defined(__linux__)
  // Android also takes this path. Prefer the rollup to parsing every mapping;
  // older kernels lack it, so fall back to smaps without changing the metric.
  std::ifstream rollup("/proc/self/smaps_rollup");
  const auto pss = internal::ReadPssBytes(rollup);
  if (pss >= 0) return pss;
  std::ifstream smaps("/proc/self/smaps");
  return internal::ReadPssBytes(smaps);
#elif defined(_WIN32)
  PROCESS_MEMORY_COUNTERS info;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
    return static_cast<int64_t>(info.WorkingSetSize);
  }
#endif
  return -1;
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
