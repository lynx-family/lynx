// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/caret_blink_controller.h"

#include <utility>

#include "base/include/fml/time/time_delta.h"

namespace clay {
namespace {

constexpr uint64_t kCaretTwinkleIntervalMs = 500;

}  // namespace

CaretBlinkController::CaretBlinkController(
    fml::RefPtr<fml::TaskRunner> task_runner,
    SetCaretDisplayCallback set_caret_display)
    : task_runner_(std::move(task_runner)),
      set_caret_display_(std::move(set_caret_display)) {}

CaretBlinkController::~CaretBlinkController() { Stop(); }

void CaretBlinkController::Start() {
  SetCaretDisplay(true);
  if (!timer_) {
    timer_ = std::make_unique<fml::RepeatingTimer>(task_runner_);
  }
  timer_->Start(fml::TimeDelta::FromMilliseconds(kCaretTwinkleIntervalMs),
                [this] { Toggle(); });
}

void CaretBlinkController::Stop() {
  if (timer_) {
    timer_.reset();
  }
  SetCaretDisplay(false);
}

void CaretBlinkController::Toggle() { SetCaretDisplay(!display_); }

void CaretBlinkController::SetCaretDisplay(bool display) {
  if (display_ == display) {
    return;
  }
  display_ = display;
  if (set_caret_display_) {
    set_caret_display_(display_);
  }
}

}  // namespace clay
