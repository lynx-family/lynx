// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/velocity_tracker.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace testing {

#if defined(OS_HARMONY)
constexpr float kVelocityScale = 1.5f;
#else
constexpr float kVelocityScale = 1.0f;
#endif

constexpr float kMicrosPerSecond = 1000000.0f;

float VelocityFromDelta(float delta, uint64_t dt_us) {
  return delta * kMicrosPerSecond / static_cast<float>(dt_us);
}

void AddSample(VelocityTracker& tracker, const FloatPoint& position,
               uint64_t time_us, bool end = false) {
  tracker.AddPosition(position, time_us, end);
}

void AddLinearSamples(VelocityTracker& tracker, uint64_t start_time_us,
                      uint64_t dt_us, FloatPoint start, const FloatSize& step,
                      int count) {
  uint64_t time_us = start_time_us;
  FloatPoint position = start;
  for (int i = 0; i < count; ++i) {
    tracker.AddPosition(position, time_us);
    time_us += dt_us;
    position.MoveBy(step);
  }
}

class VelocityTrackerTest : public ::testing::Test {};

TEST_F(VelocityTrackerTest, GetVelocityEstimate_Empty_ReturnsZero) {
  VelocityTracker tracker;
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.width(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.height(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.movement.height(), 0.0f);
}

TEST_F(VelocityTrackerTest, GetVelocityEstimate_OneSample_ReturnsZeroVelocity) {
  VelocityTracker tracker;
  AddSample(tracker, FloatPoint(10.0f, 20.0f), 0);
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.width(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.height(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.movement.height(), 0.0f);
}

TEST_F(VelocityTrackerTest,
       GetVelocityEstimate_LinearMotionX_ComputesExpectedVelocityAndMovement) {
  VelocityTracker tracker;
  constexpr uint64_t dt_us = 10000;
  constexpr float dx = 10.0f;
  constexpr int samples = 6;
  AddLinearSamples(tracker, 0, dt_us, FloatPoint(0.0f, 0.0f),
                   FloatSize(dx, 0.0f), samples);
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  float expected_velocity = VelocityFromDelta(dx, dt_us) * kVelocityScale;
  EXPECT_NEAR(estimate.pixels_per_second.width(), expected_velocity, 1e-3f);
  EXPECT_NEAR(estimate.pixels_per_second.height(), 0.0f, 1e-3f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), dx * (samples - 1));
  EXPECT_FLOAT_EQ(estimate.movement.height(), 0.0f);
}

TEST_F(
    VelocityTrackerTest,
    GetVelocityEstimate_LinearMotionDiagonal_ComputesExpectedVelocityAndMovement) {
  VelocityTracker tracker;
  constexpr uint64_t dt_us = 20000;
  constexpr float dx = 10.0f;
  constexpr float dy = -16.0f;
  constexpr int samples = 6;
  AddLinearSamples(tracker, 0, dt_us, FloatPoint(0.0f, 0.0f), FloatSize(dx, dy),
                   samples);
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  float expected_velocity_x = VelocityFromDelta(dx, dt_us) * kVelocityScale;
  float expected_velocity_y = VelocityFromDelta(dy, dt_us) * kVelocityScale;
  EXPECT_NEAR(estimate.pixels_per_second.width(), expected_velocity_x, 1e-3f);
  EXPECT_NEAR(estimate.pixels_per_second.height(), expected_velocity_y, 1e-3f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), dx * (samples - 1));
  EXPECT_FLOAT_EQ(estimate.movement.height(), dy * (samples - 1));
}

TEST_F(VelocityTrackerTest,
       GetVelocityEstimate_OppositeDirectionRejected_ReturnsZeroVelocity) {
  VelocityTracker tracker;
  AddSample(tracker, FloatPoint(0.0f, 0.0f), 0);
  AddSample(tracker, FloatPoint(100.0f, 0.0f), 10000);
  AddSample(tracker, FloatPoint(1.0f, 0.0f), 20000);
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.width(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.height(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), 1.0f);
  EXPECT_FLOAT_EQ(estimate.movement.height(), 0.0f);
}

TEST_F(VelocityTrackerTest,
       GetVelocityEstimate_GapClearsHistory_UsesRecentSegmentOnly) {
  VelocityTracker tracker;
  constexpr uint64_t dt_us = 10000;
  constexpr uint64_t gap_us = 50000;
  uint64_t time_us = 0;
  FloatPoint position(0.0f, 0.0f);
  AddSample(tracker, position, time_us);
  time_us += dt_us;
  position.MoveBy(FloatSize(10.0f, 0.0f));
  AddSample(tracker, position, time_us);
  time_us += dt_us;
  position.MoveBy(FloatSize(10.0f, 0.0f));
  AddSample(tracker, position, time_us);
  time_us += gap_us;
  position.MoveBy(FloatSize(-20.0f, 0.0f));
  AddSample(tracker, position, time_us);
  time_us += dt_us;
  position.MoveBy(FloatSize(-20.0f, 0.0f));
  AddSample(tracker, position, time_us);
  time_us += dt_us;
  position.MoveBy(FloatSize(-20.0f, 0.0f));
  AddSample(tracker, position, time_us);
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  float expected_velocity = VelocityFromDelta(-20.0f, dt_us) * kVelocityScale;
  EXPECT_NEAR(estimate.pixels_per_second.width(), expected_velocity, 1e-2f);
  EXPECT_NEAR(estimate.pixels_per_second.height(), 0.0f, 1e-3f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), -40.0f);
  EXPECT_FLOAT_EQ(estimate.movement.height(), 0.0f);
}

TEST_F(VelocityTrackerTest,
       GetVelocityEstimate_EndTrueLongGap_ClearsAndDoesNotAddSample) {
  VelocityTracker tracker;
  AddSample(tracker, FloatPoint(0.0f, 0.0f), 0);
  AddSample(tracker, FloatPoint(10.0f, 0.0f), 10000);
  AddSample(tracker, FloatPoint(10.0f, 0.0f), 90000, true);
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.width(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.pixels_per_second.height(), 0.0f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), 10.0f);
  EXPECT_FLOAT_EQ(estimate.movement.height(), 0.0f);
}

TEST_F(VelocityTrackerTest,
       GetVelocityEstimate_StationaryTail_DrivesVelocityToZero) {
  VelocityTracker tracker;
  constexpr uint64_t dt_us = 10000;
  constexpr float dx = 10.0f;
  constexpr int moving_samples = 6;
  uint64_t time_us = 0;
  FloatPoint position(0.0f, 0.0f);
  for (int i = 0; i < moving_samples; ++i) {
    AddSample(tracker, position, time_us);
    time_us += dt_us;
    if (i < moving_samples - 1) {
      position.MoveBy(FloatSize(dx, 0.0f));
    }
  }
  for (int i = 0; i < 20; ++i) {
    AddSample(tracker, position, time_us);
    time_us += dt_us;
  }
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  EXPECT_NEAR(estimate.pixels_per_second.width(), 0.0f, 1.0f);
  EXPECT_NEAR(estimate.pixels_per_second.height(), 0.0f, 1.0f);
  EXPECT_FLOAT_EQ(estimate.movement.width(), dx * (moving_samples - 1));
  EXPECT_FLOAT_EQ(estimate.movement.height(), 0.0f);
}

TEST_F(VelocityTrackerTest,
       GetVelocityEstimate_IncreasingSpeed_EstimateExceedsLastSample) {
  VelocityTracker tracker;
  constexpr uint64_t dt_us = 10000;
  constexpr int samples = 6;
  constexpr float dt_s = static_cast<float>(dt_us) / kMicrosPerSecond;

  constexpr float b0_x = 1000.0f;
  constexpr float b1_x = 1000.0f;
  constexpr float b2_x = 5000.0f;

  constexpr float b0_y = 500.0f;
  constexpr float b1_y = 2000.0f;
  constexpr float b2_y = 3000.0f;

  uint64_t time_us = 0;
  float x_prev = 0.0f;
  float y_prev = 0.0f;
  for (int i = 0; i < samples; ++i) {
    const int age_steps = (samples - 1) - i;
    const float t = -static_cast<float>(age_steps) * dt_s;
    const float x = b0_x + b1_x * t + b2_x * t * t;
    const float y = b0_y + b1_y * t + b2_y * t * t;
    AddSample(tracker, FloatPoint(x, y), time_us);
    if (i == samples - 2) {
      x_prev = x;
      y_prev = y;
    }
    time_us += dt_us;
  }

  const float x_newest = b0_x;
  const float y_newest = b0_y;
  const float last_velocity_x =
      VelocityFromDelta(x_newest - x_prev, dt_us) * kVelocityScale;
  const float last_velocity_y =
      VelocityFromDelta(y_newest - y_prev, dt_us) * kVelocityScale;
  VelocityEstimate estimate = tracker.GetVelocityEstimate();
  EXPECT_GT(estimate.pixels_per_second.width(), last_velocity_x);
  EXPECT_GT(estimate.pixels_per_second.height(), last_velocity_y);
}

};  // namespace testing

};  // namespace clay
