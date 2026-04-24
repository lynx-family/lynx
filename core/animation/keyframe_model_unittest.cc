// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/animation/keyframed_animation_curve.h"
#include "gfx/animation/animation_keyframe_model.h"
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
