// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/memory_monitor.h"

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/trace/service_trace_event_def.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace performance {

#if ENABLE_TRACE_PERFETTO
inline std::string ValueToJsonString(const pub::Value& value) {
  if (value.IsUndefined() || value.IsNil()) {
    return "null";
  }
  if (value.IsBool()) {
    return value.Bool() ? "true" : "false";
  } else if (value.IsNumber()) {
    std::ostringstream oss;
    oss.precision(15);
    oss << value.Number();
    return oss.str();
  } else if (value.IsString()) {
    std::ostringstream oss;
    oss.precision(15);
    oss << "\"" << value.str() << "\"";
    return oss.str();
  } else if (value.IsArray()) {
    std::string result = "[";
    bool first = true;
    value.ForeachArray([&](int64_t, const pub::Value& item) {
      if (!first) {
        result += ",";
      }
      result += ValueToJsonString(item);
      first = false;
    });
    result += "]";
    return result;
  } else if (value.IsMap()) {
    std::string result = "{";
    bool first = true;
    value.ForeachMap([&](const pub::Value& key, const pub::Value& val) {
      if (!first) {
        result += ",";
      }
      result += ValueToJsonString(key) + ":" + ValueToJsonString(val);
      first = false;
    });
    result += "}";
    return result;
  }
  return "\"<unknown>\"";
}
#endif

inline MemoryRecord BuildMemoryRecord(
    const rapidjson::Value& obj,
    std::unordered_map<std::string, std::string> info) {
  MemoryRecord record;
  // size_bytes
  const rapidjson::Value& heap_size_kb_after_v = obj["heapsize_after"];
  if (heap_size_kb_after_v.IsNumber()) {
    record.size_bytes_ = heap_size_kb_after_v.GetUint() * 1024;
  }
  // category
  auto category_it = info.find(kCategory);
  if (category_it != info.end()) {
    record.category_ = category_it->second;
  }
  // detail
  record.detail_ =
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::move(info));
  if (obj.IsObject()) {
    for (rapidjson::Value::ConstMemberIterator itr = obj.MemberBegin();
         itr != obj.MemberEnd(); ++itr) {
      std::string key = itr->name.GetString();
      if (key == kRawRuntimeMemoryInfo) {
        continue;
      }
      std::string value;
      if (itr->value.IsString()) {
        value = itr->value.GetString();
      } else if (itr->value.IsInt()) {
        value = std::to_string(itr->value.GetInt());
      } else if (itr->value.IsDouble()) {
        value = std::to_string(itr->value.GetDouble());
      } else if (itr->value.IsBool()) {
        value = itr->value.GetBool() ? "true" : "false";
      } else {
        value = "Unsupported type";
      }
      record.detail_->emplace(key, std::move(value));
    }
  }
  return record;
}

enum BoolValue : uint8_t { Unset = 0, False = 1, True = 2 };
// Highest priority setting
static std::atomic<BoolValue> g_force_enable_{Unset};
// Environment-based setting
static std::atomic<BoolValue> g_env_enable_{Unset};

bool MemoryMonitor::Enable() {
  // [Priority 1] Check force-enable setting first (explicit override)
  const auto force_val = g_force_enable_.load(std::memory_order_acquire);
  if (force_val != Unset) {
    return force_val == True;  // Force setting takes absolute precedence
  }

  // [Priority 2] Check cached environment setting
  const auto env_val = g_env_enable_.load(std::memory_order_acquire);
  if (env_val != Unset) {
    return env_val == True;  // Use cached environment value if available
  }

  // [Initialization] Both unset - fetch from environment (once)
  static std::once_flag init_flag;
  bool ret = false;
  std::call_once(init_flag, [&] {
    // Fetch actual value from environment source
    ret = LynxEnv::GetInstance().EnableMemoryMonitor();

    // Cache environment result for future calls
    g_env_enable_.store(ret ? True : False, std::memory_order_release);
  });
  return ret;
}

// External control interface (sets highest priority flag)
void MemoryMonitor::SetForceEnable(bool enable) {
  // This override takes precedence over environment settings
  g_force_enable_.store(enable ? True : False, std::memory_order_release);
}

uint32_t MemoryMonitor::MemoryChangeThresholdMb() {
  static uint32_t threshold_mb =
      LynxEnv::GetInstance().GetMemoryChangeThresholdMb();
  return threshold_mb;
}

uint32_t MemoryMonitor::ScriptingEngineMode() {
  uint32_t mode = 0;
  bool enable_mem_monitor = Enable();
  if (!enable_mem_monitor) {
    return mode;
  }

  // Maximum allowed value for memory threshold (8-bit unsigned max)
  constexpr uint32_t kMaxMemThreshold = std::numeric_limits<uint8_t>::max();
  uint32_t mem_increment_threshold_mb = MemoryChangeThresholdMb();
  // Cap memory threshold at 8-bit maximum (255 MB)
  if (mem_increment_threshold_mb > kMaxMemThreshold) {
    mem_increment_threshold_mb = kMaxMemThreshold;
  }

  // Bit shift position for memory threshold in the mode register
  constexpr uint32_t kMemThresholdShift = 24;
  /*
   * Mode register bit layout:
   *   Bits [31:24] - Memory increment threshold (MB)
   *   Bits [23:0]  - Reserved for other flags/values
   */
  mode = (mem_increment_threshold_mb << kMemThresholdShift);
  return mode;
}

MemoryMonitor::~MemoryMonitor() {
  // Clear records and report 0 memory usage.
  memory_records_.clear();
  bool enable = Enable();
  if (enable) {
    ReportMemory();
  }
  LOGI("[memory_monitor.h] ~MemoryMonitor, this:"
       << this << ", Enable:" << enable
       << ", MemoryChangeThresholdMb:" << MemoryChangeThresholdMb());
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
  auto it = memory_records_.find(record.category_);
  if (it != memory_records_.end()) {
    if (it->second.size_bytes_ == record.size_bytes_) {
      // No change in memory usage, no need to report.
      return;
    }
    it->second = std::move(record);
  } else {
    memory_records_.emplace(record.category_, std::move(record));
  }
  ReportMemory();
}

void MemoryMonitor::UpdateScriptingEngineMemoryUsage(
    std::unordered_map<std::string, std::string> info) {
  if (!Enable()) {
    return;
  }
  auto it = info.find(kRawRuntimeMemoryInfo);
  if (it == info.end()) {
    return;
  }

  rapidjson::Document doc;
  doc.Parse(it->second);
  info.erase(kRawRuntimeMemoryInfo);
  if (doc.HasParseError() || !doc.IsObject()) {
    return;
  }
  const rapidjson::Value& gc_info = doc["gc_info"];
  if (!gc_info.IsArray()) {
    return;
  }
  rapidjson::SizeType arraySize = gc_info.Size();
  if (arraySize == 0) {
    return;
  }
  const rapidjson::Value& lastElement = gc_info[arraySize - 1];
  if (lastElement.IsNull()) {
    return;
  }
  UpdateMemoryUsage(BuildMemoryRecord(lastElement, std::move(info)));
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
  entry_map->PushStringToMap(kPerformanceEventType, kMemoryEntryType);
  entry_map->PushStringToMap(kPerformanceEventName, kMemoryEntryType);
  int64_t size_bytes = 0;
  if (!memory_records_.empty()) {
    auto detail = sender_->GetValueFactory()->CreateMap();
    for (const auto& [category, record] : memory_records_) {
      auto record_map = sender_->GetValueFactory()->CreateMap();
      record_map->PushStringToMap(kCategory, record.category_);
      record_map->PushInt64ToMap(kSizeBytes, record.size_bytes_);
      record_map->PushInt32ToMap(kInstanceCount, record.instance_count_);
      size_bytes += record.size_bytes_;
      if (record.detail_) {
        auto map = sender_->GetValueFactory()->CreateMap();
        for (const auto& [key, value] : *(record.detail_)) {
          map->PushStringToMap(key, value);
        }
        record_map->PushValueToMap(kDetail, std::move(map));
      }
      detail->PushValueToMap(category, std::move(record_map));
    }
    entry_map->PushValueToMap(kDetail, std::move(detail));
  }
  // Throttle reporting: only report if memory change exceeds the threshold.
  static int64_t memory_report_threshold_bytes =
      static_cast<int64_t>(MemoryChangeThresholdMb()) * 1024 * 1024;
  if (std::abs(size_bytes - last_reported_size_bytes_) <
      memory_report_threshold_bytes) {
    return;
  }
  // Update the last reported size for the next check.
  last_reported_size_bytes_ = size_bytes;
  TRACE_COUNTER(
      LYNX_TRACE_CATEGORY,
      (std::string("memory_") + std::to_string(instance_id_)).c_str(),
      size_bytes,
      [&entry_map, instance_id = instance_id_,
       size_bytes](perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(kSizeBytes,
                                           std::to_string(size_bytes));
        auto detail = entry_map->GetValueForKey(kDetail);
        detail->ForeachMap([debug = ctx.event()](const pub::Value& key,
                                                 const pub::Value& val) {
          if (key.IsString()) {
            if (val.IsString()) {
              debug->add_debug_annotations(key.str(), val.str());
            } else if (val.IsBool()) {
              debug->add_debug_annotations(key.str(),
                                           std::to_string(val.Bool()));
            } else if (val.IsNumber()) {
              debug->add_debug_annotations(key.str(),
                                           std::to_string(val.Number()));
            } else if (val.IsMap()) {
              auto v = ValueToJsonString(val);
              debug->add_debug_annotations(key.str(), v);
            }
          }
        });
        ctx.event()->add_debug_annotations(INSTANCE_ID,
                                           std::to_string(instance_id));
      });

  entry_map->PushInt64ToMap(kSizeBytes, size_bytes);
  sender_->OnPerformanceEvent(std::move(entry_map), kEventTypePlatform);
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
