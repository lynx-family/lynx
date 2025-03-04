// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/time/chrono_timestamp_provider.h"

#include <chrono>

#include "base/include/fml/time/time_delta.h"

namespace lynx {
namespace fml {

ChronoTimestampProvider::ChronoTimestampProvider() = default;

ChronoTimestampProvider::~ChronoTimestampProvider() = default;

fml::TimePoint ChronoTimestampProvider::Now() {
  const auto chrono_time_point = std::chrono::steady_clock::now();
  const auto ticks_since_epoch = chrono_time_point.time_since_epoch().count();
  return fml::TimePoint::FromTicks(ticks_since_epoch);
}

fml::TimePoint ChronoTicksSinceEpoch() {
  return ChronoTimestampProvider::Instance().Now();
}

}  // namespace fml
}  // namespace lynx
