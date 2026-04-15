// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/embedder/vsync_waiter_embedder.h"

#include <memory>
#include <utility>

namespace clay {

VsyncWaiterEmbedder::VsyncWaiterEmbedder(
    const VsyncCallback& vsync_callback,
    fml::RefPtr<fml::TaskRunner> task_runner)
    : VsyncWaiter(std::move(task_runner)), vsync_callback_(vsync_callback) {
  FML_DCHECK(vsync_callback_);
}

VsyncWaiterEmbedder::~VsyncWaiterEmbedder() = default;

// |VsyncWaiter|
void VsyncWaiterEmbedder::AwaitVSync() {
  if (!IsEngineActive()) {
    return;
  }
  auto* weak_waiter = new std::weak_ptr<VsyncWaiter>(shared_from_this());
  intptr_t baton = reinterpret_cast<intptr_t>(weak_waiter);
  vsync_callback_(baton);
}

// static
bool VsyncWaiterEmbedder::OnEmbedderVsync(const clay::TaskRunners& task_runners,
                                          intptr_t baton,
                                          fml::TimePoint frame_start_time,
                                          fml::TimePoint frame_target_time) {
  if (baton == 0) {
    return false;
  }

  auto* weak_waiter = reinterpret_cast<std::weak_ptr<VsyncWaiter>*>(baton);
  auto strong_waiter = weak_waiter->lock();
  delete weak_waiter;

  if (!strong_waiter) {
    return false;
  }

  fml::TaskRunner::RunNowOrPostTask(
      strong_waiter->task_runner_,
      [strong_waiter, frame_start_time, frame_target_time] {
        strong_waiter->FireCallback(frame_start_time, frame_target_time);
      });

  return true;
}

}  // namespace clay
