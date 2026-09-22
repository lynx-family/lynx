// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/element_vsync_proxy.h"

#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "base/include/fml/message_loop.h"
#include "core/animation/animation.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

constexpr int64_t kFrameDuration = 16000000;  // ns

class CallbackTasmDelegate : public test::MockTasmDelegate {
 public:
  void SendAnimationEvent(const std::string& type, int tag,
                          const lepus::Value& dict) override {
    MockTasmDelegate::SendAnimationEvent(type, tag, dict);
    if (on_animation_event) {
      auto callback = on_animation_event;
      callback(type);
    }
  }

  std::function<void(const std::string&)> on_animation_event;
};

class CountingAnimationManager : public animation::CSSKeyframeManager {
 public:
  CountingAnimationManager(Element* element,
                           std::function<void(fml::TimePoint&)> tick)
      : CSSKeyframeManager(element), tick_(std::move(tick)) {}

  void TickAllAnimation(fml::TimePoint& time) override { tick_(time); }

  void AddAnimation(const std::shared_ptr<animation::Animation>& animation) {
    animations_map_.insert_or_assign(animation->name(), animation);
  }

 private:
  std::function<void(fml::TimePoint&)> tick_;
};

class CountingTickElement : public ViewElement {
 public:
  explicit CountingTickElement(ElementManager* manager,
                               bool with_animation = true)
      : ViewElement(manager) {
    MarkAttached();
    if (!with_animation) {
      return;
    }
    css_keyframe_manager_ = std::make_unique<CountingAnimationManager>(
        this, [this](fml::TimePoint& time) { ++tick_count; });
  }

  void TickElement(fml::TimePoint& time) override {
    ++element_tick_count;
    last_element_tick_time = time;
  }

  void AddAnimation(const std::shared_ptr<animation::Animation>& animation) {
    static_cast<CountingAnimationManager*>(css_keyframe_manager_.get())
        ->AddAnimation(animation);
  }

  int tick_count{0};
  int element_tick_count{0};
  fml::TimePoint last_element_tick_time{fml::TimePoint::Min()};
};

}  // namespace

class TestVSyncMonitor : public base::VSyncMonitor {
 public:
  TestVSyncMonitor() = default;
  ~TestVSyncMonitor() override = default;

  void RequestVSync() override { ++request_count; }

  void TriggerVsync() {
    OnVSync(current_, current_ + kFrameDuration);
    current_ += kFrameDuration;
  }

  void TriggerVsyncAt(int64_t frame_start) {
    OnVSync(frame_start, frame_start + kFrameDuration);
  }

  int request_count{0};

 private:
  int64_t current_ = kFrameDuration;
};

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class ElementVsyncProxyTest : public ::testing::Test {
 public:
  ElementVsyncProxyTest() {}
  ~ElementVsyncProxyTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<CallbackTasmDelegate>> tasm_mediator;
  std::shared_ptr<TestVSyncMonitor> vsync_monitor_;

  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator =
        std::make_shared<::testing::NiceMock<CallbackTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
    vsync_monitor_ = std::make_shared<TestVSyncMonitor>();
    vsync_monitor_->BindToCurrentThread();
    manager->vsync_monitor() = vsync_monitor_;
  }

  std::shared_ptr<animation::Animation> AddBackgroundAnimation(
      CountingTickElement* element, const char* name, long duration,
      int iterations = -1, long delay = 0) {
    auto animation = std::make_shared<animation::Animation>(name);
    animation->BindElement(element);
    auto effect = animation::KeyframeEffect::Create();
    effect->SetHasCustomPropertyKeyframes(true);
    animation->SetKeyframeEffect(std::move(effect));
    starlight::AnimationData data;
    data.duration = duration;
    data.iteration_count = iterations;
    data.delay = delay;
    animation->UpdateAnimationData(data);
    animation->Play(false);
    element->AddAnimation(animation);
    return animation;
  }

  void RunTimer(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  }

  std::shared_ptr<ElementVsyncProxy> InitTestVSyncProxy() {
    return std::make_shared<ElementVsyncProxy>(manager.get(), vsync_monitor_);
  }
};

TEST_F(ElementVsyncProxyTest, RequestNextFrame) {
  auto test_vsync_proxy = InitTestVSyncProxy();
  EXPECT_TRUE(!test_vsync_proxy->HasRequestedNextFrame());
  test_vsync_proxy->RequestNextFrame();
  EXPECT_TRUE(test_vsync_proxy->HasRequestedNextFrame());
  const auto before = fml::TimePoint::Now();
  // The test monitor deliberately uses a different clock epoch.
  vsync_monitor_->TriggerVsync();
  EXPECT_TRUE(!test_vsync_proxy->HasRequestedNextFrame());
  EXPECT_GE(test_vsync_proxy->last_tick_time(), before);
  EXPECT_LE(test_vsync_proxy->last_tick_time(), fml::TimePoint::Now());
}

TEST_F(ElementVsyncProxyTest, SetPreferredFps) {
  auto test_vsync_proxy = InitTestVSyncProxy();
  test_vsync_proxy->set_preferred_fps("high");
  EXPECT_EQ(test_vsync_proxy->preferred_fps(), "high");
}

TEST_F(ElementVsyncProxyTest, TickAllElement) {
  auto test_vsync_proxy = InitTestVSyncProxy();
  test_vsync_proxy->set_preferred_fps("low");
  auto time1 = fml::TimePoint::Now();
  test_vsync_proxy->TickAllElement(time1);
  test_vsync_proxy->set_preferred_fps("auto");
  auto time2 = fml::TimePoint::Now();
  test_vsync_proxy->TickAllElement(time2);
}

TEST_F(ElementVsyncProxyTest, StoppedPageIgnoresQueuedFrame) {
  auto element = fml::AdoptRef(new CountingTickElement(manager.get()));
  manager->RequestNextFrame(element.get());
  vsync_monitor_->TriggerVsync();
  ASSERT_EQ(element->tick_count, 1);

  manager->RequestNextFrame(element.get());
  manager->StopAnimationVsync();
  // The page and its proxy are still alive when the queued callback runs.
  vsync_monitor_->TriggerVsync();
  EXPECT_EQ(element->tick_count, 1);

  manager->RequestNextFrame(element.get());
  vsync_monitor_->TriggerVsync();
  EXPECT_EQ(element->tick_count, 1);
}

TEST_F(ElementVsyncProxyTest, StoppedPageCannotCreateAnimationVsync) {
  auto element = fml::AdoptRef(new CountingTickElement(manager.get()));
  manager->StopAnimationVsync();
  manager->RequestNextFrame(element.get());
  vsync_monitor_->TriggerVsync();
  EXPECT_EQ(element->tick_count, 0);
}

TEST_F(ElementVsyncProxyTest, InvalidatedProxyCannotRequestNextFrame) {
  auto proxy = InitTestVSyncProxy();
  proxy->RequestNextFrame();
  proxy->Invalidate();
  manager.reset();

  // A retained proxy must not dereference its former manager.
  vsync_monitor_->TriggerVsync();
  proxy->RequestNextFrame();
  EXPECT_FALSE(proxy->HasRequestedNextFrame());
}

TEST_F(ElementVsyncProxyTest, BackgroundDefersElementFramesUntilForeground) {
  for (bool with_animation : {false, true}) {
    SCOPED_TRACE(with_animation);
    const auto requests = vsync_monitor_->request_count;
    auto element =
        fml::AdoptRef(new CountingTickElement(manager.get(), with_animation));
    manager->RequestNextFrame(element.get());
    // Rapid foreground/background switches reuse the outstanding native frame.
    manager->SetElementVsyncPaused(true);
    manager->SetElementVsyncPaused(false);
    manager->SetElementVsyncPaused(true);
    vsync_monitor_->TriggerVsync();
    for (int i = 0; i < 100; ++i) {
      manager->RequestNextFrame(element.get());
    }
    EXPECT_EQ(vsync_monitor_->request_count, requests + 1);
    EXPECT_EQ(element->tick_count, 0);
    EXPECT_EQ(element->element_tick_count, 0);

    manager->SetElementVsyncPaused(false);
    EXPECT_EQ(vsync_monitor_->request_count, requests + 2);
    vsync_monitor_->TriggerVsync();
    EXPECT_EQ(element->element_tick_count, 1);
    EXPECT_EQ(element->tick_count, with_animation ? 1 : 0);
  }
}

TEST_F(ElementVsyncProxyTest, BackgroundTimerOnlyDispatchesAnimationEvents) {
  auto element = fml::AdoptRef(new CountingTickElement(manager.get()));
  AddBackgroundAnimation(element.get(), "background", 100);
  element->data_model()->SetStaticEvent("bindEvent", "animationstart", "start");
  element->data_model()->SetStaticEvent("bindEvent", "animationiteration",
                                        "iteration");

  manager->SetElementVsyncPaused(true);
  manager->RequestNextFrame(element.get());
  RunTimer(20);
  EXPECT_STREQ(tasm_mediator->GetAnimationEventType(), "animationstart");
  tasm_mediator->ClearAnimationEvent();
  const auto before_iteration_event = fml::TimePoint::Now();
  RunTimer(120);
  EXPECT_STREQ(tasm_mediator->GetAnimationEventType(), "animationiteration");
  EXPECT_EQ(element->tick_count, 0);
  EXPECT_EQ(element->element_tick_count, 0);
  EXPECT_EQ(vsync_monitor_->request_count, 0);

  // A timer from the previous background period must not fire after resume.
  manager->SetElementVsyncPaused(false);
  tasm_mediator->ClearAnimationEvent();
  RunTimer(120);
  EXPECT_EQ(tasm_mediator->GetAnimationEventType(), nullptr);
  EXPECT_EQ(vsync_monitor_->request_count, 1);
  const auto stale_time =
      fml::TimePoint::Now() - fml::TimeDelta::FromMilliseconds(500);
  vsync_monitor_->TriggerVsyncAt(stale_time.ToEpochDelta().ToNanoseconds());
  EXPECT_EQ(element->element_tick_count, 1);
  EXPECT_GE(element->last_element_tick_time, before_iteration_event);
}

TEST_F(ElementVsyncProxyTest,
       EventHandlerCanScheduleAnotherBackgroundAnimation) {
  auto element = fml::AdoptRef(new CountingTickElement(manager.get()));
  AddBackgroundAnimation(element.get(), "first", 30, 1);
  element->data_model()->SetStaticEvent("bindEvent", "animationend", "end");
  std::shared_ptr<animation::Animation> next_animation;
  tasm_mediator->on_animation_event = [&](const std::string& type) {
    if (type == "animationend") {
      next_animation = AddBackgroundAnimation(element.get(), "next", 100);
      manager->RequestNextFrame(element.get());
    }
  };
  manager->SetElementVsyncPaused(true);
  manager->RequestNextFrame(element.get());
  RunTimer(20);
  RunTimer(50);
  ASSERT_NE(next_animation, nullptr);
  RunTimer(20);
  EXPECT_NE(next_animation->start_time(), fml::TimePoint::Min());
  EXPECT_EQ(vsync_monitor_->request_count, 0);
  EXPECT_EQ(element->tick_count, 0);
}

TEST_F(ElementVsyncProxyTest, EventHandlerCanResumeOrStopPage) {
  for (bool stop : {false, true}) {
    SCOPED_TRACE(stop);
    const auto previous_requests = vsync_monitor_->request_count;
    auto element = fml::AdoptRef(new CountingTickElement(manager.get()));
    AddBackgroundAnimation(element.get(), "background", 100);
    element->data_model()->SetStaticEvent("bindEvent", "animationstart",
                                          "start");
    element->data_model()->SetStaticEvent("bindEvent", "animationiteration",
                                          "iteration");
    tasm_mediator->on_animation_event = [&, stop](const std::string&) {
      if (stop) {
        manager->StopAnimationVsync();
      } else {
        manager->SetElementVsyncPaused(false);
      }
    };
    manager->SetElementVsyncPaused(true);
    manager->RequestNextFrame(element.get());
    RunTimer(20);
    EXPECT_EQ(vsync_monitor_->request_count,
              previous_requests + (stop ? 0 : 1));
    tasm_mediator->ClearAnimationEvent();
    RunTimer(120);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
