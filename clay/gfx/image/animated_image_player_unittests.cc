// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/message_loop_impl.h"
#include "clay/gfx/image/animated_image_player.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

struct AnimationState {
  bool playing = false;
  bool paused = false;
  int loop_count = 0;
  int start_count = 0;
  int stop_count = 0;
  std::deque<PlatformImageAnimation::FrameResult> frames;
};

class FakePlatformImageAnimation final : public PlatformImageAnimation {
 public:
  explicit FakePlatformImageAnimation(std::shared_ptr<AnimationState> state)
      : state_(std::move(state)) {}

  int64_t GetDuration() override { return 100; }
  std::shared_ptr<skity::Pixmap> ToBitmap(
      const ImageInfo& render_info) override {
    return nullptr;
  }
  FrameResult DrawFrame() override {
    if (!state_->playing) {
      return FrameResult::kNoFrame;
    }
    if (state_->frames.empty()) {
      return FrameResult::kFrameReady;
    }
    auto result = state_->frames.front();
    state_->frames.pop_front();
    if (result == FrameResult::kFinalLoopComplete) {
      state_->playing = false;
    }
    return result;
  }

  void SetLoopCount(int loop_count) override {
    state_->loop_count = loop_count;
  }

  void StartAnimation() override {
    state_->start_count++;
    state_->playing = true;
    state_->paused = false;
  }

  void StopAnimation() override {
    state_->stop_count++;
    state_->playing = false;
    state_->paused = false;
  }

  void PauseAnimation() override {
    state_->playing = false;
    state_->paused = true;
  }

  void ResumeAnimation() override {
    state_->playing = true;
    state_->paused = false;
  }

 private:
  std::shared_ptr<AnimationState> state_;
};

class FakePlatformImage final : public PlatformImage {
 public:
  int GetWidth() override { return 100; }
  int GetHeight() override { return 100; }
  skity::ColorType GetColorType() override { return skity::ColorType::kRGBA; }
  skity::AlphaType GetAlphaType() override {
    return skity::AlphaType::kPremul_AlphaType;
  }
  std::shared_ptr<skity::Pixmap> ToBitmap(
      const ImageInfo& render_info) override {
    return nullptr;
  }
  bool IsAnimated() override { return true; }

  std::unique_ptr<PlatformImageAnimation> CreateAnimation() override {
    auto state = std::make_shared<AnimationState>();
    animation_states.push_back(state);
    return std::make_unique<FakePlatformImageAnimation>(std::move(state));
  }

  std::vector<std::shared_ptr<AnimationState>> animation_states;
};

TEST(AnimatedImagePlayerTest, SharedResourceCreatesIndependentPlayers) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto platform_image = std::make_shared<FakePlatformImage>();
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
  AnimatedImagePlayer first(
      platform_image->CreateAnimation(), task_runner, [] {},
      [] { return false; });
  AnimatedImagePlayer second(
      platform_image->CreateAnimation(), task_runner, [] {},
      [] { return false; });

  ASSERT_EQ(platform_image->animation_states.size(), 2u);
  first.SetAutoPlay(true);
  second.SetAutoPlay(true);
  EXPECT_TRUE(platform_image->animation_states[0]->playing);
  EXPECT_TRUE(platform_image->animation_states[1]->playing);

  first.PauseAnimation();
  EXPECT_TRUE(platform_image->animation_states[0]->paused);
  EXPECT_FALSE(platform_image->animation_states[0]->playing);
  EXPECT_TRUE(platform_image->animation_states[1]->playing);

  second.SetLoopCount(3);
  EXPECT_EQ(platform_image->animation_states[0]->loop_count, 0);
  EXPECT_EQ(platform_image->animation_states[1]->loop_count, 3);

  first.ResumeAnimation();
  second.StopAnimation();
  EXPECT_TRUE(platform_image->animation_states[0]->playing);
  EXPECT_FALSE(platform_image->animation_states[1]->playing);
}

TEST(AnimatedImagePlayerTest, AutoplayStateIsIndependent) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto platform_image = std::make_shared<FakePlatformImage>();
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
  AnimatedImagePlayer first(
      platform_image->CreateAnimation(), task_runner, [] {},
      [] { return false; });
  AnimatedImagePlayer second(
      platform_image->CreateAnimation(), task_runner, [] {},
      [] { return false; });

  first.SetAutoPlay(true);
  second.SetAutoPlay(false);

  EXPECT_TRUE(platform_image->animation_states[0]->playing);
  EXPECT_FALSE(platform_image->animation_states[1]->playing);
}

TEST(AnimatedImagePlayerTest, SetAutoPlayIsIdempotent) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto platform_image = std::make_shared<FakePlatformImage>();
  AnimatedImagePlayer player(
      platform_image->CreateAnimation(),
      fml::MessageLoop::GetCurrent().GetTaskRunner(), [] {},
      [] { return false; });
  auto state = platform_image->animation_states[0];

  player.SetAutoPlay(false);
  EXPECT_EQ(state->start_count, 0);
  EXPECT_EQ(state->stop_count, 0);

  player.SetAutoPlay(true);
  EXPECT_TRUE(state->playing);
  EXPECT_EQ(state->start_count, 1);
  EXPECT_EQ(state->stop_count, 1);

  player.SetAutoPlay(true);
  EXPECT_EQ(state->start_count, 1);
  EXPECT_EQ(state->stop_count, 1);

  player.SetAutoPlay(false);
  EXPECT_FALSE(state->playing);
  EXPECT_EQ(state->start_count, 1);
  EXPECT_EQ(state->stop_count, 2);

  player.SetAutoPlay(false);
  EXPECT_EQ(state->start_count, 1);
  EXPECT_EQ(state->stop_count, 2);
}

class AnimationTaskRunner : public fml::TaskRunner {
 public:
  AnimationTaskRunner() : TaskRunner(nullptr) {}

  void PostDelayedTask(lynx::base::closure task,
                       fml::TimeDelta delay) override {
    tasks_.push_back(std::move(task));
  }

  bool RunsTasksOnCurrentThread() override { return true; }

  void RunNext() {
    ASSERT_FALSE(tasks_.empty());
    auto task = std::move(tasks_.front());
    tasks_.pop_front();
    task();
  }

  bool HasTasks() const { return !tasks_.empty(); }

 private:
  std::deque<lynx::base::closure> tasks_;
};

class RecordingAnimationListener : public ImageAnimationListener {
 public:
  explicit RecordingAnimationListener(std::vector<std::string>& events)
      : events_(events) {}

  void OnStartPlay() override {
    events_.push_back("start");
    if (on_start) {
      on_start();
    }
  }
  void OnCurrentLoopComplete() override {
    events_.push_back("loop");
    if (on_loop) {
      on_loop();
    }
  }
  void OnFinalLoopComplete() override { events_.push_back("final"); }

  std::function<void()> on_start;
  std::function<void()> on_loop;

 private:
  std::vector<std::string>& events_;
};

class AnimatedImageCallbacksTest : public ::testing::Test {
 protected:
  using FrameResult = PlatformImageAnimation::FrameResult;

  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    task_runner_ = fml::MakeRefCounted<AnimationTaskRunner>();
    state_ = std::make_shared<AnimationState>();
    player_ = std::make_unique<AnimatedImagePlayer>(
        std::make_unique<FakePlatformImageAnimation>(state_), task_runner_,
        [this] {
          events_.push_back("frame");
          if (on_frame_) {
            on_frame_();
          }
        },
        [this] { return visible_; }, &listener_);
  }

  std::vector<std::string> events_;
  RecordingAnimationListener listener_{events_};
  fml::RefPtr<AnimationTaskRunner> task_runner_;
  std::shared_ptr<AnimationState> state_;
  bool visible_ = true;
  std::function<void()> on_frame_;
  std::unique_ptr<AnimatedImagePlayer> player_;
};

TEST_F(AnimatedImageCallbacksTest,
       StartAndResumeNotifyOnlyPlaybackTransitions) {
  visible_ = false;
  player_->SetAutoPlay(false);
  EXPECT_TRUE(events_.empty());
  player_->SetAutoPlay(true);
  player_->SetAutoPlay(true);
  player_->ResumeAnimation();
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame"}));

  player_->PauseAnimation();
  player_->ResumeAnimation();
  player_->StopAnimation();
  player_->StartAnimation();
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "start",
                                               "frame", "start", "frame"}));
}

TEST_F(AnimatedImageCallbacksTest,
       FinalLoopNotifiesCurrentLoopBeforeFinalLoop) {
  state_->frames = {FrameResult::kFrameReady, FrameResult::kLoopComplete,
                    FrameResult::kFrameReady, FrameResult::kFinalLoopComplete};
  player_->SetLoopCount(2);
  player_->StartAnimation();
  for (int i = 0; i < 4; ++i) {
    task_runner_->RunNext();
  }
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "frame",
                                               "frame", "loop", "frame",
                                               "frame", "loop", "final"}));
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_FALSE(task_runner_->HasTasks());
  player_->EnsureAnimationScheduled();
  EXPECT_FALSE(task_runner_->HasTasks());
}

TEST_F(AnimatedImageCallbacksTest, InfiniteLoopsDoNotNotifyFinalCompletion) {
  state_->frames = {FrameResult::kLoopComplete, FrameResult::kLoopComplete};
  player_->SetLoopCount(0);
  player_->StartAnimation();
  task_runner_->RunNext();
  task_runner_->RunNext();
  EXPECT_TRUE(player_->IsPlaying());
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "frame",
                                               "loop", "frame", "loop"}));
}

TEST_F(AnimatedImageCallbacksTest, DecodeFailureDoesNotNotifyCompletion) {
  state_->frames = {FrameResult::kNoFrame};
  player_->StartAnimation();
  task_runner_->RunNext();
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_FALSE(task_runner_->HasTasks());
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame"}));
}

TEST_F(AnimatedImageCallbacksTest, StoppingCancelsPendingCompletion) {
  state_->frames = {FrameResult::kFinalLoopComplete};
  player_->StartAnimation();
  player_->StopAnimation();
  task_runner_->RunNext();
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame"}));
  EXPECT_FALSE(player_->IsPlaying());
}

TEST_F(AnimatedImageCallbacksTest,
       VisibilityReschedulesWithoutAnotherStartEvent) {
  visible_ = false;
  state_->frames = {FrameResult::kFinalLoopComplete};
  player_->StartAnimation();
  EXPECT_FALSE(task_runner_->HasTasks());
  visible_ = true;
  player_->EnsureAnimationScheduled();
  task_runner_->RunNext();
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "frame",
                                               "frame", "loop", "final"}));
}

TEST_F(AnimatedImageCallbacksTest, ListenerCanDestroyPlayerOnStart) {
  listener_.on_start = [this] { player_.reset(); };
  player_->StartAnimation();
  EXPECT_FALSE(player_);
  EXPECT_FALSE(task_runner_->HasTasks());
  EXPECT_EQ(events_, (std::vector<std::string>{"start"}));
}

TEST_F(AnimatedImageCallbacksTest, ListenerCanDestroyPlayerOnLoopCompletion) {
  state_->frames = {FrameResult::kFinalLoopComplete};
  listener_.on_loop = [this] { player_.reset(); };
  player_->StartAnimation();
  task_runner_->RunNext();
  EXPECT_FALSE(player_);
  EXPECT_EQ(events_,
            (std::vector<std::string>{"start", "frame", "frame", "loop"}));
}

TEST_F(AnimatedImageCallbacksTest, FrameCallbackCanDestroyPlayer) {
  state_->frames = {FrameResult::kFinalLoopComplete};
  player_->StartAnimation();
  on_frame_ = [this] { player_.reset(); };
  task_runner_->RunNext();
  EXPECT_FALSE(player_);
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "frame"}));
}

TEST_F(AnimatedImageCallbacksTest, PlaybackEventsRemainIndependent) {
  auto second_state = std::make_shared<AnimationState>();
  std::vector<std::string> second_events;
  RecordingAnimationListener second_listener(second_events);
  AnimatedImagePlayer second(
      std::make_unique<FakePlatformImageAnimation>(second_state), task_runner_,
      [] {}, [] { return false; }, &second_listener);
  state_->frames = {FrameResult::kFinalLoopComplete};
  player_->StartAnimation();
  second.StartAnimation();
  task_runner_->RunNext();
  EXPECT_EQ(second_events, (std::vector<std::string>{"start"}));
  EXPECT_TRUE(second.IsPlaying());
  EXPECT_FALSE(player_->IsPlaying());
}

}  // namespace
}  // namespace clay
