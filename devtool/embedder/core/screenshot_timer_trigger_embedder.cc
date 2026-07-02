// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/screenshot_timer_trigger_embedder.h"

#include <utility>

#include "devtool/embedder/core/screenshot_thread_manager.h"

namespace lynx {
namespace devtool {

namespace {
// This interval defines the minimum cadence for continuous screencast capture
// attempts, not the display vsync interval or capture/encode cost. Bound it to
// 1 fps so static pages or fast-acked dynamic pages cannot trigger multiple
// captures/sends within the same second.
constexpr int64_t SCREENSHOT_INTERVAL = 1000;  // milliseconds
}  // namespace

ScreenshotTimerTriggerEmbedder::ScreenshotTimerTriggerEmbedder(
    ScreenshotCallback callback)
    : callback_(std::move(callback)) {}

void ScreenshotTimerTriggerEmbedder::Start() {
  ScreenshotThreadManager::GetScreenshotRunner()->PostTask(
      [self = shared_from_this(), callback = callback_]() {
        if (self->is_running_) {
          return;
        }
        ScreenshotThreadManager::GetTimer().StopAllTasks();
        self->task_id_ = ScreenshotThreadManager::GetTimer().SetInterval(
            std::move(callback), SCREENSHOT_INTERVAL);
        self->is_running_ = true;
      });
}

void ScreenshotTimerTriggerEmbedder::Pause() {
  ScreenshotThreadManager::GetScreenshotRunner()->PostTask(
      [self = shared_from_this()]() {
        ScreenshotThreadManager::GetTimer().StopTask(self->task_id_);
        self->is_running_ = false;
      });
}

void ScreenshotTimerTriggerEmbedder::Continue() { Start(); }

ScreenshotTimerTriggerEmbedder::~ScreenshotTimerTriggerEmbedder() {
  ScreenshotThreadManager::GetScreenshotRunner()->PostTask(
      [task_id = task_id_]() {
        ScreenshotThreadManager::GetTimer().StopTask(task_id);
      });
}

}  // namespace devtool
}  // namespace lynx
