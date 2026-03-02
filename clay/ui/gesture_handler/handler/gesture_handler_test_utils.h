// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_UI_GESTURE_HANDLER_HANDLER_GESTURE_HANDLER_TEST_UTILS_H_
#define CLAY_UI_GESTURE_HANDLER_HANDLER_GESTURE_HANDLER_TEST_UTILS_H_

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "clay/public/event_delegate.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"
#include "clay/ui/gesture_handler/gesture_detector.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace clay {
namespace testing {

class TestTaskRunner : public fml::TaskRunner {
 public:
  static fml::RefPtr<TestTaskRunner> Create() {
    return fml::AdoptRef(new TestTaskRunner());
  }

  void PostTask(lynx::base::closure task) override {
    Enqueue(std::move(task), now_);
  }

  void PostTaskForTime(lynx::base::closure task,
                       fml::TimePoint target_time) override {
    Enqueue(std::move(task), target_time);
  }

  void PostDelayedTask(lynx::base::closure task,
                       fml::TimeDelta delay) override {
    Enqueue(std::move(task), now_ + delay);
  }

  bool RunsTasksOnCurrentThread() override { return true; }

  fml::TaskQueueId GetTaskQueueId() override { return fml::TaskQueueId(0); }

  void AdvanceBy(fml::TimeDelta delta) {
    now_ = now_ + delta;
    RunUntilIdle();
  }

  void RunUntilIdle() {
    for (;;) {
      auto it =
          std::min_element(tasks_.begin(), tasks_.end(),
                           [](const ScheduledTask& a, const ScheduledTask& b) {
                             if (a.when == b.when) {
                               return a.seq < b.seq;
                             }
                             return a.when < b.when;
                           });
      if (it == tasks_.end() || it->when > now_) {
        break;
      }
      auto task = std::move(it->task);
      tasks_.erase(it);
      if (task) {
        task();
      }
    }
  }

 private:
  struct ScheduledTask {
    fml::TimePoint when;
    uint64_t seq;
    lynx::base::closure task;
  };

  TestTaskRunner() : TaskRunner(fml::RefPtr<fml::MessageLoopImpl>()) {}

  void Enqueue(lynx::base::closure task, fml::TimePoint when) {
    tasks_.push_back(ScheduledTask{when, next_seq_++, std::move(task)});
  }

  fml::TimePoint now_ = fml::TimePoint::Now();
  uint64_t next_seq_ = 0;
  std::vector<ScheduledTask> tasks_;
};

class MockEventDelegate : public EventDelegate {
 public:
  MOCK_METHOD(void, OnGestureHandlerEvent,
              (const std::string& event_name, int view_id, uint32_t gesture_id,
               float x, float y, float page_x, float page_y, int64_t timestamp,
               Value& additional_params),
              (override));

  void OnTouchEvent(const std::string&, int, float, float, float,
                    float) override {}
  void OnMouseEvent(const std::string&, int, int, int, float, float, float,
                    float, float) override {}
  void OnWheelEvent(const std::string&, int, float, float, float, float, float,
                    float) override {}
  void OnKeyEvent(const std::string&, int, const char*, bool) override {}
  void OnAnimationEvent(const std::string&, const char*, int) override {}
  void OnTransitionEvent(const std::string&, const char*, int,
                         ClayAnimationPropertyType) override {}
  void OnFocusChanged(int, bool) override {}
  void OnHoverChanged(int, bool) override {}
  void OnDragDropEvent(const std::string&, int, clay::Value::Map) override {}
  void OnViewportMetricsChanged(double, double, double, double, double, double,
                                double, bool) override {}
  void OnDrawEndEvent() override {}
  void OnSendCustomEvent(int, const std::string&, clay::Value::Map) override {}
  void OnSendGlobalEvent(const std::string&, clay::Value) override {}
  void OnFirstMeaningfulPaint() override {}
  void OnOverlayEvent(int, const char*, int, const char**,
                      const char*) override {}
  void OnLayoutChanged(int, clay::Value::Map) override {}
  void OnIntersectionEvent(int, clay::Value::Map) override {}
  void OnCallJSApiCallback(int, clay::Value) override {}
  void CallJSIntersectionObserver(int, int, clay::Value) override {}
};

class TestGestureArenaMember : public GestureArenaMember {
 public:
  explicit TestGestureArenaMember(int sign)
      : sign_(sign), member_id_(sign), weak_factory_(this) {}

  fml::WeakPtr<TestGestureArenaMember> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void SetCanConsume(bool can_consume) { can_consume_ = can_consume; }
  void SetScrollContainerDirection(int8_t direction) {
    scroll_container_direction_ = direction;
  }
  void SetBorder(bool is_start, bool at_border) {
    if (is_start) {
      at_start_ = at_border;
    } else {
      at_end_ = at_border;
    }
  }
  void SetScroll(float x, float y) {
    scroll_x_ = x;
    scroll_y_ = y;
  }
  void SetGestureDetectorMap(GestureMap detectors) {
    detectors_ = std::move(detectors);
  }
  void SetGestureHandlers(GestureHandlerMap handlers) {
    handlers_ = std::move(handlers);
  }

  std::vector<std::pair<float, float>> TakeScrollCalls() {
    return std::exchange(scroll_calls_, {});
  }

  std::vector<float> GestureScrollBy(float delta_x, float delta_y) override {
    scroll_calls_.push_back({delta_x, delta_y});
    scroll_x_ += delta_x;
    scroll_y_ += delta_y;
    return {scroll_x_, scroll_y_};
  }

  bool CanConsumeGesture(float, float) override { return can_consume_; }

  int Sign() const override { return sign_; }

  int GestureArenaMemberId() override { return member_id_; }

  float ScrollX() override { return scroll_x_; }

  int8_t GetScrollContainerDirection() override {
    return scroll_container_direction_;
  }

  bool IsAtBorder(bool is_start) override {
    return is_start ? at_start_ : at_end_;
  }

  float ScrollY() override { return scroll_y_; }

  const GestureMap& GetGestureDetectorMap() override { return detectors_; }

  const GestureHandlerMap& GetGestureHandlers() override { return handlers_; }

 private:
  int sign_;
  int member_id_;
  bool can_consume_ = true;
  int8_t scroll_container_direction_ = 0;
  bool at_start_ = false;
  bool at_end_ = false;
  float scroll_x_ = 0;
  float scroll_y_ = 0;
  std::vector<std::pair<float, float>> scroll_calls_;
  GestureMap detectors_;
  GestureHandlerMap handlers_;
  fml::WeakPtrFactory<TestGestureArenaMember> weak_factory_;
};

inline PointerEvent MakePointerEvent(PointerEvent::EventType type,
                                     FloatPoint position,
                                     uint64_t timestamp = 0) {
  PointerEvent event(type);
  event.position = position;
  event.timestamp = timestamp;
  return event;
}
}  // namespace testing

inline std::shared_ptr<GestureDetector> MakeDetector(
    uint32_t id, GestureHandlerType type,
    std::vector<std::string> callbacks = {},
    std::unordered_map<std::string, std::vector<uint32_t>> relation_map = {},
    Value config = {}) {
  return std::make_shared<GestureDetector>(id, type, callbacks, relation_map,
                                           std::move(config));
}

inline std::unique_ptr<PageView> MakeTestPageView(
    uint32_t id, const fml::RefPtr<fml::TaskRunner>& ui_task_runner) {
  return std::make_unique<PageView>(id, nullptr, ui_task_runner);
}

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_GESTURE_HANDLER_TEST_UTILS_H_
