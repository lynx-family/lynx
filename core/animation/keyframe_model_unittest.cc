// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>
#include <memory>

#include "core/animation/keyframed_animation_curve.h"
#include "gfx/animation/animation_keyframe_model.h"
#include "gfx/animation/animation_timing.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace animation {
namespace tasm {
namespace test {

std::unique_ptr<gfx::KeyframeModel> InitTestModel() {
  auto test_curve = KeyframedOpacityAnimationCurve::Create();
  auto test_frame1 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(0.0), nullptr);
  test_frame1->SetOpacity(1.0f);
  test_curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetOpacity(0.0f);
  test_curve->AddKeyframe(std::move(test_frame2));
  return gfx::KeyframeModel::Create(std::move(test_curve));
}

fml::TimePoint TimePointFromMs(int64_t milliseconds) {
  return fml::TimePoint::FromEpochDelta(
      fml::TimeDelta::FromMilliseconds(milliseconds));
}

gfx::AnimationData MakeTimingData(
    int iteration_count = 1,
    gfx::AnimationFillModeType fill_mode = gfx::AnimationFillModeType::kNone,
    gfx::AnimationDirectionType direction =
        gfx::AnimationDirectionType::kNormal,
    long delay = 0) {
  gfx::AnimationData data;
  data.duration = 1000;
  data.delay = delay;
  data.iteration_count = iteration_count;
  data.fill_mode = fill_mode;
  data.direction = direction;
  return data;
}

gfx::AnimationTimingInput MakeTimingInput(
    const gfx::AnimationData* data,
    fml::TimeDelta duration = fml::TimeDelta::FromMilliseconds(1000)) {
  gfx::AnimationTimingInput input;
  input.animation_data = data;
  input.duration = duration;
  input.start_time = TimePointFromMs(0);
  return input;
}

TEST(AnimationTimingTest, RepeatDurationHandlesZeroInfiniteAndOverflow) {
  EXPECT_EQ(
      fml::TimeDelta::Zero(),
      gfx::GetRepeatDuration(nullptr, fml::TimeDelta::FromMilliseconds(1000)));

  gfx::AnimationData data = MakeTimingData(0);
  EXPECT_EQ(
      fml::TimeDelta::Zero(),
      gfx::GetRepeatDuration(&data, fml::TimeDelta::FromMilliseconds(1000)));

  data.iteration_count = 3;
  EXPECT_EQ(
      fml::TimeDelta::FromMilliseconds(1500),
      gfx::GetRepeatDuration(&data, fml::TimeDelta::FromMilliseconds(500)));

  data.iteration_count = -1;
  EXPECT_EQ(
      fml::TimeDelta::Max(),
      gfx::GetRepeatDuration(&data, fml::TimeDelta::FromMilliseconds(500)));

  data.iteration_count = 2;
  EXPECT_EQ(fml::TimeDelta::Max(),
            gfx::GetRepeatDuration(
                &data, fml::TimeDelta::FromNanoseconds(
                           std::numeric_limits<int64_t>::max() / 2)));
}

TEST(AnimationTimingTest, LocalTimeAndPhaseRespectPauseDelayAndPlayback) {
  gfx::AnimationData data =
      MakeTimingData(2, gfx::AnimationFillModeType::kNone,
                     gfx::AnimationDirectionType::kNormal, 500);
  auto input = MakeTimingInput(&data);
  input.start_time = TimePointFromMs(1000);
  input.total_paused_duration = fml::TimeDelta::FromMilliseconds(200);

  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(1300),
            gfx::ConvertMonotonicTimeToLocalTime(input, TimePointFromMs(2500)));

  input.is_paused = true;
  input.pause_time = TimePointFromMs(1800);
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(600),
            gfx::ConvertMonotonicTimeToLocalTime(input, TimePointFromMs(2500)));

  input.is_paused = false;
  EXPECT_EQ(gfx::TimingPhase::BEFORE,
            gfx::CalculatePhase(input, fml::TimeDelta::FromMilliseconds(499)));
  EXPECT_EQ(gfx::TimingPhase::ACTIVE,
            gfx::CalculatePhase(input, fml::TimeDelta::FromMilliseconds(500)));
  EXPECT_EQ(gfx::TimingPhase::AFTER,
            gfx::CalculatePhase(input, fml::TimeDelta::FromMilliseconds(2500)));
  EXPECT_EQ(gfx::TimingPhase::ACTIVE,
            gfx::CalculatePhase(input, TimePointFromMs(1700)));

  input.playback_rate = -1.0;
  EXPECT_EQ(gfx::TimingPhase::BEFORE,
            gfx::CalculatePhase(input, fml::TimeDelta::FromMilliseconds(500)));
  EXPECT_EQ(gfx::TimingPhase::ACTIVE,
            gfx::CalculatePhase(input, fml::TimeDelta::FromMilliseconds(2500)));
}

TEST(AnimationTimingTest, ActiveTimeAndInEffectRespectFillModes) {
  gfx::AnimationData data =
      MakeTimingData(1, gfx::AnimationFillModeType::kNone,
                     gfx::AnimationDirectionType::kNormal, 500);
  auto input = MakeTimingInput(&data);

  EXPECT_EQ(fml::TimeDelta::Min(),
            gfx::CalculateActiveTime(input, TimePointFromMs(200)));
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(100),
            gfx::CalculateActiveTime(input, TimePointFromMs(600)));
  EXPECT_EQ(fml::TimeDelta::Min(),
            gfx::CalculateActiveTime(input, TimePointFromMs(1600)));
  EXPECT_FALSE(gfx::IsInEffect(input, TimePointFromMs(200)));

  data.fill_mode = gfx::AnimationFillModeType::kBackwards;
  EXPECT_EQ(fml::TimeDelta::Zero(),
            gfx::CalculateActiveTime(input, TimePointFromMs(200)));
  EXPECT_EQ(fml::TimeDelta::Min(),
            gfx::CalculateActiveTime(input, TimePointFromMs(1600)));

  data.fill_mode = gfx::AnimationFillModeType::kForwards;
  EXPECT_EQ(fml::TimeDelta::Min(),
            gfx::CalculateActiveTime(input, TimePointFromMs(200)));
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(1000),
            gfx::CalculateActiveTime(input, TimePointFromMs(1600)));

  data.fill_mode = gfx::AnimationFillModeType::kBoth;
  EXPECT_TRUE(gfx::IsInEffect(input, TimePointFromMs(200)));
  EXPECT_TRUE(gfx::IsInEffect(input, TimePointFromMs(1600)));
}

TEST(AnimationTimingTest, UpdateTimingStateCoversRunStateTransitions) {
  gfx::AnimationData data =
      MakeTimingData(1, gfx::AnimationFillModeType::kNone,
                     gfx::AnimationDirectionType::kNormal, 500);
  auto input = MakeTimingInput(&data);

  input.run_state = gfx::TimingRunState::STARTING;
  auto update = gfx::UpdateTimingState(input, TimePointFromMs(600));
  EXPECT_EQ(gfx::TimingRunState::RUNNING, update.run_state);
  EXPECT_TRUE(update.start_event_due);
  EXPECT_FALSE(update.end_event_due);

  update = gfx::UpdateTimingState(input, TimePointFromMs(1600));
  EXPECT_EQ(gfx::TimingRunState::FINISHED, update.run_state);
  EXPECT_TRUE(update.start_event_due);
  EXPECT_TRUE(update.end_event_due);

  input.run_state = gfx::TimingRunState::RUNNING;
  update = gfx::UpdateTimingState(input, TimePointFromMs(200));
  EXPECT_EQ(gfx::TimingRunState::STARTING, update.run_state);
  EXPECT_FALSE(update.start_event_due);
  EXPECT_TRUE(update.end_event_due);

  update = gfx::UpdateTimingState(input, TimePointFromMs(1600));
  EXPECT_EQ(gfx::TimingRunState::FINISHED, update.run_state);
  EXPECT_FALSE(update.start_event_due);
  EXPECT_TRUE(update.end_event_due);

  input.run_state = gfx::TimingRunState::PAUSED;
  EXPECT_EQ(gfx::TimingRunState::STARTING,
            gfx::UpdateTimingState(input, TimePointFromMs(200)).run_state);
  EXPECT_EQ(gfx::TimingRunState::RUNNING,
            gfx::UpdateTimingState(input, TimePointFromMs(600)).run_state);
  EXPECT_EQ(gfx::TimingRunState::FINISHED,
            gfx::UpdateTimingState(input, TimePointFromMs(1600)).run_state);

  input.run_state = gfx::TimingRunState::FINISHED;
  update = gfx::UpdateTimingState(input, TimePointFromMs(200));
  EXPECT_EQ(gfx::TimingRunState::STARTING, update.run_state);
  EXPECT_FALSE(update.start_event_due);
  EXPECT_FALSE(update.end_event_due);

  update = gfx::UpdateTimingState(input, TimePointFromMs(600));
  EXPECT_EQ(gfx::TimingRunState::RUNNING, update.run_state);
  EXPECT_TRUE(update.start_event_due);
  EXPECT_FALSE(update.end_event_due);
}

TEST(AnimationTimingTest, TrimTimeHandlesDirectionPlaybackAndInvalidInput) {
  gfx::AnimationData data =
      MakeTimingData(3, gfx::AnimationFillModeType::kBoth);
  auto input = MakeTimingInput(&data);

  auto trimmed =
      gfx::TrimTimeToCurrentIteration(input, TimePointFromMs(2200), 0);
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(200), trimmed.time);
  EXPECT_EQ(2, trimmed.current_iteration_count);

  data.direction = gfx::AnimationDirectionType::kReverse;
  trimmed = gfx::TrimTimeToCurrentIteration(input, TimePointFromMs(2200), 0);
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(800), trimmed.time);
  EXPECT_EQ(2, trimmed.current_iteration_count);

  data.direction = gfx::AnimationDirectionType::kAlternate;
  trimmed = gfx::TrimTimeToCurrentIteration(input, TimePointFromMs(1200), 0);
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(800), trimmed.time);
  EXPECT_EQ(1, trimmed.current_iteration_count);

  data.direction = gfx::AnimationDirectionType::kAlternateReverse;
  trimmed = gfx::TrimTimeToCurrentIteration(input, TimePointFromMs(200), 0);
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(800), trimmed.time);
  EXPECT_EQ(0, trimmed.current_iteration_count);

  input.playback_rate = -1.0;
  data.direction = gfx::AnimationDirectionType::kNormal;
  trimmed = gfx::TrimTimeToCurrentIteration(input, TimePointFromMs(1000), 0);
  EXPECT_EQ(fml::TimeDelta::Zero(), trimmed.time);
  EXPECT_EQ(2, trimmed.current_iteration_count);

  input.animation_data = nullptr;
  trimmed = gfx::TrimTimeToCurrentIteration(input, TimePointFromMs(1000), 2);
  EXPECT_EQ(fml::TimeDelta::Zero(), trimmed.time);
  EXPECT_EQ(0, trimmed.current_iteration_count);
}

TEST(KeyframeModelTest, GetRepeatDuration) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 0;
  test_model->SetAnimationData(&default_data);
  auto result_1 = test_model->GetRepeatDuration();
  EXPECT_EQ(result_1, fml::TimeDelta::Zero());

  default_data.duration = 1234;
  default_data.delay = 1000;
  default_data.iteration_count = 10;
  test_model->SetAnimationData(&default_data);
  auto result_2 = test_model->GetRepeatDuration();
  EXPECT_EQ(result_2, fml::TimeDelta::FromMilliseconds(12340));
}

TEST(KeyframeModelTest, CalculatePhase) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 2;
  test_model->SetAnimationData(&default_data);

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.0f));
  test_model->set_start_time(start_time);
  test_model->SetRunState(gfx::KeyframeModel::STARTING, start_time);

  auto phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(-1.0));
  EXPECT_EQ(gfx::KeyframeModel::Phase::BEFORE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(0.5));
  EXPECT_EQ(gfx::KeyframeModel::Phase::BEFORE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(1.5));
  EXPECT_EQ(gfx::KeyframeModel::Phase::ACTIVE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(2.5));
  EXPECT_EQ(gfx::KeyframeModel::Phase::ACTIVE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(3.1));
  EXPECT_EQ(gfx::KeyframeModel::Phase::AFTER, phase);

  default_data.iteration_count = -1;
  test_model->SetAnimationData(&default_data);
  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(3.1));
  EXPECT_EQ(gfx::KeyframeModel::Phase::ACTIVE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(999999999));
  EXPECT_EQ(gfx::KeyframeModel::Phase::ACTIVE, phase);

  default_data.duration = 1000;
  default_data.delay = 0;
  default_data.iteration_count = 2;
  test_model->SetAnimationData(&default_data);
  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(2.5));
  EXPECT_EQ(gfx::KeyframeModel::Phase::AFTER, phase);

  default_data.duration = 1000;
  default_data.delay = 2000;
  default_data.iteration_count = 1;
  test_model->SetAnimationData(&default_data);
  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(1.5));
  EXPECT_EQ(gfx::KeyframeModel::Phase::BEFORE, phase);
  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(3.1));
  EXPECT_EQ(gfx::KeyframeModel::Phase::AFTER, phase);
}

TEST(KeyframeModelTest, CalculateActiveTime) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 1;
  default_data.fill_mode = gfx::AnimationFillModeType::kBoth;
  test_model->SetAnimationData(&default_data);

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  test_model->set_start_time(start_time);
  test_model->SetRunState(gfx::KeyframeModel::STARTING, start_time);

  fml::TimeDelta active_time = test_model->CalculateActiveTime(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(500)));
  EXPECT_EQ(active_time, fml::TimeDelta());
  active_time = test_model->CalculateActiveTime(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(1100)));
  EXPECT_EQ(active_time, fml::TimeDelta::FromMilliseconds(100));
  active_time = test_model->CalculateActiveTime(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(1500)));
  EXPECT_EQ(active_time, fml::TimeDelta::FromMilliseconds(500));
  active_time = test_model->CalculateActiveTime(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(2100)));
  EXPECT_EQ(active_time, fml::TimeDelta::FromMilliseconds(1000));

  default_data.fill_mode = gfx::AnimationFillModeType::kNone;
  test_model->SetAnimationData(&default_data);
  active_time = test_model->CalculateActiveTime(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(500)));
  EXPECT_EQ(active_time, fml::TimeDelta::Min());
  active_time = test_model->CalculateActiveTime(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(1100)));
  EXPECT_EQ(active_time, fml::TimeDelta::FromMilliseconds(100));
  active_time = test_model->CalculateActiveTime(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(2100)));
  EXPECT_EQ(active_time, fml::TimeDelta::Min());
}

TEST(KeyframeModelTest, TrimTimeToCurrentIteration) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 3;
  default_data.fill_mode = gfx::AnimationFillModeType::kBoth;
  test_model->SetAnimationData(&default_data);

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  test_model->set_start_time(start_time);
  test_model->SetRunState(gfx::KeyframeModel::STARTING, start_time);

  int iteration_count = 0;

  fml::TimePoint test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(-1000));
  fml::TimeDelta trimmed_time =
      test_model->TrimTimeToCurrentIteration(test_time, iteration_count);
  EXPECT_EQ(trimmed_time, fml::TimeDelta());
  EXPECT_EQ(iteration_count, 0);

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(500));
  trimmed_time =
      test_model->TrimTimeToCurrentIteration(test_time, iteration_count);
  EXPECT_EQ(trimmed_time, fml::TimeDelta());
  EXPECT_EQ(iteration_count, 0);

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(1500));
  trimmed_time =
      test_model->TrimTimeToCurrentIteration(test_time, iteration_count);
  EXPECT_EQ(trimmed_time, fml::TimeDelta::FromMilliseconds(500));
  EXPECT_EQ(iteration_count, 0);

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(2100));
  trimmed_time =
      test_model->TrimTimeToCurrentIteration(test_time, iteration_count);
  EXPECT_EQ(trimmed_time, fml::TimeDelta::FromMilliseconds(100));
  EXPECT_EQ(iteration_count, 1);

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(3100));
  trimmed_time =
      test_model->TrimTimeToCurrentIteration(test_time, iteration_count);
  EXPECT_EQ(trimmed_time, fml::TimeDelta::FromMilliseconds(100));
  EXPECT_EQ(iteration_count, 2);

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(4100));
  trimmed_time =
      test_model->TrimTimeToCurrentIteration(test_time, iteration_count);
  EXPECT_EQ(trimmed_time, fml::TimeDelta::FromMilliseconds(1000));
  EXPECT_EQ(iteration_count, 2);
}

TEST(KeyframeModelTest, CountIterationEventsDueOnlyForForwardProgress) {
  EXPECT_EQ(gfx::CountIterationEventsDue(1, 3), 2);
  EXPECT_EQ(gfx::CountIterationEventsDue(2, 2), 0);
  EXPECT_EQ(gfx::CountIterationEventsDue(3, 1), 0);
}

TEST(KeyframeModelTest, InEffect) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 3;
  default_data.fill_mode = gfx::AnimationFillModeType::kBoth;
  test_model->SetAnimationData(&default_data);

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  test_model->set_start_time(start_time);
  test_model->SetRunState(gfx::KeyframeModel::STARTING, start_time);

  fml::TimePoint test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(-1.0f));
  EXPECT_TRUE(test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
  EXPECT_TRUE(test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
  EXPECT_TRUE(test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
  EXPECT_TRUE(test_model->InEffect(test_time));

  default_data.fill_mode = gfx::AnimationFillModeType::kNone;
  test_model->SetAnimationData(&default_data);

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(-1.0f));
  EXPECT_FALSE(test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
  EXPECT_FALSE(test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
  EXPECT_TRUE(test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
  EXPECT_FALSE(test_model->InEffect(test_time));
}

TEST(KeyframeModelTest, SetRunState) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  test_model->SetAnimationData(&default_data);

  fml::TimePoint base_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));

  test_model->SetRunState(gfx::KeyframeModel::STARTING, base_time);
  EXPECT_EQ(gfx::KeyframeModel::STARTING, test_model->GetRunState());

  test_model->SetRunState(gfx::KeyframeModel::RUNNING, base_time);
  EXPECT_EQ(gfx::KeyframeModel::RUNNING, test_model->GetRunState());

  test_model->SetRunState(gfx::KeyframeModel::PAUSED, base_time);
  EXPECT_EQ(gfx::KeyframeModel::PAUSED, test_model->GetRunState());
  EXPECT_EQ(base_time, test_model->pause_time());

  fml::TimePoint run_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0f));
  test_model->SetRunState(gfx::KeyframeModel::STARTING, run_time);
  EXPECT_EQ(gfx::KeyframeModel::STARTING, test_model->GetRunState());
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(1.0f),
            test_model->total_paused_duration());

  fml::TimePoint base_time1 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.0f));
  test_model->SetRunState(gfx::KeyframeModel::PAUSED, base_time1);
  EXPECT_EQ(gfx::KeyframeModel::PAUSED, test_model->GetRunState());
  EXPECT_EQ(base_time1, test_model->pause_time());

  fml::TimePoint run_time1 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.0f));
  test_model->SetRunState(gfx::KeyframeModel::RUNNING, run_time1);
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(2.0f),
            test_model->total_paused_duration());

  test_model->SetRunState(gfx::KeyframeModel::FINISHED, run_time1);
  EXPECT_EQ(gfx::KeyframeModel::FINISHED, test_model->GetRunState());

  fml::TimePoint base_time2 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.0f));
  test_model->SetRunState(gfx::KeyframeModel::PAUSED, base_time2);
  EXPECT_EQ(gfx::KeyframeModel::PAUSED, test_model->GetRunState());
  EXPECT_EQ(base_time2, test_model->pause_time());

  fml::TimePoint run_time2 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(6.0f));
  test_model->SetRunState(gfx::KeyframeModel::FINISHED, run_time2);
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(3.0f),
            test_model->total_paused_duration());
}

TEST(KeyframeModelTest, SetAnimationData) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  test_model->SetAnimationData(&default_data);

  default_data.duration = 2000;
  test_model->SetAnimationData(&default_data);
  EXPECT_EQ(2.0, test_model->curve()->scaled_duration());
}

TEST(KeyframeModelTest, NullCurveUsesDefaultTimingSafely) {
  std::unique_ptr<gfx::AnimationCurve> empty_curve;
  auto test_model = gfx::KeyframeModel::Create(std::move(empty_curve));
  EXPECT_EQ(nullptr, test_model->curve());

  gfx::AnimationData data = MakeTimingData(2);
  test_model->SetAnimationData(&data);
  EXPECT_EQ(fml::TimeDelta::Zero(), test_model->GetRepeatDuration());

  fml::TimePoint start_time = TimePointFromMs(1000);
  test_model->set_start_time(start_time);
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(500),
            test_model->ConvertMonotonicTimeToLocalTime(TimePointFromMs(1500)));

  int iteration_count = 3;
  EXPECT_EQ(fml::TimeDelta::Zero(),
            test_model->TrimTimeToCurrentIteration(TimePointFromMs(1500),
                                                   iteration_count));
  EXPECT_EQ(0, iteration_count);

  test_model->SetAnimationData(nullptr);
  EXPECT_EQ(fml::TimeDelta::Zero(), test_model->GetRepeatDuration());
  test_model->EnsureFromAndToKeyframe();
}

TEST(KeyframeModelTest, TimingInputUsesPauseAndPlaybackState) {
  auto test_model = InitTestModel();
  gfx::AnimationData data =
      MakeTimingData(3, gfx::AnimationFillModeType::kBoth);
  test_model->SetAnimationData(&data);

  fml::TimePoint start_time = TimePointFromMs(1000);
  test_model->set_start_time(start_time);
  test_model->SetRunState(gfx::KeyframeModel::STARTING, start_time);

  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(1200),
            test_model->ConvertMonotonicTimeToLocalTime(TimePointFromMs(2200)));

  fml::TimePoint pause_time = TimePointFromMs(1800);
  test_model->SetRunState(gfx::KeyframeModel::PAUSED, pause_time);
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(800),
            test_model->ConvertMonotonicTimeToLocalTime(TimePointFromMs(3000)));

  test_model->SetRunState(gfx::KeyframeModel::RUNNING, TimePointFromMs(2300));
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(1500),
            test_model->ConvertMonotonicTimeToLocalTime(TimePointFromMs(3000)));

  test_model->set_playback_rate(-1.0);
  int iteration_count = 0;
  EXPECT_EQ(fml::TimeDelta::Zero(),
            test_model->TrimTimeToCurrentIteration(TimePointFromMs(2500),
                                                   iteration_count));
  EXPECT_EQ(2, iteration_count);
}

TEST(KeyframeModelTest, EnsureFromAndToKeyframe) {
  auto test_curve = KeyframedOpacityAnimationCurve::Create();
  auto test_frame1 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(0.3), nullptr);
  test_frame1->SetOpacity(1.0f);
  test_curve->AddKeyframe(std::move(test_frame1));

  auto test_frame2 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(0.7), nullptr);
  test_frame2->SetOpacity(0.0f);
  test_curve->AddKeyframe(std::move(test_frame2));

  auto test_model = gfx::KeyframeModel::Create(std::move(test_curve));
  gfx::AnimationData default_data = gfx::AnimationData();
  test_model->SetAnimationData(&default_data);
  test_model->EnsureFromAndToKeyframe();
  EXPECT_EQ(4ul, test_model->curve()->get_keyframes_size());
}

TEST(KeyframeModelTest, UpdateState) {
  auto test_model = InitTestModel();
  gfx::AnimationData default_data = gfx::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 1;
  test_model->SetAnimationData(&default_data);

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.0f));
  test_model->set_start_time(start_time);
  test_model->SetRunState(gfx::KeyframeModel::STARTING, start_time);

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
  }

  {
    fml::TimePoint paused_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
    test_model->SetRunState(gfx::KeyframeModel::PAUSED, paused_time);
    EXPECT_EQ(gfx::KeyframeModel::PAUSED, test_model->GetRunState());
    EXPECT_EQ(paused_time, test_model->pause_time());
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
    EXPECT_EQ(fml::TimeDelta::FromSecondsF(1.0f),
              test_model->total_paused_duration());
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.1f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_TRUE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
  }

  {
    fml::TimePoint paused_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.5f));
    test_model->SetRunState(gfx::KeyframeModel::PAUSED, paused_time);
    EXPECT_EQ(gfx::KeyframeModel::PAUSED, test_model->GetRunState());
    EXPECT_EQ(paused_time, test_model->pause_time());
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
    EXPECT_EQ(fml::TimeDelta::FromSecondsF(2.0f),
              test_model->total_paused_duration());
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.1f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_TRUE(should_send_end_event);
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
  }

  {
    fml::TimePoint paused_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
    test_model->SetRunState(gfx::KeyframeModel::PAUSED, paused_time);
    EXPECT_EQ(gfx::KeyframeModel::PAUSED, test_model->GetRunState());
    EXPECT_EQ(paused_time, test_model->pause_time());
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
    EXPECT_EQ(fml::TimeDelta::FromSecondsF(3.0f),
              test_model->total_paused_duration());
  }

  gfx::AnimationData data1 = gfx::AnimationData();
  data1.duration = 0;
  data1.delay = 5000;
  data1.iteration_count = 1;
  test_model->SetAnimationData(&data1);

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(6.0f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
  }

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(8.1f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_TRUE(should_send_start_event);
    EXPECT_TRUE(should_send_end_event);
  }

  gfx::AnimationData data2 = gfx::AnimationData();
  data2.duration = 6000;
  data2.delay = 0;
  data2.iteration_count = 1;
  test_model->SetAnimationData(&data2);

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(8.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_TRUE(should_send_start_event);
    EXPECT_FALSE(should_send_end_event);
  }

  gfx::AnimationData data3 = gfx::AnimationData();
  data3.duration = 1000;
  data3.delay = 7000;
  data3.iteration_count = 1;
  test_model->SetAnimationData(&data3);

  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(9.0f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(gfx::KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_FALSE(should_send_start_event);
    EXPECT_TRUE(should_send_end_event);
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace animation
}  // namespace lynx
