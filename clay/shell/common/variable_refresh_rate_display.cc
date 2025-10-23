// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/variable_refresh_rate_display.h"

#include <memory>

#include "clay/fml/logging.h"

static double GetInitialRefreshRate(
    const std::weak_ptr<clay::VariableRefreshRateReporter>&
        refresh_rate_reporter) {
  if (auto reporter = refresh_rate_reporter.lock()) {
    return reporter->GetRefreshRate();
  }
  return 0;
}

namespace clay {

VariableRefreshRateDisplay::VariableRefreshRateDisplay(
    DisplayId display_id,
    const std::weak_ptr<VariableRefreshRateReporter>& refresh_rate_reporter)
    : Display(display_id, GetInitialRefreshRate(refresh_rate_reporter)),
      refresh_rate_reporter_(refresh_rate_reporter) {}

VariableRefreshRateDisplay::VariableRefreshRateDisplay(
    const std::weak_ptr<VariableRefreshRateReporter>& refresh_rate_reporter)
    : Display(GetInitialRefreshRate(refresh_rate_reporter)),
      refresh_rate_reporter_(refresh_rate_reporter) {}

double VariableRefreshRateDisplay::GetRefreshRate() const {
  return GetInitialRefreshRate(refresh_rate_reporter_);
}

}  // namespace clay
