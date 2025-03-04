// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TIME_CHRONO_TIMESTAMP_PROVIDER_H_
#define BASE_INCLUDE_FML_TIME_CHRONO_TIMESTAMP_PROVIDER_H_

#include "base/include/fml/macros.h"
#include "base/include/fml/time/time_point.h"
#include "base/include/fml/time/timestamp_provider.h"

namespace lynx {
namespace fml {

/// TimestampProvider implementation that is backed by std::chrono::steady_clock
/// meant to be used only in tests for `fml`. Other components needing the
/// current time ticks since epoch should instantiate their own time stamp
/// provider backed by Dart clock.
class ChronoTimestampProvider : TimestampProvider {
 public:
  static ChronoTimestampProvider& Instance() {
    static ChronoTimestampProvider instance;
    return instance;
  }

  ~ChronoTimestampProvider() override;

  fml::TimePoint Now() override;

 private:
  ChronoTimestampProvider();

  BASE_DISALLOW_COPY_AND_ASSIGN(ChronoTimestampProvider);
};

fml::TimePoint ChronoTicksSinceEpoch();

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::ChronoTicksSinceEpoch;
using lynx::fml::ChronoTimestampProvider;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_TIME_CHRONO_TIMESTAMP_PROVIDER_H_
