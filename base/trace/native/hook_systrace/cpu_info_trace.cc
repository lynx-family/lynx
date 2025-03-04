// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/hook_systrace/cpu_info_trace.h"

#include <string>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"

#if OS_ANDROID
#include <unistd.h>

#include <fstream>
#endif

#if OS_IOS
#include <mach/mach_time.h>
#include <mach/machine.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace lynx {
namespace trace {

namespace {
#if OS_IOS
#if defined(__arm64__)
uint64_t GetSystemBootTimeSecond() {
  auto init_time_factor = []() -> uint64_t {
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    return timebase_info.numer / timebase_info.denom;
  };
  static uint64_t monotonic_timebase_factor = init_time_factor();

  return std::chrono::nanoseconds(mach_absolute_time() *
                                  monotonic_timebase_factor)
      .count();
}
#endif

float GetCpuCurFreq() {
#if defined(__arm64__)
  // 10000 loop times is a balance between time-cost and accuracy
  int count = 10000;
  uint64_t start_time = GetSystemBootTimeSecond();
  /**
   * approximate calculating iOS cpu frequency.
   * To reduce instruction-level concurrency, we use 24 general-purpose
   * registers and every add instrument's input is the output of the previous
   * instruction. next assembly codes executes for approximately 240000 clock
   * cycles.
   */
  asm volatile(
      "0:"
      "add     x2,  x2,  x1  \n"
      "add     x3,  x3,  x2  \n"
      "add     x4,  x4,  x3  \n"
      "add     x5,  x5,  x4  \n"
      "add     x6,  x6,  x5  \n"
      "add     x7,  x7,  x6  \n"
      "add     x9,  x9,  x7  \n"
      "add     x10, x10, x9  \n"
      "add     x11, x11, x10 \n"
      "add     x12, x12, x11 \n"
      "add     x13, x13, x12 \n"
      "add     x14, x14, x13 \n"
      "add     x15, x15, x14 \n"
      "add     x19, x19, x15 \n"
      "add     x20, x20, x19 \n"
      "add     x21, x21, x20 \n"
      "add     x22, x22, x21 \n"
      "add     x23, x23, x22 \n"
      "add     x24, x24, x23 \n"
      "add     x25, x25, x24 \n"
      "add     x26, x26, x25 \n"
      "add     x27, x27, x26 \n"
      "add     x28, x28, x27 \n"
      "add     x1, x1, x28 \n"
      "subs    %x0, %x0, #1  \n"
      "bne     0b            \n"
      : "=r"(count)
      : "0"(count)
      : "cc", "memory", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10",
        "x11", "x12", "x13", "x14", "x15", "x19", "x20", "x21", "x22", "x23",
        "x24", "x25", "x26", "x27", "x28");
  uint64_t cost_time = GetSystemBootTimeSecond() - start_time;
  // clock cycle counts divide by cost time
  float freq = 240000.0 / cost_time;
  return freq;
#endif
  return 0.0;
}

void ReadCpuFreqs(std::vector<CpuInfoTrace::CpuFreq>& cpu_freqs) {
  const float cur_freq = GetCpuCurFreq();
  cpu_freqs.emplace_back(CpuInfoTrace::CpuFreq{0, cur_freq});
}
#elif OS_ANDROID
void ReadCpuFreqs(std::vector<CpuInfoTrace::CpuFreq>& cpu_freqs) {
  static const std::string cpu_dir_path = "/sys/devices/system/cpu";
  static const std::string cpu_file_path = "/cpufreq/scaling_cur_freq";
  auto num_cpus = static_cast<size_t>(sysconf(_SC_NPROCESSORS_CONF));
  for (size_t i = 0; i < num_cpus; i++) {
    std::string cpu_file =
        cpu_dir_path + "/cpu" + std::to_string(i) + cpu_file_path;
    std::ifstream fin(cpu_file);
    if (!fin.is_open()) {
      cpu_freqs.emplace_back(CpuInfoTrace::CpuFreq{i, 0.0});
      continue;
    }
    std::string str;
    getline(fin, str);
    float freq = std::stoul(str) / 1000 / 1000.0;  // Ghz
    cpu_freqs.emplace_back(CpuInfoTrace::CpuFreq{i, freq});
    fin.close();
  }
}
#else
void ReadCpuFreqs(std::vector<CpuInfoTrace::CpuFreq>&) {}
#endif
}  // namespace

CpuInfoTrace::CpuInfoTrace() : thread_("cpu_freq_thread") {}

void CpuInfoTrace::DispatchBegin() {
  // 16ms is a balance between time-cost and accuracy.
  const static uint32_t sDelayTimeForCpuFreqTrace = 16;  // Ms
  auto record_cpu_freq_task = [&] {
    const std::vector<CpuInfoTrace::CpuFreq> cpu_freqs = this->ReadCpuCurFreq();
    for (auto cpu_freq : cpu_freqs) {
      const std::string track_name =
          std::string("cpu") + std::to_string(cpu_freq.first);
      TRACE_COUNTER(INTERNAL_TRACE_CATEGORY_VITALS, track_name.c_str(),
                    cpu_freq.second);
    }
  };

  thread_.GetTaskRunner()->PostTask([this, record_cpu_freq_task] {
    timer_ = std::make_unique<lynx::base::TimedTaskManager>();
    timer_->SetInterval(std::move(record_cpu_freq_task),
                        sDelayTimeForCpuFreqTrace);
  });
}

void CpuInfoTrace::DispatchEnd() {
  if (timer_ != nullptr) {
    thread_.GetTaskRunner()->PostTask([&] { timer_.reset(nullptr); });
  }
}

const std::vector<CpuInfoTrace::CpuFreq> CpuInfoTrace::ReadCpuCurFreq() {
  std::vector<CpuInfoTrace::CpuFreq> cpu_freqs;
  ReadCpuFreqs(cpu_freqs);
  return cpu_freqs;
}

}  // namespace trace
}  // namespace lynx
