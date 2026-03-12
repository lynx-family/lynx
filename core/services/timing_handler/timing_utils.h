// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_UTILS_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_UTILS_H_

#include "core/services/timing_handler/timing_map.h"

namespace lynx {
namespace tasm {
namespace timing {

// Error code for duration calculation.
enum DurationError {
  // Error code when start time is 0.
  kErrorStartIsZero = -1,
  // Error code when end time is 0.
  kErrorEndIsZero = -2,
  // Error code when start time and end time are both 0.
  kErrorStartAndEndAreZero = -3,
  // Error code when start time is greater than end time.
  kErrorStartTimeGreaterThanEndTime = -4
};

TimestampMsFraction CalculateDuration(TimestampMs start_time,
                                      TimestampMs end_time);

TimestampMsFraction CalculateMsDuration(TimestampUs start_time,
                                        TimestampUs end_time);

// Helper function to get ms_timestamp from us_timestamp
constexpr TimestampUs ConvertUsToMS(TimestampUs us_timestamp) {
  return us_timestamp / 1000;
}

// Helper function to get double timestamp from us_timestamp
constexpr TimestampMsFraction ConvertUsToDouble(TimestampUs us_timestamp) {
  return static_cast<double>(us_timestamp) / 1000.0;
}

bool TryUpdatePolyfillTimingKey(const TimestampKey& key,
                                TimestampKey& polyfill_key);

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_UTILS_H_
