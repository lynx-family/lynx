// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/memory/process_memory_info.h"

#include <limits>

#include "build/build_config.h"

#if defined(OS_ANDROID)
#include "base/include/platform/android/process_memory_info.h"
#elif defined(OS_IOS) || defined(OS_OSX) || defined(OS_MACOSX)
#include <mach/mach.h>
#elif defined(OS_HARMONY)
#include <hidebug/hidebug.h>
#elif defined(OS_LINUX)
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#elif defined(OS_WIN)
// psapi.h requires Windows types before its declarations.
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#endif

namespace lynx {
namespace base {
namespace {

#if defined(OS_ANDROID) || defined(OS_HARMONY) || defined(OS_LINUX)
constexpr int64_t kBytesPerKilobyte = 1024;
#endif

#if defined(OS_LINUX)
int64_t ReadPssBytes(const char* path) {
  std::FILE* file = std::fopen(path, "r");
  if (file == nullptr) {
    return -1;
  }

  int64_t kilobytes = 0;
  bool found = false;
  char line[256];
  while (std::fgets(line, sizeof(line), file) != nullptr) {
    if (std::strncmp(line, "Pss:", 4) != 0) {
      continue;
    }

    const char* value_start = line + 4;
    char* value_end = nullptr;
    errno = 0;
    const long long value = std::strtoll(value_start, &value_end, 10);
    if (value_start == value_end || errno == ERANGE || value < 0) {
      std::fclose(file);
      return -1;
    }
    while (std::isspace(static_cast<unsigned char>(*value_end))) {
      ++value_end;
    }
    if (value_end[0] != 'k' || value_end[1] != 'B') {
      std::fclose(file);
      return -1;
    }

    value_end += 2;
    while (std::isspace(static_cast<unsigned char>(*value_end))) {
      ++value_end;
    }
    if (*value_end != '\0' ||
        value > (std::numeric_limits<int64_t>::max)() / kBytesPerKilobyte -
                    kilobytes) {
      std::fclose(file);
      return -1;
    }

    kilobytes += value;
    found = true;
  }

  const bool read_success = std::feof(file) != 0;
  std::fclose(file);
  return found && read_success ? kilobytes * kBytesPerKilobyte : -1;
}
#endif

}  // namespace

BASE_EXTERN_C int64_t GetProcessPssBytes() {
#if defined(OS_ANDROID)
  const auto stats = android::GetProcessMemoryInfo();
  for (size_t i = 0; i + 1 < stats.size(); i += 2) {
    if (stats[i] == "summary.total-pss") {
      return static_cast<int64_t>(std::stoull(stats[i + 1])) *
             kBytesPerKilobyte;
    }
  }
#elif defined(OS_IOS) || defined(OS_OSX) || defined(OS_MACOSX)
  task_vm_info_data_t info{};
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_VM_INFO,
                reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS &&
      count >= TASK_VM_INFO_REV1_COUNT &&
      info.phys_footprint <=
          static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
    return static_cast<int64_t>(info.phys_footprint);
  }
#elif defined(OS_HARMONY)
  HiDebug_NativeMemInfo info{};
  OH_HiDebug_GetAppNativeMemInfo(&info);
  if (info.pss > 0) {
    return static_cast<int64_t>(info.pss) * kBytesPerKilobyte;
  }
#elif defined(OS_LINUX)
  const int64_t rollup = ReadPssBytes("/proc/self/smaps_rollup");
  if (rollup >= 0) {
    return rollup;
  }
  return ReadPssBytes("/proc/self/smaps");
#elif defined(OS_WIN)
  PROCESS_MEMORY_COUNTERS info{};
  if (::GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
    return static_cast<int64_t>(info.WorkingSetSize);
  }
#endif
  return -1;
}

}  // namespace base
}  // namespace lynx
