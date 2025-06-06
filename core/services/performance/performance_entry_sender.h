// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_PERFORMANCE_ENTRY_SENDER_H_
#define CORE_SERVICES_PERFORMANCE_PERFORMANCE_ENTRY_SENDER_H_

#include <cstddef>
#include <memory>

#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {
namespace performance {

using PerformanceObserverEnv = uint8_t;

inline constexpr PerformanceObserverEnv kPerformanceObserverOfPlatform = 1 << 0;
inline constexpr PerformanceObserverEnv kPerformanceObserverOfBTSEngine = 1
                                                                          << 1;
inline constexpr PerformanceObserverEnv kPerformanceObserverOfMTSEngine = 1
                                                                          << 2;
inline constexpr PerformanceObserverEnv kPerformanceObserverOfAll =
    kPerformanceObserverOfPlatform | kPerformanceObserverOfBTSEngine |
    kPerformanceObserverOfMTSEngine;

class PerformanceEntrySender {
 public:
  explicit PerformanceEntrySender(
      std::shared_ptr<pub::PubValueFactory> value_factory)
      : value_factory_(value_factory) {}
  virtual ~PerformanceEntrySender() = default;
  virtual void OnPerformanceEvent(
      std::unique_ptr<lynx::pub::Value> entry,
      PerformanceObserverEnv env = kPerformanceObserverOfAll) = 0;
  virtual const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() {
    return value_factory_;
  };

 protected:
  std::shared_ptr<pub::PubValueFactory> value_factory_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_PERFORMANCE_ENTRY_SENDER_H_
