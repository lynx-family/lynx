// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/memory/system_memory_pressure_evaluator_mac.h"

#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_host.h>
#include <mach/vm_statistics.h>
#include <sys/sysctl.h>

#include "base/include/fml/message_loop.h"
#include "clay/fml/logging.h"

namespace clay {

namespace {
MemoryPressureListener::MemoryPressureLevel MapFromMacPressureLevel(
    dispatch_source_memorypressure_flags_t mac_pressure_level) {
  switch (mac_pressure_level) {
    case DISPATCH_MEMORYPRESSURE_NORMAL:
      return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
    case DISPATCH_MEMORYPRESSURE_WARN:
      return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;
    case DISPATCH_MEMORYPRESSURE_CRITICAL:
      return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
  }
  return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
}
}  // namespace

const float SystemMemoryPressureEvaluatorMac::kCriticalPressureThreshold = 0.9;
const float SystemMemoryPressureEvaluatorMac::kModeratePressureThreshold = 0.75;

const fml::TimeDelta SystemMemoryPressureEvaluatorMac::kReCheckMemoryPeriod =
    fml::TimeDelta::FromSeconds(5);

SystemMemoryPressureEvaluatorMac::SystemMemoryPressureEvaluatorMac() {
  memory_event_source_ = dispatch_source_create(
      DISPATCH_SOURCE_TYPE_MEMORYPRESSURE, 0,
      DISPATCH_MEMORYPRESSURE_WARN | DISPATCH_MEMORYPRESSURE_CRITICAL |
          DISPATCH_MEMORYPRESSURE_NORMAL,
      dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0));

  fml::MessageLoop::EnsureInitializedForCurrentThread();
  task_runner_ = fml::MessageLoop::GetCurrent().GetTaskRunner();

  StartObserving();
}

SystemMemoryPressureEvaluatorMac::~SystemMemoryPressureEvaluatorMac() {
  StopObserving();
}

MemoryPressureListener::MemoryPressureLevel
SystemMemoryPressureEvaluatorMac::MapPressureLevelForTest(
    dispatch_source_memorypressure_flags_t mac_level) {
  return MapFromMacPressureLevel(mac_level);
}

void SystemMemoryPressureEvaluatorMac::StartObserving() {
  FML_DCHECK(memory_event_source_);
  dispatch_source_set_event_handler(memory_event_source_, ^{
    dispatch_source_memorypressure_flags_t mac_pressure_level =
        dispatch_source_get_data(memory_event_source_);
    MemoryPressureLevel pressure_level =
        MapFromMacPressureLevel(mac_pressure_level);
    task_runner_->PostTask([this, pressure_level]() {
      SetCurrentPressureLevel(pressure_level);
      OnMemoryPressureChanged();
    });
  });

  dispatch_resume(memory_event_source_);
}

void SystemMemoryPressureEvaluatorMac::StopObserving() {
  // Remove the memory pressure event source.
  if (memory_event_source_) {
    dispatch_source_cancel(memory_event_source_);
  }
}

void SystemMemoryPressureEvaluatorMac::OnMemoryPressureChanged() {
  auto current_level = GetCurrentPressureLevel();
  bool notify =
      current_level != MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE;
  if (notify) {
    MemoryPressureListener::NotifyMemoryPressure(current_level);
  }
  // We need to recheck to assure that the pressure level is no longer
  // critical.
  if (current_level == MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL) {
    task_runner_->PostDelayedTask([this]() { CheckMemoryPressure(); },
                                  kReCheckMemoryPeriod);
  }
}

void SystemMemoryPressureEvaluatorMac::CheckMemoryPressure() {
  int current_level;
  size_t length = sizeof(int);
  int error = sysctlbyname("kern.memorystatus_vm_pressure_level",
                           &current_level, &length, nullptr, 0);
  if (error != 0) {
    FML_LOG(ERROR) << "sysctl kern.memorystatus_vm_pressure_level failed!";
    if (!CalculatePressureLevelByAvailableMemory(&current_level)) {
      return;
    }
  }

  SetCurrentPressureLevel(MapFromMacPressureLevel(current_level));

  OnMemoryPressureChanged();
}

bool SystemMemoryPressureEvaluatorMac::CalculatePressureLevelByAvailableMemory(
    int* pressure_level) {
  vm_statistics64_data_t vm_stats;
  mach_msg_type_number_t infoCount = HOST_VM_INFO64_COUNT;
  if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                        reinterpret_cast<host_info64_t>(&vm_stats),
                        &infoCount) != KERN_SUCCESS) {
    FML_LOG(ERROR) << "Failed to get vm stats info";
    return false;
  }
  vm_size_t page_size;
  if (host_page_size(mach_host_self(), &page_size) != KERN_SUCCESS) {
    FML_LOG(ERROR) << "Failed to get page size";
    return false;
  }
  uint64_t total_memory = GetTotalMemory();
  uint64_t memory_used =
      (vm_stats.active_count + vm_stats.inactive_count + vm_stats.wire_count +
       vm_stats.compressor_page_count + vm_stats.purgeable_count +
       vm_stats.speculative_count) *
      page_size;
  float memory_use_percentage =
      static_cast<float>(memory_used) / static_cast<float>(total_memory);

  if (memory_use_percentage > kCriticalPressureThreshold) {
    *pressure_level = DISPATCH_MEMORYPRESSURE_CRITICAL;
  } else if (memory_use_percentage > kModeratePressureThreshold) {
    *pressure_level = DISPATCH_MEMORYPRESSURE_WARN;
  } else {
    *pressure_level = DISPATCH_MEMORYPRESSURE_NORMAL;
  }
  return true;
}

uint64_t SystemMemoryPressureEvaluatorMac::GetTotalMemory() {
  static std::once_flag flag;
  static uint64_t total_memory;
  std::call_once(flag, []() {
    size_t length = sizeof(uint64_t);
    sysctlbyname("hw.memsize", &total_memory, &length, nullptr, 0);
  });
  return total_memory;
}

}  // namespace clay
