// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_MAP_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_MAP_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {
namespace timing {

using TimestampKey = std::string;
// Represents a timestamp in microseconds, like 17179869184
using TimestampUs = uint64_t;
// Represents a timestamp in milliseconds, like 17179869
using TimestampMs = uint64_t;
// Represents a timestamp in milliseconds with fraction part for microseconds,
// like 17179869.184
using TimestampMsFraction = double;

/**
 * @brief A map that stores timing information.
 *
 * The TimingMap class is designed to handle microsecond timing values,
 * providing methods to add, retrieve, and convert these values to lepus::value
 * for further processing. It uses an unordered_map to store the timestamp
 * values associated with unique keys.
 *
 */
class TimingMap {
 public:
  TimingMap() = default;
  ~TimingMap() = default;

  TimingMap(TimingMap&) = delete;
  TimingMap(const TimingMap&) = default;
  TimingMap(TimingMap&&) = default;
  TimingMap& operator=(const TimingMap&) = default;
  TimingMap& operator=(TimingMap&&) = default;

  // Method to add timing value
  bool SetTimestamp(const TimestampKey& timing_key, TimestampUs timing_value);

  // Method to safely get timing value
  std::optional<TimestampUs> GetTimestamp(const TimestampKey& timing_key) const;

  // Method to check if all keys exist in timing_infos.
  bool CheckAllKeysExist(const std::initializer_list<std::string>& keys) const;

  inline bool Empty() { return timing_infos_.empty(); };

  inline void Clear() { timing_infos_.clear(); };

  std::unique_ptr<lynx::pub::Value> ToPubMap(
      bool as_milliseconds,
      const std::shared_ptr<pub::PubValueFactory>& value_factory) const;
  // Method to merge other TimingMap
  void Merge(const TimingMap& other);

  TimingMap GetSubMap(const std::initializer_list<std::string>& keys) const;

 private:
  // All of the timestamp is saved as microsecond
  using TimestampMap = std::unordered_map<TimestampKey, TimestampUs>;
  TimestampMap timing_infos_;
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_MAP_H_
