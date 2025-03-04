// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/element_vsync_proxy.h"

#include <memory>

#include "core/animation/animation.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/common/vsync_monitor.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

constexpr int64_t kFrameDuration = 16;  // ms

}  // namespace

class TestVSyncMonitor : public shell::VSyncMonitor {
 public:
  TestVSyncMonitor() = default;
  ~TestVSyncMonitor() override = default;

  void RequestVSync() override {}

  void TriggerVsync() {
    OnVSync(current_, current_ + kFrameDuration);
    current_ += kFrameDuration;
  }

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
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  std::shared_ptr<TestVSyncMonitor> vsync_monitor_;

  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
    vsync_monitor_ = std::make_shared<TestVSyncMonitor>();
    vsync_monitor_->BindToCurrentThread();
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
  vsync_monitor_->TriggerVsync();
  EXPECT_TRUE(!test_vsync_proxy->HasRequestedNextFrame());
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

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
