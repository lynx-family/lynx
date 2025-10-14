// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>
#include <unwind.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "base/include/string/string_utils.h"
#include "clay/fml/backtrace.h"
#include "clay/fml/logging.h"
#include "clay/fml/platform/linux/proc_maps_linux.h"

#ifdef __LP64__
#define FMT_ADDR "0x%016lx"
#else
#define FMT_ADDR "0x%08x"
#endif

namespace fml {

namespace {

template <typename... Args>
std::string StringFormat(const std::string& format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
               1;  // Extra space for '\0'
  if (size_s <= 0) {
    return "";
  }
  auto size = static_cast<size_t>(size_s);
  auto buf = std::make_unique<char[]>(size);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1);  // We don't want the '\0' inside
}

struct StackCrawlState {
  StackCrawlState(uintptr_t* frames, size_t max_depth)
      : frames(frames),
        frame_count(0),
        max_depth(max_depth),
        have_skipped_self(false) {}

  uintptr_t* frames;
  size_t frame_count;
  size_t max_depth;
  bool have_skipped_self;
};

_Unwind_Reason_Code TraceStackFrame(_Unwind_Context* context, void* arg) {
  StackCrawlState* state = static_cast<StackCrawlState*>(arg);
  uintptr_t ip = _Unwind_GetIP(context);

  // The first stack frame is this function itself.  Skip it.
  if (ip != 0 && !state->have_skipped_self) {
    state->have_skipped_self = true;
    return _URC_NO_REASON;
  }

  state->frames[state->frame_count++] = ip;
  if (state->frame_count >= state->max_depth) {
    return _URC_END_OF_STACK;
  }
  return _URC_NO_REASON;
}

size_t CollectStackTrace(void** trace, size_t count) {
  StackCrawlState state(reinterpret_cast<uintptr_t*>(trace), count);
  _Unwind_Backtrace(&TraceStackFrame, &state);
  return state.frame_count;
}

void DumpBacktrace(std::ostream* os, void** trace, size_t count) {
  std::string proc_maps;
  std::vector<MappedMemoryRegion> regions;

  if (!ReadProcMaps(&proc_maps)) {
    FML_LOG(ERROR) << "Failed to read /proc/self/maps";
  } else if (!ParseProcMaps(proc_maps, &regions)) {
    FML_LOG(ERROR) << "Failed to parse /proc/self/maps";
  }

  *os << "\n";
  for (size_t i = 0; i < count; ++i) {
    // Subtract one as return address of function may be in the next
    // function when a function is annotated as noreturn.
    uintptr_t address = reinterpret_cast<uintptr_t>(trace[i]) - 1;

    std::vector<MappedMemoryRegion>::iterator iter = regions.begin();
    while (iter != regions.end()) {
      if (address >= iter->start && address < iter->end &&
          !iter->path.empty()) {
        break;
      }
      ++iter;
    }

    const char* symbol = "";
    Dl_info info;
    if (dladdr(reinterpret_cast<const void*>(address), &info) &&
        info.dli_sname) {
      symbol = info.dli_sname;
    }

    // Adjust absolute address to be an offset within the mapped region, to
    // match the format dumped by Android's crash output.
    unsigned long relative_address = reinterpret_cast<char*>(address) -
                                     reinterpret_cast<char*>(info.dli_fbase);
    // The format below intentionally matches that of Android's debuggerd
    // output. This simplifies decoding by scripts such as stack.py.
    *os << StringFormat("#%02zd pc ", i);
    *os << StringFormat(FMT_ADDR " ", relative_address);

    if (iter != regions.end()) {
      *os << StringFormat("%s", iter->path.c_str());
      if (lynx::base::EndsWith(iter->path, ".apk")) {
        *os << StringFormat(" (offset 0x%llx)", iter->offset);
      } else if (info.dli_sname) {
        *os << StringFormat(" (%s)", symbol);
      }
    } else {
      *os << "<Unknown>";
    }

    *os << "\n";
  }
}

}  // namespace

std::string BacktraceHere(size_t offset) {
  static constexpr size_t kMaxTraces = 62;
  void* trace[kMaxTraces];
  memset(trace, 0, kMaxTraces * sizeof(trace[0]));

  // The number of valid frames in |trace|
  size_t count = CollectStackTrace(trace, std::min(offset, kMaxTraces));
  std::stringstream stream;
  DumpBacktrace(&stream, trace, count);
  return stream.str();
}

void InstallCrashHandler() {
  // Not supported.
}

bool IsCrashHandlingSupported() { return false; }

}  // namespace fml
