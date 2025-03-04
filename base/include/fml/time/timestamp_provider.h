// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TIME_TIMESTAMP_PROVIDER_H_
#define BASE_INCLUDE_FML_TIME_TIMESTAMP_PROVIDER_H_

#include <cstdint>

#include "base/include/fml/time/time_point.h"

namespace lynx {
namespace fml {

/// Pluggable provider of monotonic timestamps. Invocations of `Now` must return
/// unique values. Any two consecutive invocations must be ordered.
class TimestampProvider {
 public:
  virtual ~TimestampProvider(){};

  // Returns the number of ticks elapsed by a monotonic clock since epoch.
  virtual fml::TimePoint Now() = 0;
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::TimestampProvider;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_TIME_TIMESTAMP_PROVIDER_H_
