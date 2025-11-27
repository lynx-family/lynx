// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_VSYNC_WAITER_EMBEDDER_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_VSYNC_WAITER_EMBEDDER_H_

#include "base/include/fml/macros.h"
#include "clay/shell/common/vsync_waiter.h"

namespace clay {

class VsyncWaiterEmbedder final : public VsyncWaiter {
 public:
  using VsyncCallback = std::function<void(intptr_t)>;

  VsyncWaiterEmbedder(const VsyncCallback& callback,
                      fml::RefPtr<fml::TaskRunner> task_runner);

  ~VsyncWaiterEmbedder() override;

  static bool OnEmbedderVsync(const clay::TaskRunners& task_runners,
                              intptr_t baton, fml::TimePoint frame_start_time,
                              fml::TimePoint frame_target_time);

 private:
  const VsyncCallback vsync_callback_;

  // |VsyncWaiter|
  void AwaitVSync() override;

  BASE_DISALLOW_COPY_AND_ASSIGN(VsyncWaiterEmbedder);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_VSYNC_WAITER_EMBEDDER_H_
