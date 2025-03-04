// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_map.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/services/timing_handler/timing_utils.h"

namespace lynx {
namespace tasm {
namespace timing {

bool TimingMap::SetTimestamp(const TimestampKey& timing_key,
                             TimestampUs timing_value) {
  auto result = timing_infos_.emplace(timing_key, timing_value);
  if (!result.second) {
    LOGE("Set duplicated timing_key, timing_key is " << timing_key);
    return false;
  }
  return true;
}

std::optional<TimestampUs> TimingMap::GetTimestamp(
    const TimestampKey& timing_key) const {
  auto it = timing_infos_.find(timing_key);
  if (it == timing_infos_.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool TimingMap::CheckAllKeysExist(
    const std::initializer_list<std::string>& keys) const {
  // Check if all keys in the list exist in the map
  for (const auto& key : keys) {
    if (timing_infos_.find(key) == timing_infos_.end()) {
      return false;  // If any key is missing, return false
    }
  }
  return true;  // All keys exist
}

std::unique_ptr<lynx::pub::Value> TimingMap::ToPubMap(
    bool as_milliseconds,
    const std::shared_ptr<pub::PubValueFactory>& value_factory) const {
  if (!value_factory) {
    return nullptr;
  }
  auto dict = value_factory->CreateMap();

  if (as_milliseconds) {
    for (const auto& [timing_key, timestamp] : timing_infos_) {
      dict->PushUInt64ToMap(timing_key, ConvertUsToMS(timestamp));
    }
  } else {
    for (const auto& [timing_key, timestamp] : timing_infos_) {
      dict->PushDoubleToMap(timing_key, ConvertUsToDouble(timestamp));
    }
  }

  return dict;
}

// Method to merge other TimingMap
void TimingMap::Merge(const TimingMap& other) {
  timing_infos_.insert(other.timing_infos_.begin(), other.timing_infos_.end());
}

TimingMap TimingMap::GetSubMap(
    const std::initializer_list<std::string>& keys) const {
  TimingMap subMap = TimingMap();
  for (const auto& key : keys) {
    auto it = timing_infos_.find(key);
    if (it == timing_infos_.end()) {
      continue;  // If any key is missing, just continue
    } else {
      subMap.SetTimestamp(key, it->second);
    }
  }
  return subMap;
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
