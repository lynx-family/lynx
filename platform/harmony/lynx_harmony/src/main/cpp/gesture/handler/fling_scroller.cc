// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/fling_scroller.h"

#include "base/include/thread/timed_task.h"
#include "core/base/threading/vsync_monitor.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {

FlingScroller::FlingScroller() { last_time_ = 0; }

FlingScroller::~FlingScroller() {}

void FlingScroller::Start(float velocity_x, float velocity_y,
                          FlingCallback callback) {
  if (current_fling_state_ == static_cast<uint8_t>(FlingState::FLING)) {
    Stop();
  }
  // set TimingFunctionData
  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func =
      starlight::TimingFunctionType::kCubicBezier;
  timing_function_data.x1 = 0.25f;
  timing_function_data.y1 = 0.1f;
  timing_function_data.x2 = 0.25f;
  timing_function_data.y2 = 1.0f;

  last_time_ = 0;
  // set AnimationData
  starlight::AnimationData data;
  data.duration = 1800;
  data.fill_mode = starlight::AnimationFillModeType::kForwards;
  data.timing_func = timing_function_data;
  auto cb = std::make_shared<FlingCallback>(std::move(callback));
  fling_scroller_animator_ =
      std::make_shared<animation::basic::LynxBasicAnimator>(
          data, nullptr,
          animation::basic::LynxBasicAnimator::BasicValueType::FLOAT);

  fling_scroller_animator_->RegisterEventCallback(
      [cb, this]() {
        current_fling_state_ = static_cast<uint8_t>(FlingState::IDLE);
        (*cb)(static_cast<uint8_t>(FlingState::IDLE), 0.f, 0.f);
      },
      animation::basic::Animation::EventType::End);

  fling_scroller_animator_->RegisterEventCallback(
      [cb, this]() {
        current_fling_state_ = static_cast<uint8_t>(FlingState::IDLE);
        (*cb)(static_cast<uint8_t>(FlingState::IDLE), 0.f, 0.f);
      },
      animation::basic::Animation::EventType::Cancel);

  fling_scroller_animator_->RegisterCustomCallback([cb, velocity_x, velocity_y,
                                                    this](float progress) {
    if (last_time_ == 0) {
      last_time_ = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
      return;
    }
    auto current_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();

    int64_t delta_time = current_time - last_time_;
    float delta_x = (velocity_x * (1.0f - progress) / 1000) * delta_time;
    float delta_y = (velocity_y * (1.0f - progress) / 1000) * delta_time;
    current_fling_state_ = static_cast<uint8_t>(FlingState::FLING);
    (*cb)(static_cast<uint8_t>(FlingState::FLING), -delta_x, -delta_y);
    this->last_time_ = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  });
  fling_scroller_animator_->Start();
}

bool FlingScroller::IsIdle() {
  return current_fling_state_ == static_cast<uint8_t>(FlingState::IDLE);
}

void FlingScroller::Stop() {
  if (fling_scroller_animator_) {
    fling_scroller_animator_->Stop();
  }
  current_fling_state_ = static_cast<uint8_t>(FlingState::IDLE);
  fling_scroller_animator_ = nullptr;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
