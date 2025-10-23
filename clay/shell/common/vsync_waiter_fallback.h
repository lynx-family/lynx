// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_VSYNC_WAITER_FALLBACK_H_
#define CLAY_SHELL_COMMON_VSYNC_WAITER_FALLBACK_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/time/time_point.h"
#include "clay/shell/common/vsync_waiter.h"

namespace clay {

/// A |VsyncWaiter| that will fire at 60 fps irrespective of the vsync.
class VsyncWaiterFallback final : public VsyncWaiter {
 public:
  explicit VsyncWaiterFallback(fml::RefPtr<fml::TaskRunner> task_runner);

  ~VsyncWaiterFallback() override;
  double GetRefreshRate() const override;

 private:
  fml::TimePoint phase_;

  // |VsyncWaiter|
  void AwaitVSync() override;

  BASE_DISALLOW_COPY_AND_ASSIGN(VsyncWaiterFallback);
};

std::shared_ptr<class VsyncWaiterService> CreateFallbackVsyncWaiterService();

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_VSYNC_WAITER_FALLBACK_H_
