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
  MOCK_METHOD(void, OnUnifiedGestureHandlerEvent,
              (const std::string& event_name, int view_id, uint32_t gesture_id,
               Value params),
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

inline PointerEvent MakePointerEvent(PointerEvent::EventType type,
                                     FloatPoint position,
                                     uint64_t timestamp = 0) {
  PointerEvent event(type);
  event.position = position;
  event.timestamp = timestamp;
  return event;
}
}  // namespace testing

inline lynx::tasm::gesture::GestureDefinition MakeDetector(
    uint32_t id, lynx::tasm::GestureType type,
    std::vector<lynx::tasm::gesture::GestureCallbackType> callbacks = {}) {
  lynx::tasm::gesture::GestureDefinition definition;
  definition.gesture_id = id;
  definition.gesture_type = type;
  definition.callbacks = std::move(callbacks);
  return definition;
}

inline std::unique_ptr<PageView> MakeTestPageView(
    uint32_t id, const fml::RefPtr<fml::TaskRunner>& ui_task_runner) {
  return std::make_unique<PageView>(id, nullptr, ui_task_runner);
}

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_GESTURE_HANDLER_TEST_UTILS_H_
