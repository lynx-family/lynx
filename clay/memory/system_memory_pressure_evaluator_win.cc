// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/memory/system_memory_pressure_evaluator_win.h"

#include "base/include/fml/task_runner.h"
#include "clay/fml/logging.h"

namespace clay {

namespace {
static const DWORDLONG kMBBytes = 1024 * 1024;

static VOID CALLBACK OnLowMemoryNotification(PVOID lpParameter,
                                             BOOLEAN TimerOrWaitFired) {
  FML_DCHECK(!TimerOrWaitFired);

  SystemMemoryPressureEvaluatorWin* self =
      static_cast<SystemMemoryPressureEvaluatorWin*>(lpParameter);

  if (!self) {
    return;
  }

  self->UnregisterWaitInsideCallback();

  // On Windows it is possible that this callback function is called
  // but there is still enough space for the application to continue running.
  if (self->CheckMemoryPressure()) {
    self->SetEnablePolling(true);
    self->PeriodicallyCheck();
  }
}
}  // namespace

const fml::TimeDelta
    SystemMemoryPressureEvaluatorWin::kModeratePressureCooldown =
        fml::TimeDelta::FromSeconds(10);

// A system is considered 'high memory' if it has more than 1.5GB of system
// memory available for use by the memory manager (not reserved for hardware
// and drivers). This is a fuzzy version of the ~2GB discussed below.
const int SystemMemoryPressureEvaluatorWin::kLargeMemoryThresholdMb = 1536;
// These are the default thresholds used for systems with < ~2GB of physical
// memory. Such systems have been observed to always maintain ~100MB of
// available memory, paging until that is the case. To try to avoid paging a
// threshold slightly above this is chosen. The moderate threshold is slightly
// less grounded in reality and chosen as 2.5x critical.
const int
    SystemMemoryPressureEvaluatorWin::kSmallMemoryDefaultModerateThresholdMb =
        500;
const int
    SystemMemoryPressureEvaluatorWin::kSmallMemoryDefaultCriticalThresholdMb =
        200;
// These are the default thresholds used for systems with >= ~2GB of physical
// memory. Such systems have been observed to always maintain ~300MB of
// available memory, paging until that is the case.
const int
    SystemMemoryPressureEvaluatorWin::kLargeMemoryDefaultModerateThresholdMb =
        1000;
const int
    SystemMemoryPressureEvaluatorWin::kLargeMemoryDefaultCriticalThresholdMb =
        400;

// Check the amount of RAM left every 5 seconds.
const fml::TimeDelta SystemMemoryPressureEvaluatorWin::kMemorySamplingPeriod =
    fml::TimeDelta::FromSeconds(5);

SystemMemoryPressureEvaluatorWin::SystemMemoryPressureEvaluatorWin(
    fml::RefPtr<fml::TaskRunner> task_runner)
    : moderate_pressure_repeat_count_(0),
      moderate_threshold_mb_(0),
      critical_threshold_mb_(0),
      task_runner_(task_runner),
      weak_factory_(this) {
  InferThresholds();
  StartObserving();
}

SystemMemoryPressureEvaluatorWin::~SystemMemoryPressureEvaluatorWin() {
  enable_polling_ = false;
  if (wait_handle_ != nullptr) {
    UnregisterWait(wait_handle_);
  }
  if (low_memory_handle_) {
    CloseHandle(low_memory_handle_);
  }
}

void SystemMemoryPressureEvaluatorWin::UnregisterWaitInsideCallback() {
  UnregisterWaitEx(wait_handle_, INVALID_HANDLE_VALUE);
  wait_handle_ = nullptr;
}

void SystemMemoryPressureEvaluatorWin::StartObserving() {
  if (!TryOsSignalObserving()) {
    // Cannot listen to OS signals, fall back to polling.
    enable_polling_ = true;
    PeriodicallyCheck();
  } else {
    enable_os_signal_ = true;
  }
}

bool SystemMemoryPressureEvaluatorWin::TryOsSignalObserving() {
  if (!low_memory_handle_) {
    low_memory_handle_ =
        CreateMemoryResourceNotification(LowMemoryResourceNotification);
  }
  if (!low_memory_handle_) {
    FML_LOG(ERROR) << "Failed to CreateMemoryResourceNotification with error "
                   << GetLastError()
                   << ". Will check memory pressure by polling.";
    return false;
  }
  DWORD wait_flags = WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE;
  bool success = RegisterWaitForSingleObject(&wait_handle_, low_memory_handle_,
                                             OnLowMemoryNotification, this,
                                             INFINITE, wait_flags);
  if (!success) {
    if (wait_handle_ != nullptr) {
      UnregisterWait(wait_handle_);
      wait_handle_ = nullptr;
    }
    if (low_memory_handle_) {
      CloseHandle(low_memory_handle_);
      low_memory_handle_ = nullptr;
    }
    FML_LOG(ERROR) << "Failed to RegisterWaitForSingleObject with error "
                   << GetLastError()
                   << ". Will check memory pressure by polling.";
    return false;
  }
  return true;
}

void SystemMemoryPressureEvaluatorWin::PeriodicallyCheck() {
  if (task_runner_) {
    auto weak_self = weak_factory_.GetWeakPtr();
    task_runner_->PostDelayedTask(
        [self = weak_self]() {
          if (!self) {
            return;
          }
          self->CheckMemoryPressure();
          if (self->EnablePolling()) {
            self->PeriodicallyCheck();
          }
        },
        kMemorySamplingPeriod);
  }
}

bool SystemMemoryPressureEvaluatorWin::CheckMemoryPressure() {
  MemoryPressureLevel old_pressure_level = GetCurrentPressureLevel();
  SetCurrentPressureLevel(CalculateCurrentPressureLevel());

  bool notify = false;
  switch (GetCurrentPressureLevel()) {
    case MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE:
      break;
    case MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE:
      if (old_pressure_level != GetCurrentPressureLevel()) {
        moderate_pressure_repeat_count_ = 1;
      } else {
        // Already in moderate pressure, only notify if sustained over the
        // cooldown period.
        const int kModeratePressureCooldownCycles =
            kModeratePressureCooldown / kMemorySamplingPeriod;
        if (++moderate_pressure_repeat_count_ ==
            kModeratePressureCooldownCycles) {
          notify = true;
          moderate_pressure_repeat_count_ = 0;
        }
      }
      break;
    case MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL:
      notify = true;
      break;
  }
  if (notify) {
    SendCurrentPressureLevel(notify);
  }
  if (enable_os_signal_ && !notify) {
    enable_polling_ = false;
    TryOsSignalObserving();
  }
  return notify;
}

MemoryPressureListener::MemoryPressureLevel
SystemMemoryPressureEvaluatorWin::CalculateCurrentPressureLevel() {
  MEMORYSTATUSEX mem_info;
  if (!GetSystemMemoryStatus(&mem_info)) {
    FML_LOG(ERROR) << "Get system memory status failed!";
    return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
  }
  // How much system memory is actively available for use right now, in MBs.
  int phys_free = static_cast<int>(mem_info.ullAvailPhys / kMBBytes);
  // Determine if the physical memory is under critical memory pressure.
  if (phys_free <= critical_threshold_mb_)
    return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;

  // Determine if the physical memory is under moderate memory pressure.
  if (phys_free <= moderate_threshold_mb_)
    return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;

  // No memory pressure was detected.
  return MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
}

void SystemMemoryPressureEvaluatorWin::InferThresholds() {
  // Default to high memory situation for conservative considering.
  bool high_memory = true;
  MEMORYSTATUSEX mem_info;
  if (GetSystemMemoryStatus(&mem_info)) {
    static const DWORDLONG kLargeMemoryThresholdBytes =
        static_cast<DWORDLONG>(kLargeMemoryThresholdMb) * kMBBytes;
    high_memory = mem_info.ullTotalPhys >= kLargeMemoryThresholdBytes;
  }
  if (high_memory) {
    moderate_threshold_mb_ = kLargeMemoryDefaultModerateThresholdMb;
    critical_threshold_mb_ = kLargeMemoryDefaultCriticalThresholdMb;
  } else {
    moderate_threshold_mb_ = kSmallMemoryDefaultModerateThresholdMb;
    critical_threshold_mb_ = kSmallMemoryDefaultCriticalThresholdMb;
  }
}

bool SystemMemoryPressureEvaluatorWin::GetSystemMemoryStatus(
    MEMORYSTATUSEX* mem_info) {
  FML_DCHECK(mem_info);
  mem_info->dwLength = sizeof(*mem_info);
  return GlobalMemoryStatusEx(mem_info);
}

}  // namespace clay
