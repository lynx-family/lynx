// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/gfx/image/animated_image_player.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

// No playback cursor or control methods: platforms only expose frame data.
class FakePlatformImageAnimation final : public PlatformImageAnimation {
 public:
  explicit FakePlatformImageAnimation(std::vector<int64_t> durations)
      : durations_(std::move(durations)) {}

  int GetFrameCount() const override { return durations_.size(); }
  int64_t GetFrameDuration(int frame_index) const override {
    return durations_[frame_index];
  }
  std::shared_ptr<skity::Pixmap> DecodeFrame(
      int frame_index, const ImageInfo& render_info) override {
    ++decode_count;
    return nullptr;
  }

  int decode_count = 0;

 private:
  std::vector<int64_t> durations_;
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

class AnimatedImagePlayerTest : public ::testing::Test {
 protected:
  void SetUp() override { CreatePlayer({100, 200, 50}); }

  void CreatePlayer(std::vector<int64_t> durations) {
    player_ = std::make_unique<AnimatedImagePlayer>(
        std::make_unique<FakePlatformImageAnimation>(std::move(durations)),
        handler_.GetWeakPtr(),
        [this] {
          events_.push_back("frame");
          if (on_frame_) {
            on_frame_();
          }
        },
        [this] { return visible_; }, &listener_);
  }

  void Tick(int64_t time) { handler_.DoAnimationFrame(time); }

  AnimationHandler handler_;
  std::vector<std::string> events_;
  RecordingAnimationListener listener_{events_};
  bool visible_ = true;
  std::function<void()> on_frame_;
  std::unique_ptr<AnimatedImagePlayer> player_;
};

TEST_F(AnimatedImagePlayerTest, UsesExternalTimeAndUnequalFrameDurations) {
  player_->StartAnimation();
  Tick(1000);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  Tick(1099);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  Tick(1100);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  Tick(1299);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  Tick(1300);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 2);
  Tick(1350);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "frame",
                                               "frame", "frame", "loop"}));
  // Advancing time never performs decoding on the animation thread.
  auto animation = std::static_pointer_cast<FakePlatformImageAnimation>(
      player_->GetAnimation());
  EXPECT_EQ(animation->decode_count, 0);
}

TEST_F(AnimatedImagePlayerTest, ZeroTimestampIsAValidPlaybackOrigin) {
  player_->StartAnimation();
  Tick(0);
  Tick(100);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
}

TEST_F(AnimatedImagePlayerTest, DuplicateTicksAndSameFrameDoNotInvalidate) {
  player_->StartAnimation();
  Tick(1000);
  Tick(1100);
  const auto events = events_;
  Tick(1100);
  Tick(1150);
  EXPECT_EQ(events_, events);
}

TEST_F(AnimatedImagePlayerTest, TimeJumpSelectsTargetAndCoalescesMissedLoops) {
  player_->StartAnimation();
  Tick(1000);
  Tick(2500);  // Four loops plus 100 ms, directly selects frame 1.
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  EXPECT_EQ(events_,
            (std::vector<std::string>{"start", "frame", "frame", "loop"}));
  EXPECT_TRUE(player_->IsPlaying());
}

TEST_F(AnimatedImagePlayerTest, LastFrameRemainsForItsEntireDuration) {
  player_->SetLoopCount(1);
  player_->StartAnimation();
  Tick(1000);
  Tick(1300);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 2);
  EXPECT_TRUE(player_->IsPlaying());
  Tick(1349);
  EXPECT_TRUE(player_->IsPlaying());
  Tick(1350);
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 2);
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "frame",
                                               "loop", "final"}));
  EXPECT_EQ(handler_.GetAnimationCount(), 0);
  player_->DoAnimationFrame(1700);
  EXPECT_EQ(events_.back(), "final");
  EXPECT_EQ(events_.size(), 5u);
}

TEST_F(AnimatedImagePlayerTest, FiniteLoopJumpClampsToFinalFrame) {
  player_->SetLoopCount(2);
  player_->StartAnimation();
  Tick(0);
  Tick(100000);
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 2);
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame", "frame",
                                               "loop", "final"}));
}

TEST_F(AnimatedImagePlayerTest, PauseResumePreservesPositionWithinFrame) {
  player_->StartAnimation();
  Tick(1000);
  Tick(1150);
  player_->PauseAnimation();
  EXPECT_EQ(handler_.GetAnimationCount(), 0);
  Tick(5000);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  player_->ResumeAnimation();
  Tick(6000);  // Resumes at 150 ms, not at the start of frame 1.
  Tick(6149);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  Tick(6150);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 2);
  EXPECT_TRUE(player_->IsPlaying());
}

TEST_F(AnimatedImagePlayerTest, RestartResetsTimelineAndCompletion) {
  player_->SetLoopCount(1);
  player_->StartAnimation();
  Tick(1000);
  Tick(1350);
  player_->StartAnimation();
  EXPECT_TRUE(player_->IsPlaying());
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  Tick(5000);
  Tick(5100);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  Tick(5350);
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_EQ(events_.back(), "final");
}

TEST_F(AnimatedImagePlayerTest, RollbackResamplesWithoutReplayingLoopEvents) {
  player_->StartAnimation();
  Tick(1000);
  Tick(1500);  // Loop 1, frame 1.
  Tick(1050);  // Seek backwards to frame 0.
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  Tick(1500);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  EXPECT_EQ(std::count(events_.begin(), events_.end(), "loop"), 1);
}

TEST_F(AnimatedImagePlayerTest, SeekBeforeOriginClampsToFirstFrame) {
  player_->StartAnimation();
  Tick(1000);
  Tick(1300);
  Tick(500);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
}

TEST_F(AnimatedImagePlayerTest, VisibilitySuspendsTicksThenCatchesUp) {
  int wakeups = 0;
  handler_.SetAnimationCallback([&](int64_t) { ++wakeups; });
  player_->StartAnimation();
  Tick(1000);
  visible_ = false;
  EXPECT_FALSE(handler_.DoAnimationFrame(1100));
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  EXPECT_EQ(handler_.GetAnimationCount(), 1);
  visible_ = true;
  player_->EnsureAnimationScheduled();
  EXPECT_EQ(wakeups, 2);
  Tick(1500);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  EXPECT_EQ(std::count(events_.begin(), events_.end(), "start"), 1);
}

TEST_F(AnimatedImagePlayerTest, AutoplayAndResumeAreIdempotent) {
  player_->SetAutoPlay(false);
  EXPECT_TRUE(events_.empty());
  player_->SetAutoPlay(true);
  player_->SetAutoPlay(true);
  player_->ResumeAnimation();
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame"}));
  player_->PauseAnimation();
  player_->ResumeAnimation();
  EXPECT_EQ(std::count(events_.begin(), events_.end(), "start"), 2);
}

TEST_F(AnimatedImagePlayerTest, StopCancelsPendingCompletion) {
  player_->SetLoopCount(1);
  player_->StartAnimation();
  Tick(1000);
  player_->StopAnimation();
  Tick(2000);
  player_->EnsureAnimationScheduled();
  EXPECT_EQ(handler_.GetAnimationCount(), 0);
  EXPECT_EQ(events_, (std::vector<std::string>{"start", "frame"}));
}

TEST_F(AnimatedImagePlayerTest, PlayersHaveIndependentTimelinesAndControls) {
  AnimatedImagePlayer other(
      std::make_unique<FakePlatformImageAnimation>(
          std::vector<int64_t>{100, 200, 50}),
      handler_.GetWeakPtr(), [] {}, [] { return true; });
  other.SetLoopCount(1);
  other.StartAnimation();
  Tick(1000);
  player_->StartAnimation();
  Tick(1200);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  EXPECT_EQ(other.GetCurrentFrameIndex(), 1);
  player_->PauseAnimation();
  Tick(1350);
  EXPECT_FALSE(other.IsPlaying());
  EXPECT_EQ(other.GetCurrentFrameIndex(), 2);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
}

TEST_F(AnimatedImagePlayerTest, EmptyAnimationIsInvalid) {
  CreatePlayer({});
  EXPECT_FALSE(player_->IsValid());
  player_->StartAnimation();
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_TRUE(events_.empty());
}

TEST_F(AnimatedImagePlayerTest, SingleFrameCompletesAfterItsDuration) {
  CreatePlayer({100});
  player_->SetLoopCount(1);
  player_->StartAnimation();
  Tick(0);
  Tick(99);
  EXPECT_TRUE(player_->IsPlaying());
  Tick(100);
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_EQ(events_,
            (std::vector<std::string>{"start", "frame", "loop", "final"}));
}

TEST_F(AnimatedImagePlayerTest, InvalidDurationsAreNormalizedInPlayer) {
  CreatePlayer({0, -1, 100});
  player_->StartAnimation();
  Tick(0);
  Tick(1);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
  Tick(2);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 2);
}

TEST_F(AnimatedImagePlayerTest, DurationOverflowIsInvalid) {
  CreatePlayer({std::numeric_limits<int64_t>::max(), 1});
  EXPECT_FALSE(player_->IsValid());
}

TEST_F(AnimatedImagePlayerTest, HugeTimestampDoesNotOverflowPosition) {
  player_->StartAnimation();
  Tick(0);
  Tick(100);
  player_->PauseAnimation();
  player_->ResumeAnimation();
  Tick(0);
  Tick(std::numeric_limits<int64_t>::max());
  EXPECT_TRUE(player_->IsPlaying());
  EXPECT_GE(player_->GetCurrentFrameIndex(), 0);
  EXPECT_LT(player_->GetCurrentFrameIndex(), 3);
}

TEST_F(AnimatedImagePlayerTest, ListenerCanDestroyPlayerOnStart) {
  listener_.on_start = [this] { player_.reset(); };
  player_->StartAnimation();
  EXPECT_FALSE(player_);
  EXPECT_EQ(handler_.GetAnimationCount(), 0);
}

TEST_F(AnimatedImagePlayerTest, ListenerCanDestroyPlayerOnLoopCompletion) {
  player_->SetLoopCount(1);
  listener_.on_loop = [this] { player_.reset(); };
  player_->StartAnimation();
  Tick(0);
  Tick(350);
  EXPECT_FALSE(player_);
  EXPECT_EQ(events_.back(), "loop");
}

TEST_F(AnimatedImagePlayerTest, FrameCallbackCanDestroyPlayer) {
  player_->StartAnimation();
  Tick(0);
  on_frame_ = [this] { player_.reset(); };
  Tick(100);
  EXPECT_FALSE(player_);
  EXPECT_EQ(handler_.GetAnimationCount(), 0);
}

TEST_F(AnimatedImagePlayerTest, LoopCallbackCanRestartWithoutStaleFinalEvent) {
  player_->SetLoopCount(1);
  player_->StartAnimation();
  Tick(0);
  listener_.on_loop = [this] { player_->StartAnimation(); };
  Tick(350);
  EXPECT_TRUE(player_->IsPlaying());
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 0);
  EXPECT_EQ(std::count(events_.begin(), events_.end(), "final"), 0);
  Tick(1000);
  Tick(1100);
  EXPECT_EQ(player_->GetCurrentFrameIndex(), 1);
}

TEST_F(AnimatedImagePlayerTest, FrameCallbackCanStopBeforeCompletionEvents) {
  player_->SetLoopCount(1);
  player_->StartAnimation();
  Tick(0);
  on_frame_ = [this] { player_->StopAnimation(); };
  Tick(350);
  EXPECT_FALSE(player_->IsPlaying());
  EXPECT_EQ(events_.back(), "frame");
}

TEST_F(AnimatedImagePlayerTest, PlayerCanOutliveItsFrameSource) {
  auto handler = std::make_unique<AnimationHandler>();
  AnimatedImagePlayer player(
      std::make_unique<FakePlatformImageAnimation>(
          std::vector<int64_t>{100, 50}),
      handler->GetWeakPtr(), [] {}, [] { return true; });
  player.StartAnimation();
  handler.reset();
  player.StopAnimation();
  player.StartAnimation();
  player.DoAnimationFrame(1000);
  player.DoAnimationFrame(1100);
  EXPECT_EQ(player.GetCurrentFrameIndex(), 1);
}

}  // namespace
}  // namespace clay
