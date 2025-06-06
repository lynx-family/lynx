// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/memory_monitor.h"

#include <utility>

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {
namespace performance {

static std::atomic<bool> g_force_enable_ = false;

bool MemoryMonitor::Enable() {
  return g_force_enable_ || LynxEnv::GetInstance().EnableMemoryMonitor();
}

void MemoryMonitor::SetForceEnable(bool force_enable) {
  g_force_enable_ = force_enable;
}

uint32_t MemoryMonitor::MemoryChangeThresholdMb() {
  return LynxEnv::GetInstance().GetMemoryChangeThresholdMb();
}

uint32_t MemoryMonitor::ScriptingEngineMode() {
  uint32_t mode = 0;
  bool enable_mem_monitor = Enable();
  mode |= (enable_mem_monitor << 0);
  uint32_t mem_increment_threshold_mb = MemoryChangeThresholdMb();
  if (mem_increment_threshold_mb > 100) {
    mem_increment_threshold_mb = 100;
  }
  mode |= (mem_increment_threshold_mb << 1);
  return mode;
}

MemoryMonitor::MemoryMonitor(MemoryMonitor&& other)
    : sender_(other.sender_),
      memory_records_(std::move(other.memory_records_)) {
  other.sender_ = nullptr;
}

MemoryMonitor& MemoryMonitor::operator=(MemoryMonitor&& other) {
  if (this != &other) {
    // move
    sender_ = other.sender_;
    memory_records_ = std::move(other.memory_records_);
    // clear other
    other.sender_ = nullptr;
  }
  return *this;
}

MemoryMonitor::~MemoryMonitor() {
  if (Enable()) {
    ReportMemory();
  }
}

void MemoryMonitor::AllocateMemory(MemoryRecord&& record) {
  if (!Enable()) {
    return;
  }
  auto ret_record_it = memory_records_.find(record.category_);
  if (ret_record_it == memory_records_.end()) {
    memory_records_.emplace(record.category_, std::move(record));
  } else {
    ret_record_it->second += record;
  }
  ReportMemory();
}

void MemoryMonitor::DeallocateMemory(MemoryRecord&& record) {
  if (!Enable()) {
    return;
  }
  auto ret_record_it = memory_records_.find(record.category_);
  if (ret_record_it == memory_records_.end()) {
    return;
  }
  ret_record_it->second -= record;
  ReportMemory();
}

void MemoryMonitor::UpdateMemoryUsage(MemoryRecord&& record) {
  if (!Enable()) {
    return;
  }
  memory_records_[record.category_] = std::move(record);
  ReportMemory();
}

void MemoryMonitor::ReportMemory() {
  if (sender_ == nullptr) {
    return;
  }
  auto& factory = sender_->GetValueFactory();
  if (factory == nullptr) {
    return;
  }
  auto entry_map = factory->CreateMap();
  entry_map->PushStringToMap("entryType", "memory");
  entry_map->PushStringToMap("name", "memory");
  float sizeKb = 0.f;
  if (memory_records_.empty()) {
    entry_map->PushDoubleToMap("sizeKb", sizeKb);
    sender_->OnPerformanceEvent(std::move(entry_map));
    return;
  }
  auto detail = sender_->GetValueFactory()->CreateMap();
  for (const auto& [category, record] : memory_records_) {
    auto record_map = sender_->GetValueFactory()->CreateMap();
    record_map->PushStringToMap("category", record.category_);
    record_map->PushDoubleToMap("sizeKb", record.size_kb_);
    sizeKb += record.size_kb_;
    if (record.detail_) {
      auto map = sender_->GetValueFactory()->CreateMap();
      for (const auto& [key, value] : *(record.detail_)) {
        map->PushStringToMap(key, value);
      }
      record_map->PushValueToMap("detail", std::move(map));
    }
    detail->PushValueToMap(category, std::move(record_map));
  }
  entry_map->PushDoubleToMap("sizeKb", sizeKb);
  entry_map->PushValueToMap("detail", std::move(detail));
  sender_->OnPerformanceEvent(std::move(entry_map));
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
