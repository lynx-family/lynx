// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "base/include/fml/message_loop.h"
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
  bool DrawFrame() override { return state_->playing; }

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

}  // namespace
}  // namespace clay
