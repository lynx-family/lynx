// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/animation_keyframe_effect.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace gfx {

std::unique_ptr<KeyframeEffect> KeyframeEffect::Create() {
  return std::make_unique<KeyframeEffect>();
}

KeyframeEffect::KeyframeEffect() = default;

void KeyframeEffect::AddKeyframeModel(KeyframeModel* model) {
  if (model == nullptr) {
    return;
  }
  models_.push_back(model);
}

void KeyframeEffect::SetStartTime(fml::TimePoint& time) {
  for (auto* model : models_) {
    if (model) {
      model->set_start_time(time);
    }
  }
}

void KeyframeEffect::SetPauseTime(fml::TimePoint& time) {
  for (auto* model : models_) {
    if (model) {
      model->SetRunState(KeyframeModel::PAUSED, time);
    }
  }
}

void KeyframeEffect::EnsureFromAndToKeyframe() {
  for (auto* model : models_) {
    if (model) {
      model->EnsureFromAndToKeyframe();
    }
  }
}

bool KeyframeEffect::HasFinishedAll() const {
  KeyframeModel* first_model = nullptr;
  for (auto* model : models_) {
    if (model != nullptr) {
      first_model = model;
      break;
    }
  }
  if (first_model == nullptr) {
    return true;
  }

  const bool has_finished = first_model->is_finished();
  for (auto* model : models_) {
    if (model == nullptr) {
      continue;
    }
    DCHECK_EQ(model->is_finished(), has_finished);
  }
  return has_finished;
}

KeyframeEffect::TickResult KeyframeEffect::Tick(fml::TimePoint monotonic_time) {
  TickResult result;

  for (auto* model : models_) {
    if (model == nullptr) {
      continue;
    }

    bool start_event_due = false;
    bool end_event_due = false;
    std::tie(start_event_due, end_event_due) =
        model->UpdateState(monotonic_time);
    result.start_event_due = start_event_due;
    result.end_event_due = end_event_due;

    if (!model->InEffect(monotonic_time)) {
      continue;
    }

    // Keyframe models in one effect share timing data. Expose the first
    // in-effect active time so the core layer can keep the old new animator's
    // one-check-per-tick reporting semantics.
    if (!result.active_time) {
      result.active_time = model->CalculateActiveTime(monotonic_time);
    }
    const int old_iteration_count = current_iteration_count_;
    fml::TimeDelta trimmed = model->TrimTimeToCurrentIteration(
        monotonic_time, current_iteration_count_);
    if (current_iteration_count_ != old_iteration_count) {
      ++result.iteration_events_due;
    }
    result.samples.push_back({model->curve(), trimmed});
  }
  result.has_finished_all = HasFinishedAll();

  return result;
}

}  // namespace gfx
}  // namespace lynx
