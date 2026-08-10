// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "devtool/lynx_devtool/input/input_event_target.h"
#define private public
#include "devtool/lynx_devtool/input/synthetic_gesture_controller.h"
#undef private
#include "devtool/lynx_devtool/input/synthetic_tap_gesture.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace input {
namespace {

class RecordingInputEventTarget : public InputEventTarget {
 public:
  PointerCapabilities GetPointerCapabilities() const override {
    PointerCapabilities capabilities;
    capabilities.default_source_type = PointerSourceType::kTouch;
    capabilities.supports_touch = true;
    return capabilities;
  }

  bool InjectPointerEvent(const PointerEvent& event) override {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(event);
    if (fail_on_injection_index_ >= 0 &&
        static_cast<int>(events_.size()) - 1 == fail_on_injection_index_) {
      return false;
    }
    return injection_result_;
  }

  void WaitForInputProcessed(std::function<void(bool)> callback) override {
    if (!defer_processing_) {
      callback(processing_result_);
      return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    processing_callbacks_.push_back(std::move(callback));
  }

  void CompleteNextProcessing(bool success) {
    std::function<void(bool)> callback;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      ASSERT_FALSE(processing_callbacks_.empty());
      callback = std::move(processing_callbacks_.front());
      processing_callbacks_.erase(processing_callbacks_.begin());
    }
    callback(success);
  }

  std::vector<PointerEvent> Events() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
  }

  size_t EventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
  }

  bool injection_result_ = true;
  bool processing_result_ = true;
  bool defer_processing_ = false;
  int fail_on_injection_index_ = -1;

 private:
  mutable std::mutex mutex_;
  std::vector<PointerEvent> events_;
  std::vector<std::function<void(bool)>> processing_callbacks_;
};

bool WaitForEventCount(const std::shared_ptr<RecordingInputEventTarget>& target,
                       size_t expected_count) {
  constexpr int kMaxAttempts = 100;
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    if (target->EventCount() >= expected_count) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return false;
}

TEST(PointerEventTest, FindsPointerById) {
  PointerEvent event;
  Pointer first_pointer;
  first_pointer.id = 3;
  first_pointer.x = 12.f;
  first_pointer.y = 24.f;
  event.pointers.push_back(first_pointer);
  Pointer second_pointer;
  second_pointer.id = 7;
  second_pointer.x = 36.f;
  second_pointer.y = 48.f;
  event.pointers.push_back(second_pointer);

  ASSERT_NE(event.FindPointer(7), nullptr);
  EXPECT_FLOAT_EQ(event.FindPointer(7)->x, 36.f);
  EXPECT_EQ(event.FindPointer(5), nullptr);
}

TEST(PointerCapabilitiesTest, SupportsOnlyConcreteSources) {
  PointerCapabilities capabilities;
  capabilities.default_source_type = PointerSourceType::kTouch;
  capabilities.supports_touch = true;

  EXPECT_TRUE(capabilities.Supports(PointerSourceType::kTouch));
  EXPECT_FALSE(capabilities.Supports(PointerSourceType::kMouse));
  EXPECT_FALSE(capabilities.Supports(PointerSourceType::kDefault));
}

TEST(SyntheticTapGestureTest, EmitsDownAndUpThroughInputTarget) {
  RecordingInputEventTarget target;
  SyntheticTapGesture gesture(12.f, 24.f, 50, PointerSourceType::kTouch);

  EXPECT_EQ(gesture.ForwardInputEvents(1000, &target),
            SyntheticGestureResult::kRunning);
  auto events = target.Events();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].type, PointerEventType::kDown);

  EXPECT_EQ(gesture.ForwardInputEvents(51000, &target),
            SyntheticGestureResult::kDone);
  events = target.Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[1].type, PointerEventType::kUp);
  EXPECT_EQ(events[0].action_pointer_id, events[1].action_pointer_id);
}

TEST(SyntheticTapGestureTest, StopsWhenTargetInjectionFails) {
  RecordingInputEventTarget target;
  target.injection_result_ = false;
  SyntheticTapGesture gesture(12.f, 24.f, 0, PointerSourceType::kTouch);

  EXPECT_EQ(gesture.ForwardInputEvents(1000, &target),
            SyntheticGestureResult::kFailed);
  const auto events = target.Events();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].type, PointerEventType::kDown);
}

TEST(SyntheticTapGestureTest, EmitsIndependentTapCount) {
  RecordingInputEventTarget target;
  SyntheticTapGesture gesture(12.f, 24.f, 0, PointerSourceType::kTouch);

  EXPECT_EQ(gesture.ForwardInputEvents(1000, &target),
            SyntheticGestureResult::kDone);
  const auto events = target.Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].click_count, 1);
  EXPECT_EQ(events[1].click_count, 1);
}

TEST(SyntheticTapGestureTest, StopsWhenReleaseInjectionFails) {
  RecordingInputEventTarget target;
  target.fail_on_injection_index_ = 1;
  SyntheticTapGesture gesture(12.f, 24.f, 1, PointerSourceType::kTouch);

  EXPECT_EQ(gesture.ForwardInputEvents(1000, &target),
            SyntheticGestureResult::kRunning);
  EXPECT_EQ(gesture.ForwardInputEvents(2000, &target),
            SyntheticGestureResult::kFailed);
  const auto events = target.Events();
  ASSERT_EQ(events.size(), 3u);
  EXPECT_EQ(events[1].type, PointerEventType::kUp);
  EXPECT_EQ(events[2].type, PointerEventType::kCancel);
}

TEST(SyntheticTapGestureTest, CancelReleasesAnActivePointer) {
  RecordingInputEventTarget target;
  SyntheticTapGesture gesture(12.f, 24.f, 50, PointerSourceType::kTouch);

  EXPECT_EQ(gesture.ForwardInputEvents(1000, &target),
            SyntheticGestureResult::kRunning);
  gesture.Cancel(2000, &target);

  const auto events = target.Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].type, PointerEventType::kDown);
  EXPECT_EQ(events[1].type, PointerEventType::kCancel);
  EXPECT_EQ(events[0].action_pointer_id, events[1].action_pointer_id);
}

TEST(SyntheticGestureControllerTest, CompletesWhenVSyncDoesNotRespond) {
  auto target = std::make_shared<RecordingInputEventTarget>();
  fml::Thread thread("synthetic_gesture_test");
  std::shared_ptr<SyntheticGestureController> controller;
  std::promise<SyntheticGestureResult> completion;
  auto result = completion.get_future();

  thread.GetTaskRunner()->PostSyncTask(
      [&controller, &target, &thread, &completion]() {
        controller =
            SyntheticGestureController::Create(target, thread.GetTaskRunner());
        controller->QueueSyntheticGesture(
            std::make_unique<SyntheticTapGesture>(12.f, 24.f, 0,
                                                  PointerSourceType::kTouch),
            [&completion](SyntheticGestureResult result) {
              completion.set_value(result);
            });
      });

  ASSERT_EQ(result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(result.get(), SyntheticGestureResult::kDone);
  const auto events = target->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].type, PointerEventType::kDown);
  EXPECT_EQ(events[1].type, PointerEventType::kUp);
  thread.GetTaskRunner()->PostSyncTask([&controller]() { controller.reset(); });
}

TEST(SyntheticGestureControllerTest, HonorsDurationWhenVSyncDoesNotRespond) {
  auto target = std::make_shared<RecordingInputEventTarget>();
  fml::Thread thread("synthetic_gesture_duration_test");
  std::shared_ptr<SyntheticGestureController> controller;
  std::promise<SyntheticGestureResult> completion;
  auto result = completion.get_future();
  const auto started_at = std::chrono::steady_clock::now();

  thread.GetTaskRunner()->PostSyncTask(
      [&controller, &target, &thread, &completion]() {
        controller =
            SyntheticGestureController::Create(target, thread.GetTaskRunner());
        controller->QueueSyntheticGesture(
            std::make_unique<SyntheticTapGesture>(12.f, 24.f, 100,
                                                  PointerSourceType::kTouch),
            [&completion](SyntheticGestureResult result) {
              completion.set_value(result);
            });
      });

  ASSERT_EQ(result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(result.get(), SyntheticGestureResult::kDone);
  EXPECT_GE(std::chrono::steady_clock::now() - started_at,
            std::chrono::milliseconds(100));
  const auto events = target->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_GE(events[1].timestamp_us - events[0].timestamp_us, 100000);
  thread.GetTaskRunner()->PostSyncTask([&controller]() { controller.reset(); });
}

TEST(SyntheticGestureControllerTest, WaitsForAckBeforeStartingNextGesture) {
  auto target = std::make_shared<RecordingInputEventTarget>();
  target->defer_processing_ = true;
  fml::Thread thread("synthetic_gesture_queue_test");
  std::shared_ptr<SyntheticGestureController> controller;
  std::promise<SyntheticGestureResult> first_completion;
  std::promise<SyntheticGestureResult> second_completion;
  auto first_result = first_completion.get_future();
  auto second_result = second_completion.get_future();

  thread.GetTaskRunner()->PostSyncTask([&]() {
    controller =
        SyntheticGestureController::Create(target, thread.GetTaskRunner());
    controller->QueueSyntheticGesture(
        std::make_unique<SyntheticTapGesture>(12.f, 24.f, 0,
                                              PointerSourceType::kTouch),
        [&first_completion](SyntheticGestureResult result) {
          first_completion.set_value(result);
        });
    controller->QueueSyntheticGesture(
        std::make_unique<SyntheticTapGesture>(36.f, 48.f, 0,
                                              PointerSourceType::kTouch),
        [&second_completion](SyntheticGestureResult result) {
          second_completion.set_value(result);
        });
  });

  ASSERT_TRUE(WaitForEventCount(target, 2u));
  EXPECT_EQ(first_result.wait_for(std::chrono::milliseconds(0)),
            std::future_status::timeout);
  EXPECT_EQ(target->EventCount(), 2u);

  thread.GetTaskRunner()->PostSyncTask(
      [&target]() { target->CompleteNextProcessing(true); });
  ASSERT_EQ(first_result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(first_result.get(), SyntheticGestureResult::kDone);
  ASSERT_TRUE(WaitForEventCount(target, 4u));

  thread.GetTaskRunner()->PostSyncTask(
      [&target]() { target->CompleteNextProcessing(true); });
  ASSERT_EQ(second_result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(second_result.get(), SyntheticGestureResult::kDone);

  const auto events = target->Events();
  ASSERT_EQ(events.size(), 4u);
  EXPECT_FLOAT_EQ(events[0].pointers[0].x, 12.f);
  EXPECT_FLOAT_EQ(events[2].pointers[0].x, 36.f);
  thread.GetTaskRunner()->PostSyncTask([&controller]() { controller.reset(); });
}

TEST(SyntheticGestureControllerTest, FailsTimedOutAckAndStartsNextGesture) {
  auto target = std::make_shared<RecordingInputEventTarget>();
  target->defer_processing_ = true;
  fml::Thread thread("synthetic_gesture_ack_timeout_test");
  std::shared_ptr<SyntheticGestureController> controller;
  std::promise<SyntheticGestureResult> first_completion;
  std::promise<SyntheticGestureResult> second_completion;
  auto first_result = first_completion.get_future();
  auto second_result = second_completion.get_future();

  thread.GetTaskRunner()->PostSyncTask([&]() {
    controller =
        SyntheticGestureController::Create(target, thread.GetTaskRunner());
    controller->QueueSyntheticGesture(
        std::make_unique<SyntheticTapGesture>(12.f, 24.f, 0,
                                              PointerSourceType::kTouch),
        [&first_completion, &target](SyntheticGestureResult result) {
          target->defer_processing_ = false;
          first_completion.set_value(result);
        });
    controller->QueueSyntheticGesture(
        std::make_unique<SyntheticTapGesture>(36.f, 48.f, 0,
                                              PointerSourceType::kTouch),
        [&second_completion](SyntheticGestureResult result) {
          second_completion.set_value(result);
        });
  });

  ASSERT_TRUE(WaitForEventCount(target, 2u));
  thread.GetTaskRunner()->PostSyncTask([&controller]() {
    // Simulate the watchdog firing without waiting for the production timeout.
    controller->OnInputProcessed(controller->input_ack_id_, false);
  });

  ASSERT_EQ(first_result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(first_result.get(), SyntheticGestureResult::kFailed);
  ASSERT_EQ(second_result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(second_result.get(), SyntheticGestureResult::kDone);

  const auto events = target->Events();
  ASSERT_EQ(events.size(), 4u);
  EXPECT_FLOAT_EQ(events[0].pointers[0].x, 12.f);
  EXPECT_FLOAT_EQ(events[2].pointers[0].x, 36.f);

  thread.GetTaskRunner()->PostSyncTask(
      [&target]() { target->CompleteNextProcessing(true); });
  EXPECT_EQ(target->EventCount(), 4u);
  thread.GetTaskRunner()->PostSyncTask([&controller]() { controller.reset(); });
}

TEST(SyntheticGestureControllerTest, ReportsInputProcessingFailure) {
  auto target = std::make_shared<RecordingInputEventTarget>();
  target->defer_processing_ = true;
  fml::Thread thread("synthetic_gesture_failure_test");
  std::shared_ptr<SyntheticGestureController> controller;
  std::promise<SyntheticGestureResult> completion;
  auto result = completion.get_future();

  thread.GetTaskRunner()->PostSyncTask([&]() {
    controller =
        SyntheticGestureController::Create(target, thread.GetTaskRunner());
    controller->QueueSyntheticGesture(
        std::make_unique<SyntheticTapGesture>(12.f, 24.f, 0,
                                              PointerSourceType::kTouch),
        [&completion](SyntheticGestureResult result) {
          completion.set_value(result);
        });
  });

  ASSERT_TRUE(WaitForEventCount(target, 2u));
  thread.GetTaskRunner()->PostSyncTask(
      [&target]() { target->CompleteNextProcessing(false); });
  ASSERT_EQ(result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(result.get(), SyntheticGestureResult::kFailed);
  thread.GetTaskRunner()->PostSyncTask([&controller]() { controller.reset(); });
}

TEST(SyntheticGestureControllerTest,
     CompletionMayReleaseLastExternalControllerReference) {
  auto target = std::make_shared<RecordingInputEventTarget>();
  fml::Thread thread("synthetic_gesture_lifetime_test");
  std::shared_ptr<SyntheticGestureController> controller;
  std::promise<SyntheticGestureResult> completion;
  auto result = completion.get_future();

  thread.GetTaskRunner()->PostSyncTask([&]() {
    controller =
        SyntheticGestureController::Create(target, thread.GetTaskRunner());
    controller->QueueSyntheticGesture(
        std::make_unique<SyntheticTapGesture>(12.f, 24.f, 0,
                                              PointerSourceType::kTouch),
        [&controller, &completion](SyntheticGestureResult result) {
          controller.reset();
          completion.set_value(result);
        });
  });

  ASSERT_EQ(result.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  EXPECT_EQ(result.get(), SyntheticGestureResult::kDone);
  thread.GetTaskRunner()->PostSyncTask(
      [&controller]() { EXPECT_EQ(controller, nullptr); });
}

}  // namespace
}  // namespace input
}  // namespace devtool
}  // namespace lynx
