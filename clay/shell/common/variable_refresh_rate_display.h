// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_VARIABLE_REFRESH_RATE_DISPLAY_H_
#define CLAY_SHELL_COMMON_VARIABLE_REFRESH_RATE_DISPLAY_H_

#include <memory>
#include <optional>

#include "base/include/fml/macros.h"
#include "clay/shell/common/display.h"
#include "clay/shell/common/variable_refresh_rate_reporter.h"

namespace clay {

/// A Display where the refresh rate can change over time.
class VariableRefreshRateDisplay : public Display {
 public:
  explicit VariableRefreshRateDisplay(
      DisplayId display_id,
      const std::weak_ptr<VariableRefreshRateReporter>& refresh_rate_reporter);
  explicit VariableRefreshRateDisplay(
      const std::weak_ptr<VariableRefreshRateReporter>& refresh_rate_reporter);
  ~VariableRefreshRateDisplay() = default;

  // |Display|
  double GetRefreshRate() const override;

 private:
  const std::weak_ptr<VariableRefreshRateReporter> refresh_rate_reporter_;

  BASE_DISALLOW_COPY_AND_ASSIGN(VariableRefreshRateDisplay);
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_VARIABLE_REFRESH_RATE_DISPLAY_H_
