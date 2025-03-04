// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_TESTING_FIBER_ELEMENT_TEST_H_
#define CORE_RENDERER_DOM_TESTING_FIBER_ELEMENT_TEST_H_

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "core/renderer/dom/testing/fiber_mock_painting_context.h"
#include "core/shell/common/vsync_monitor.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
static constexpr int64_t kFrameDuration = 16;  // ms

static constexpr double COMPARE_EPSILON = 0.00001;

const std::tuple<bool, int> fiber_element_generation_params[] = {
    std::make_tuple(
        false, 0),  // disable parallel flush with ALL_ON_UI thread strategy
    std::make_tuple(
        false, 3),  // disable parellel flush with MULTI_THREADS thread strategy
    std::make_tuple(true,
                    0),  // enable parallel flush with ALL_ON_UI thread strategy
    std::make_tuple(
        true, 3),  // enable parallel flush with MULTI_THREADS thread strategy
};

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

class FiberElementMockTasmDelegate : public test::MockTasmDelegate {
 public:
  void UpdateLayoutNodeByBundle(
      int32_t id, std::unique_ptr<tasm::LayoutBundle> bundle) override {
    std::unique_lock<std::mutex> locker(mutex_);
    captured_ids_.emplace_back(id);
    captured_bundles_.emplace_back(std::move(bundle));
  }

  std::mutex mutex_;
  std::vector<int> captured_ids_;
  std::vector<std::unique_ptr<LayoutBundle>> captured_bundles_;
};

class FiberElementTest
    : public ::testing::TestWithParam<std::tuple<bool, int>> {
 public:
  FiberElementTest() { current_parameter_ = GetParam(); }
  ~FiberElementTest() override {}
  lynx::tasm::ElementManager* manager;
  ::testing::NiceMock<FiberElementMockTasmDelegate> tasm_mediator;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm;
  FiberMockPaintingContext* platform_impl_;
  std::shared_ptr<TestVSyncMonitor> vsync_monitor_;

  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override;

  bool HasCapturePlatformNodeTag(int32_t target_id, std::string expected_tag);

  bool HasCaptureSignWithLayoutAttribute(
      int32_t target_id, starlight::LayoutAttribute target_key,
      const lepus::Value& target_value = lepus::Value(), int32_t count = 1);

  bool HasCaptureSignWithStyleKeyAndValuePattern(
      int32_t target_id, CSSPropertyID target_key,
      const tasm::CSSValue& target_value, int32_t count = 1);

  bool HasCaptureSignWithStyleKeyAndValue(int32_t target_id,
                                          CSSPropertyID target_key,
                                          const tasm::CSSValue& target_value,
                                          int32_t count = 1);

  bool HasCaptureSignWithResetStyle(int32_t target_id, CSSPropertyID target_key,
                                    int32_t count = 1);

  bool HasCaptureSignWithTag(int32_t target_id, const std::string& target_tag,
                             int32_t count = 1);

  bool HasCaptureSignWithInlineParentContainer(int32_t target_id,
                                               bool is_parent_inline_container,
                                               int32_t count = 1);

  bool HasCaptureSignWithFontSize(int32_t target_id, double cur_node_font_size,
                                  double root_node_font_size, double font_scale,
                                  int32_t count = 1);

 protected:
  std::tuple<bool, int> current_parameter_;
  int32_t thread_strategy;
  bool enable_parallel_element_flush;
};

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_TESTING_FIBER_ELEMENT_TEST_H_
