// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_VARIABLE_REFRESH_RATE_REPORTER_H_
#define CLAY_SHELL_COMMON_VARIABLE_REFRESH_RATE_REPORTER_H_

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "base/include/fml/macros.h"

namespace clay {

/// Abstract class that represents a platform specific mechanism to report
/// current refresh rates.
class VariableRefreshRateReporter {
 public:
  VariableRefreshRateReporter() = default;

  virtual double GetRefreshRate() const = 0;

  BASE_DISALLOW_COPY_AND_ASSIGN(VariableRefreshRateReporter);
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_VARIABLE_REFRESH_RATE_REPORTER_H_
