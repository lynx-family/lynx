// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/animation/keyframe_model.h"

#include <memory>

#include "core/animation/animation.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/starlight/types/nlength.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace animation {
namespace tasm {
namespace test {
std::unique_ptr<KeyframeModel> InitTestModel() {
  std::unique_ptr<KeyframedOpacityAnimationCurve> test_curve(
      KeyframedOpacityAnimationCurve::Create());
  auto test_frame1 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(0.0), nullptr);
  test_frame1->SetOpacity(1.0f);
  test_curve->AddKeyframe(std::move(test_frame1));
  test_curve->type_ = AnimationCurve::CurveType::OPACITY;
  auto test_frame2 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame2->SetOpacity(0.0f);
  test_curve->AddKeyframe(std::move(test_frame2));
  return std::make_unique<KeyframeModel>(std::move(test_curve));
}

TEST(KeyframeModelTest, GetRepeatDuration) {
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  starlight::AnimationData default_data = starlight::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 0;
  test_model->UpdateAnimationData(&default_data);
  auto result_1 = test_model->GetRepeatDuration();
  EXPECT_EQ(result_1, fml::TimeDelta::Zero());

  default_data.duration = 1234;
  default_data.delay = 1000;
  default_data.iteration_count = 10;
  test_model->UpdateAnimationData(&default_data);
  auto result_3 = test_model->GetRepeatDuration();
  EXPECT_EQ(result_3, fml::TimeDelta::FromMilliseconds(12340));
}

TEST(KeyframeModelTest, CalculatePhase) {
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  starlight::AnimationData default_data = starlight::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 2;
  test_model->UpdateAnimationData(&default_data);
  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.0f));
  test_model->set_start_time(start_time);
  test_model->SetRunState(KeyframeModel::STARTING, start_time);

  KeyframeModel::Phase phase =
      test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(-1.0));
  EXPECT_EQ(KeyframeModel::Phase::BEFORE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(0.5));
  EXPECT_EQ(KeyframeModel::Phase::BEFORE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(1.5));
  EXPECT_EQ(KeyframeModel::Phase::ACTIVE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(2.5));
  EXPECT_EQ(KeyframeModel::Phase::ACTIVE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(3.1));
  EXPECT_EQ(KeyframeModel::Phase::AFTER, phase);

  // Negative iterations_ represents "infinite iterations".
  default_data.iteration_count = -1;
  test_model->UpdateAnimationData(&default_data);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(3.1));
  EXPECT_EQ(KeyframeModel::Phase::ACTIVE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(999999999));
  EXPECT_EQ(KeyframeModel::Phase::ACTIVE, phase);

  // Negative duration test
  default_data.duration = 1000;
  default_data.delay = 0;
  default_data.iteration_count = 2;
  test_model->UpdateAnimationData(&default_data);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(2.5));
  EXPECT_EQ(KeyframeModel::Phase::AFTER, phase);

  default_data.duration = 1000;
  default_data.delay = 2000;
  default_data.iteration_count = 1;
  test_model->UpdateAnimationData(&default_data);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(1.5));
  EXPECT_EQ(KeyframeModel::Phase::BEFORE, phase);

  phase = test_model->CalculatePhase(fml::TimeDelta::FromSecondsF(3.1));
  EXPECT_EQ(KeyframeModel::Phase::AFTER, phase);
}

TEST(KeyframeModelTest, CalculateActiveTime) {
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  starlight::AnimationData default_data = starlight::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 1;
  default_data.fill_mode = starlight::AnimationFillModeType::kBoth;
  test_model->UpdateAnimationData(&default_data);
  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  test_model->set_start_time(start_time);
  test_model->SetRunState(KeyframeModel::STARTING, start_time);

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

  default_data.fill_mode = starlight::AnimationFillModeType::kNone;
  test_model->UpdateAnimationData(&default_data);
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
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  starlight::AnimationData default_data = starlight::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 3;
  default_data.fill_mode = starlight::AnimationFillModeType::kBoth;
  test_model->UpdateAnimationData(&default_data);
  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  test_model->set_start_time(start_time);
  test_model->SetRunState(KeyframeModel::STARTING, start_time);

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
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  starlight::AnimationData default_data = starlight::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 3;
  default_data.fill_mode = starlight::AnimationFillModeType::kBoth;
  test_model->UpdateAnimationData(&default_data);
  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  test_model->set_start_time(start_time);
  test_model->SetRunState(KeyframeModel::STARTING, start_time);

  fml::TimePoint test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(-1.0f));
  EXPECT_EQ(true, test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
  EXPECT_EQ(true, test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
  EXPECT_EQ(true, test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
  EXPECT_EQ(true, test_model->InEffect(test_time));

  default_data.fill_mode = starlight::AnimationFillModeType::kNone;
  test_model->UpdateAnimationData(&default_data);

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(-1.0f));
  EXPECT_EQ(false, test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
  EXPECT_EQ(false, test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
  EXPECT_EQ(true, test_model->InEffect(test_time));

  test_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
  EXPECT_EQ(false, test_model->InEffect(test_time));
}

TEST(KeyframeModelTest, SetRunState) {
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  std::unique_ptr<starlight::AnimationData> default_data =
      std::make_unique<starlight::AnimationData>();
  // default_data->delay=2.0;
  test_model->set_animation_data(default_data.get());

  fml::TimePoint base_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));

  test_model->SetRunState(KeyframeModel::STARTING, base_time);
  EXPECT_EQ(KeyframeModel::STARTING, test_model->GetRunState());

  test_model->SetRunState(KeyframeModel::RUNNING, base_time);
  EXPECT_EQ(KeyframeModel::RUNNING, test_model->GetRunState());

  test_model->SetRunState(KeyframeModel::PAUSED, base_time);
  EXPECT_EQ(KeyframeModel::PAUSED, test_model->GetRunState());
  EXPECT_EQ(base_time, test_model->pause_time());

  fml::TimePoint run_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0f));
  test_model->SetRunState(KeyframeModel::STARTING, run_time);
  EXPECT_EQ(KeyframeModel::STARTING, test_model->GetRunState());
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(1.0f),
            test_model->total_paused_duration());

  fml::TimePoint base_time1 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.0f));

  test_model->SetRunState(KeyframeModel::PAUSED, base_time1);
  EXPECT_EQ(KeyframeModel::PAUSED, test_model->GetRunState());
  EXPECT_EQ(base_time1, test_model->pause_time());

  fml::TimePoint run_time1 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.0f));
  test_model->SetRunState(KeyframeModel::RUNNING, run_time1);
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(2.0f),
            test_model->total_paused_duration());

  test_model->SetRunState(KeyframeModel::FINISHED, run_time1);
  EXPECT_EQ(KeyframeModel::FINISHED, test_model->GetRunState());

  fml::TimePoint base_time2 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.0f));

  test_model->SetRunState(KeyframeModel::PAUSED, base_time2);
  EXPECT_EQ(KeyframeModel::PAUSED, test_model->GetRunState());
  EXPECT_EQ(base_time2, test_model->pause_time());

  fml::TimePoint run_time2 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(6.0f));
  test_model->SetRunState(KeyframeModel::FINISHED, run_time2);
  EXPECT_EQ(fml::TimeDelta::FromSecondsF(3.0f),
            test_model->total_paused_duration());
}

TEST(KeyframeModelTest, UpdateAnimationData) {
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  std::unique_ptr<starlight::AnimationData> default_data =
      std::make_unique<starlight::AnimationData>();
  test_model->set_animation_data(default_data.get());
  default_data->duration = 2000.0;
  test_model->UpdateAnimationData(default_data.get());

  EXPECT_EQ(2.0, test_model->animation_curve()->scaled_duration());
}

TEST(KeyframeModelTest, EnsureFromAndToKeyframe) {
  std::unique_ptr<KeyframedOpacityAnimationCurve> test_curve(
      KeyframedOpacityAnimationCurve::Create());
  auto test_frame1 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(0.3), nullptr);
  test_frame1->SetOpacity(1.0f);
  test_curve->AddKeyframe(std::move(test_frame1));
  test_curve->type_ = AnimationCurve::CurveType::OPACITY;
  auto test_frame2 =
      OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(0.7), nullptr);
  test_frame2->SetOpacity(0.0f);
  test_curve->AddKeyframe(std::move(test_frame2));
  auto test_model = std::make_unique<KeyframeModel>(std::move(test_curve));

  std::unique_ptr<starlight::AnimationData> default_data =
      std::make_unique<starlight::AnimationData>();
  test_model->set_animation_data(default_data.get());

  test_model->animation_curve()->EnsureFromAndToKeyframe();
  EXPECT_EQ(4ul, test_model->animation_curve()->get_keyframes_size());
}

TEST(KeyframeModelTest, UpdateState) {
  std::unique_ptr<KeyframeModel> test_model = InitTestModel();
  starlight::AnimationData default_data = starlight::AnimationData();
  default_data.duration = 1000;
  default_data.delay = 1000;
  default_data.iteration_count = 1;
  test_model->UpdateAnimationData(&default_data);

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.0f));
  test_model->set_start_time(start_time);
  test_model->SetRunState(KeyframeModel::STARTING, start_time);

  // 0.5s stage: STARTING -> STARTING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
  }

  // paused on 0.5s stage: STARTING -> PAUSED
  {
    fml::TimePoint paused_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(0.5f));
    test_model->SetRunState(KeyframeModel::PAUSED, paused_time);
    EXPECT_EQ(KeyframeModel::PAUSED, test_model->GetRunState());
    EXPECT_EQ(paused_time, test_model->pause_time());
  }

  // resume on 1.5s stage, paused time = 1.0s: PAUSED -> STARTING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
    EXPECT_EQ(fml::TimeDelta::FromSecondsF(1.0f),
              test_model->total_paused_duration());
  }

  // 2.1s stage: STARTING -> RUNNING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.1f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_EQ(true, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
  }

  // 2.5s stage: RUNNING -> RUNNING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
  }

  // paused on 2.5s stage: RUNNING -> PAUSED
  {
    fml::TimePoint paused_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.5f));
    test_model->SetRunState(KeyframeModel::PAUSED, paused_time);
    EXPECT_EQ(KeyframeModel::PAUSED, test_model->GetRunState());
    EXPECT_EQ(paused_time, test_model->pause_time());
  }

  // resume on 3.5s stage, paused time = 1.0s, total paused time = 2.0s: PAUSED
  // -> RUNNING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
    EXPECT_EQ(fml::TimeDelta::FromSecondsF(2.0f),
              test_model->total_paused_duration());
  }

  // 4.1s stage: RUNNING -> FINISHED
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.1f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(true, should_send_end_event);
  }

  // 4.5s stage: FINISHED -> FINISHED
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
  }

  // paused on 4.5s stage: FINISHED -> PAUSED
  {
    fml::TimePoint paused_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.5f));
    test_model->SetRunState(KeyframeModel::PAUSED, paused_time);
    EXPECT_EQ(KeyframeModel::PAUSED, test_model->GetRunState());
    EXPECT_EQ(paused_time, test_model->pause_time());
  }

  // resume on 5.5s stage, paused time = 1.0s, total paused time = 3.0s: PAUSED
  // -> FINISHED
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
    EXPECT_EQ(fml::TimeDelta::FromSecondsF(3.0f),
              test_model->total_paused_duration());
  }

  // update delay to 5.0s, duration to 0.0s
  starlight::AnimationData default_data1 = starlight::AnimationData();
  default_data1.duration = 0;
  default_data1.delay = 5000;
  default_data1.iteration_count = 1;
  test_model->UpdateAnimationData(&default_data1);

  // 6.0s stage: FINISHED -> STARTING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(6.0f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
  }

  // 8.1s stage: STARTING -> FINISHED
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(8.1f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::FINISHED, test_model->GetRunState());
    EXPECT_EQ(true, should_send_start_event);
    EXPECT_EQ(true, should_send_end_event);
  }

  // update delay to 0.0s, duration to 6.0s
  starlight::AnimationData default_data2 = starlight::AnimationData();
  default_data2.duration = 6000;
  default_data2.delay = 0;
  default_data2.iteration_count = 1;
  test_model->UpdateAnimationData(&default_data2);

  // 8.5s stage: FINISHED -> RUNNING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(8.5f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::RUNNING, test_model->GetRunState());
    EXPECT_EQ(true, should_send_start_event);
    EXPECT_EQ(false, should_send_end_event);
  }

  // update delay to 7.0s, duration to 1.0s
  starlight::AnimationData default_data3 = starlight::AnimationData();
  default_data3.duration = 1000;
  default_data3.delay = 7000;
  default_data3.iteration_count = 1;
  test_model->UpdateAnimationData(&default_data3);

  // 9.0s stage: RUNNING -> STARTING
  {
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(9.0f));
    bool should_send_start_event = false;
    bool should_send_end_event = false;
    std::tie(should_send_start_event, should_send_end_event) =
        test_model->UpdateState(tick_time);
    EXPECT_EQ(KeyframeModel::STARTING, test_model->GetRunState());
    EXPECT_EQ(false, should_send_start_event);
    EXPECT_EQ(true, should_send_end_event);
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace animation
}  // namespace lynx
