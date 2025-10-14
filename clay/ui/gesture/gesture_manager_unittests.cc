// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <future>
#include <memory>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/thread.h"
#include "clay/gfx/scroll_direction.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/long_press_gesture_recognizer.h"
#include "clay/ui/gesture/macros.h"
#include "clay/ui/gesture/multi_tap_gesture_recognizer.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace testing {

constexpr uint64_t kFastMoveTime = 1000ul;      // in micro second
constexpr uint64_t kLongPressTimeout = 1000ul;  // in milli second

static int ID() {
  static int id = 0;
  return id++;
}

PointerEvent CreateSinglePointer(int pointer_id, PointerEvent::EventType type,
                                 FloatPoint position = FloatPoint()) {
  PointerEvent pointer(type);
  pointer.buttons = PointerEvent::kPrimary;
  pointer.pointer_id = pointer_id;
  pointer.position = position;
  return pointer;
}

std::vector<PointerEvent> CreatePointer(int pointer_id,
                                        PointerEvent::EventType type,
                                        FloatPoint position = FloatPoint()) {
  return {CreateSinglePointer(pointer_id, type, position)};
}

void MovePointer(PointerEvent& pointer, const FloatSize& delta,
                 uint64_t time_elapsed) {
  pointer.type = PointerEvent::EventType::kMoveEvent;
  pointer.delta = delta;
  pointer.position.MoveBy(delta);
  pointer.timestamp += time_elapsed;
}

void UpPointer(PointerEvent& pointer) {
  pointer.type = PointerEvent::EventType::kUpEvent;
  pointer.delta = FloatSize();
}

#define MOVE_MULTI_STEPS(steps, args...) \
  for (int i = 0; i < steps; ++i) {      \
    MovePointer(args);                   \
  }

// Default target do nothing.
class MockHitTestTargetBase : public HitTestTarget {
 public:
  virtual ~MockHitTestTargetBase() = default;
  void HandleEvent(const PointerEvent& event) override {}
  bool HasDragGestureRecognizer(ScrollDirection direction) const override {
    return false;
  }
  bool HasTapGestureRecognizer() const override { return false; }
  bool HasLongPressGestureRecognizer() const override { return false; }
  bool HasTapEvent() const override { return false; }
  bool HasLongPressEvent() const override { return false; }
  bool ShouldBlockNativeEvent() const override { return false; }
};

class MultiRecognizerHitTestTarget : public MockHitTestTargetBase {
 public:
  void HandleEvent(const PointerEvent& event) override {
    if (event.type == PointerEvent::EventType::kDownEvent) {
      for (const auto& recognizer : recognizers_) {
        recognizer->AddPointer(event);
      }
    }
  }

  std::list<std::unique_ptr<GestureRecognizer>> recognizers_;
};

class MockHitTestable : public HitTestable {
 public:
  MockHitTestable() = default;
  virtual ~MockHitTestable() = default;

  bool HitTest(const PointerEvent& event, HitTestResult& result) override {
    result = hit_test_result_;
    return true;
  }

  // Simplify hit test process.
  HitTestResult hit_test_result_;
};

class GestureManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    thread_ = std::make_unique<fml::Thread>("gesture");
    fml::AutoResetWaitableEvent latch;
    task_runner()->PostTask([&]() {
      hit_test_root_.reset(new MockHitTestable());
      gesture_manager_.reset(new GestureManager());
      latch.Signal();
    });
    latch.Wait();
  }

  void TearDown() override {
    fml::AutoResetWaitableEvent latch;
    task_runner()->PostTask([&]() {
      hit_test_root_.reset();
      gesture_manager_.reset();
      latch.Signal();
    });
    latch.Wait();
    thread_ = nullptr;
  }

  fml::RefPtr<fml::TaskRunner> task_runner() {
    return thread_->GetTaskRunner();
  }

  void AddHitTestTarget(HitTestTarget* target) {
    hit_test_root_->hit_test_result_.emplace_back(
        target->GetHitTestTargetWeakPtr());
  }

  void ClearHitTestTarget() { hit_test_root_->hit_test_result_.clear(); }

  GestureManager* gesture_manager() { return gesture_manager_.get(); }

  HitTestable* root() { return hit_test_root_.get(); }

 private:
  std::unique_ptr<MockHitTestable> hit_test_root_;
  std::unique_ptr<GestureManager> gesture_manager_;

  // long press or double tap needs timer.
  std::unique_ptr<fml::Thread> thread_;
};

// Test with abnormal case like pointers are not come by sequence.
// For example, Up/Move comes before Down. Or event with type Unknown.
// No crash is what we want.
TEST_F(GestureManagerTest, Exception_Test) {
  int pointer_id = ID();

  auto pointers =
      CreatePointer(pointer_id, PointerEvent::EventType::kUnkownEvent);
  gesture_manager()->HandlePointerEvents(root(), pointers);
  auto pointers2 =
      CreatePointer(pointer_id, PointerEvent::EventType::kMoveEvent);
  gesture_manager()->HandlePointerEvents(root(), pointers2);
  auto pointers3 = CreatePointer(pointer_id, PointerEvent::EventType::kUpEvent);
  gesture_manager()->HandlePointerEvents(root(), pointers3);
  auto pointers4 =
      CreatePointer(pointer_id, PointerEvent::EventType::kDownEvent);
  gesture_manager()->HandlePointerEvents(root(), pointers4);
  auto pointers5 = CreatePointer(pointer_id, PointerEvent::EventType::kUpEvent);
  gesture_manager()->HandlePointerEvents(root(), pointers5);
}

TEST_F(GestureManagerTest, NoConflict_Tap_Test) {
  auto empty_target = std::make_unique<MockHitTestTargetBase>();
  AddHitTestTarget(empty_target.get());

  auto target_with_tap = std::make_unique<MultiRecognizerHitTestTarget>();
  auto tap_recognizer =
      std::make_unique<TapGestureRecognizer>(gesture_manager());
  bool tapped = false;
  tap_recognizer->SetTapUpCallback(
      [&tapped](const PointerEvent& pointer) { tapped = true; });
  target_with_tap->recognizers_.emplace_back(std::move(tap_recognizer));
  AddHitTestTarget(target_with_tap.get());

  {
    auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer = pointers[0];

    gesture_manager()->HandlePointerEvents(root(), pointers);
    // Although tap recognizer has already won, tap shouldn't callback until
    // up.
    EXPECT_TRUE(!tapped);

    MovePointer(pointer, FloatSize(1.f, 1.f), kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers);
    EXPECT_TRUE(!tapped);

    UpPointer(pointer);
    gesture_manager()->HandlePointerEvents(root(), pointers);
    EXPECT_TRUE(tapped);
  }

  // Test drift too much.
  {
    tapped = false;
    auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer = pointers[0];

    gesture_manager()->HandlePointerEvents(root(), pointers);
    MovePointer(pointer, FloatSize(100.f, 0.f), kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers);
    MovePointer(pointer, FloatSize(-100.f, 0.f), kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers);
    UpPointer(pointer);
    gesture_manager()->HandlePointerEvents(root(), pointers);
    EXPECT_TRUE(!tapped);
  }
}

enum LongPressStatus : uint32_t {
  kNoLongPressStatus = 0,
  kLongPressDown = 1 << 0,
  kLongPressStart = 1 << 1,
  kLongPressMove = 1 << 2,
  kLongPressEnd = 1 << 3,
  kLongPressCancel = 1 << 4,
};

TEST_F(GestureManagerTest, NoConflict_Long_Press_Test) {
  constexpr float kDriftTolerance = 20.f;
  fml::AutoResetWaitableEvent latch;
  auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
  std::unique_ptr<MockHitTestTargetBase> empty_target;
  std::unique_ptr<MultiRecognizerHitTestTarget> hit_test_target;
  std::unique_ptr<LongPressGestureRecognizer> long_press_recognizer;
  LongPressStatus long_press_status = kNoLongPressStatus;
  LongPressStatus expected_status = kLongPressDown;

  task_runner()->PostTask([&, this]() {
    empty_target = std::make_unique<MockHitTestTargetBase>();
    AddHitTestTarget(empty_target.get());

    hit_test_target = std::make_unique<MultiRecognizerHitTestTarget>();
    long_press_recognizer = std::make_unique<LongPressGestureRecognizer>(
        gesture_manager(), kLongPressTimeout);
    long_press_recognizer->SetTaskRunner(task_runner());
    long_press_recognizer->SetDriftTolerance(kDriftTolerance);
    long_press_recognizer->SetLongPressDownCallback(
        [&long_press_status](const PointerEvent&) {
          long_press_status =
              static_cast<LongPressStatus>(long_press_status | kLongPressDown);
        });
    long_press_recognizer->SetLongPressStartCallback(
        [&long_press_status](const PointerEvent&) {
          long_press_status =
              static_cast<LongPressStatus>(long_press_status | kLongPressStart);
        });
    long_press_recognizer->SetLongPressMoveCallback(
        [&long_press_status](const PointerEvent&) {
          long_press_status =
              static_cast<LongPressStatus>(long_press_status | kLongPressMove);
        });
    long_press_recognizer->SetLongPressEndCallback(
        [&long_press_status](const PointerEvent&) {
          long_press_status =
              static_cast<LongPressStatus>(long_press_status | kLongPressEnd);
        });
    long_press_recognizer->SetLongPressCancelCallback([&long_press_status]() {
      long_press_status =
          static_cast<LongPressStatus>(long_press_status | kLongPressCancel);
    });

    hit_test_target->recognizers_.emplace_back(
        std::move(long_press_recognizer));
    AddHitTestTarget(hit_test_target.get());

    // Normal case
    {
      long_press_status = kNoLongPressStatus;
      auto& pointer = pointers[0];

      gesture_manager()->HandlePointerEvents(root(), pointers);
      EXPECT_EQ(long_press_status, expected_status);
      MovePointer(pointer, FloatSize(kDriftTolerance - 1.f, 0.f),
                  kFastMoveTime);
      gesture_manager()->HandlePointerEvents(root(), pointers);
      // Not trigger start
      EXPECT_EQ(long_press_status, expected_status);

      task_runner()->PostDelayedTask(
          [&]() {
            expected_status =
                static_cast<LongPressStatus>(expected_status | kLongPressStart);
            EXPECT_EQ(long_press_status, expected_status);

            MovePointer(pointer, FloatSize(1.f, 0.f), kFastMoveTime);
            gesture_manager()->HandlePointerEvents(root(), pointers);
            expected_status =
                static_cast<LongPressStatus>(expected_status | kLongPressMove);
            EXPECT_EQ(long_press_status, expected_status);

            UpPointer(pointer);
            gesture_manager()->HandlePointerEvents(root(), pointers);
            expected_status =
                static_cast<LongPressStatus>(expected_status | kLongPressEnd);
            EXPECT_EQ(long_press_status, expected_status);

            latch.Signal();
          },
          fml::TimeDelta::FromMilliseconds(kLongPressTimeout));
    }
  });
  latch.Wait();

  // Test drift too much.
  {
    latch.Reset();
    long_press_status = kNoLongPressStatus;
    expected_status = kLongPressDown;

    pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer = pointers[0];

    task_runner()->PostTask([&]() {
      gesture_manager()->HandlePointerEvents(root(), pointers);
      EXPECT_EQ(long_press_status, expected_status);
      MovePointer(pointer, FloatSize(kDriftTolerance + 1.f, 0.f),
                  kFastMoveTime);
      gesture_manager()->HandlePointerEvents(root(), pointers);
      expected_status =
          static_cast<LongPressStatus>(expected_status | kLongPressCancel);
      EXPECT_EQ(long_press_status, expected_status);

      task_runner()->PostDelayedTask(
          [&]() {
            EXPECT_EQ(long_press_status, expected_status);

            MovePointer(pointer, FloatSize(1.f, 0.f), kFastMoveTime);
            gesture_manager()->HandlePointerEvents(root(), pointers);
            EXPECT_EQ(long_press_status, expected_status);

            UpPointer(pointer);
            gesture_manager()->HandlePointerEvents(root(), pointers);
            EXPECT_EQ(long_press_status, expected_status);

            hit_test_target.reset();  // reset at last.
            empty_target.reset();

            latch.Signal();
          },
          fml::TimeDelta::FromMilliseconds(kLongPressTimeout));
    });
    latch.Wait();
  }
}

enum DragStatus : uint8_t {
  kNoDragStatus = 0,
  kHasDragDown = 1 << 0,
  kHasDragStart = 1 << 1,
  kHasDragUpdate = 1 << 2,
  kHasDragEnd = 1 << 3,
  kHasDragCancel = 1 << 4,
};

TEST_F(GestureManagerTest, NoConflict_Drag_Normal_Test) {
  // hypotenuse value is 10.f
  const FloatSize kMoveStep(6.f, 8.f);

  auto target_with_drag = std::make_unique<MultiRecognizerHitTestTarget>();

  DragStatus expected_status = kNoDragStatus;
  DragStatus status = kNoDragStatus;
  auto drag_recognizer =
      std::make_unique<DragGestureRecognizer>(gesture_manager());
  drag_recognizer->SetTouchSlop(18.f);
  drag_recognizer->SetDragDownCallback([&status](const PointerEvent& pointer) {
    status = static_cast<DragStatus>(status | kHasDragDown);
  });
  drag_recognizer->SetDragStartCallback([&status](const FloatPoint& position) {
    status = static_cast<DragStatus>(status | kHasDragStart);
  });
  drag_recognizer->SetDragUpdateCallback(
      [&status](const FloatPoint& position, const FloatSize& delta) {
        status = static_cast<DragStatus>(status | kHasDragUpdate);
      });
  drag_recognizer->SetDragEndCallback([&status](const Velocity& velocity) {
    status = static_cast<DragStatus>(status | kHasDragEnd);
  });
  drag_recognizer->SetDragCancelCallback([&status]() {
    status = static_cast<DragStatus>(status | kHasDragCancel);
  });
  target_with_drag->recognizers_.emplace_back(std::move(drag_recognizer));
  AddHitTestTarget(target_with_drag.get());

  auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
  auto& pointer = pointers[0];
  gesture_manager()->HandlePointerEvents(root(), pointers);
  // Accepted immediately because no conflict.
  expected_status = static_cast<DragStatus>(expected_status | kHasDragStart);
  expected_status = static_cast<DragStatus>(expected_status | kHasDragDown);
  EXPECT_EQ(status, expected_status);

  MovePointer(pointer, kMoveStep, kFastMoveTime);
  gesture_manager()->HandlePointerEvents(root(), pointers);
  expected_status = static_cast<DragStatus>(expected_status | kHasDragUpdate);
  EXPECT_EQ(status, expected_status);

  for (int i = 0; i < 10; ++i) {
    MovePointer(pointer, kMoveStep, kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers);
  }

  pointer.type = PointerEvent::EventType::kUpEvent;
  gesture_manager()->HandlePointerEvents(root(), pointers);
  expected_status = static_cast<DragStatus>(expected_status | kHasDragEnd);
  EXPECT_EQ(status, expected_status);
}

// Mixed tap & drag & long press
TEST_F(GestureManagerTest, Conflict_Drag_Tap_LongPress_Test) {
  // hypotenuse value is 10.f
  const FloatSize kMoveStep(6.f, 8.f);

  auto hit_test_target = std::make_unique<MultiRecognizerHitTestTarget>();

  DragStatus status = kNoDragStatus;
  auto drag_recognizer =
      std::make_unique<DragGestureRecognizer>(gesture_manager());
  drag_recognizer->SetTouchSlop(18.f);
  drag_recognizer->SetDragDownCallback([&status](const PointerEvent& pointer) {
    status = static_cast<DragStatus>(status | kHasDragDown);
  });
  drag_recognizer->SetDragStartCallback([&status](const FloatPoint& position) {
    status = static_cast<DragStatus>(status | kHasDragStart);
  });
  drag_recognizer->SetDragUpdateCallback(
      [&status](const FloatPoint& position, const FloatSize& delta) {
        status = static_cast<DragStatus>(status | kHasDragUpdate);
      });
  drag_recognizer->SetDragEndCallback([&status](const Velocity& velocity) {
    status = static_cast<DragStatus>(status | kHasDragEnd);
  });
  drag_recognizer->SetDragCancelCallback([&status]() {
    status = static_cast<DragStatus>(status | kHasDragCancel);
  });

  auto tap_recognizer =
      std::make_unique<TapGestureRecognizer>(gesture_manager());
  bool tapped = false;
  tap_recognizer->SetTapUpCallback(
      [&tapped](const PointerEvent& pointer) { tapped = true; });
  // Set large tolerance in case tap reject before drag start.
  tap_recognizer->SetDriftTolerance(50.f);

  auto long_press_recognizer = std::make_unique<LongPressGestureRecognizer>(
      gesture_manager(), kLongPressTimeout);
  bool long_press_started = false;
  long_press_recognizer->SetTaskRunner(task_runner());
  long_press_recognizer->SetDriftTolerance(20.f);
  long_press_recognizer->SetLongPressStartCallback(
      [&long_press_started](const PointerEvent&) {
        long_press_started = true;
      });

  hit_test_target->recognizers_.emplace_back(std::move(tap_recognizer));
  hit_test_target->recognizers_.emplace_back(std::move(drag_recognizer));
  hit_test_target->recognizers_.emplace_back(std::move(long_press_recognizer));
  AddHitTestTarget(hit_test_target.get());

  {
    DragStatus expected_status = kNoDragStatus;
    status = kNoDragStatus;
    // Begin drag gesture
    auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer = pointers[0];
    gesture_manager()->HandlePointerEvents(root(), pointers);
    // Because of tap conflict, cannot resolve immediately.
    expected_status = static_cast<DragStatus>(expected_status | kHasDragDown);
    EXPECT_EQ(status, expected_status);

    MovePointer(pointer, kMoveStep, kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers);
    // Not reach touch slop for recognizing as drag gesture. (10.f < 18.f)
    EXPECT_EQ(status, expected_status);
    MovePointer(pointer, kMoveStep, kFastMoveTime);
    // Moved enough space for drag gesture (20.f > 18.f)
    gesture_manager()->HandlePointerEvents(root(), pointers);
    expected_status = static_cast<DragStatus>(expected_status | kHasDragStart);
    EXPECT_EQ(status, expected_status);

    for (int i = 0; i < 10; ++i) {
      MovePointer(pointer, kMoveStep, kFastMoveTime);
      gesture_manager()->HandlePointerEvents(root(), pointers);
    }
    expected_status = static_cast<DragStatus>(expected_status | kHasDragUpdate);

    pointer.type = PointerEvent::EventType::kUpEvent;
    gesture_manager()->HandlePointerEvents(root(), pointers);
    expected_status = static_cast<DragStatus>(expected_status | kHasDragEnd);
    EXPECT_EQ(status, expected_status);
  }

  EXPECT_FALSE(tapped);
  {
    status = kNoDragStatus;
    // Begin tap gesture: Down -> Move little -> Up
    auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer = pointers[0];
    gesture_manager()->HandlePointerEvents(root(), pointers);
    MovePointer(pointer, kMoveStep, kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers);

    pointer.type = PointerEvent::EventType::kUpEvent;
    gesture_manager()->HandlePointerEvents(root(), pointers);
    EXPECT_TRUE(tapped);
    // Now that tap is accepted, drag should be canceled.
    EXPECT_EQ(static_cast<uint8_t>(status & kHasDragCancel), kHasDragCancel);
  }

  {
    // Test recognizer reuse.
    status = kNoDragStatus;
    // Begin drag gesture
    auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer = pointers[0];
    gesture_manager()->HandlePointerEvents(root(), pointers);
    MovePointer(pointer, kMoveStep, kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers);

    for (int i = 0; i < 10; ++i) {
      MovePointer(pointer, kMoveStep, kFastMoveTime);
      gesture_manager()->HandlePointerEvents(root(), pointers);
    }

    pointer.type = PointerEvent::EventType::kUpEvent;
    gesture_manager()->HandlePointerEvents(root(), pointers);
    EXPECT_EQ(status, 0xf);  // Down | Start | Update | End
  }

  EXPECT_FALSE(long_press_started);
}

// Test when same gesture in parent/child, child will always win.
TEST_F(GestureManagerTest, HitTest_Hierarchy_Test) {
  int tap_flag = 0;

  auto target_child = std::make_unique<MultiRecognizerHitTestTarget>();
  auto child_recognizer =
      std::make_unique<TapGestureRecognizer>(gesture_manager());
  child_recognizer->SetTapUpCallback(
      [&tap_flag](const PointerEvent& pointer) { tap_flag = 1; });
  target_child->recognizers_.emplace_back(std::move(child_recognizer));

  auto target_parent = std::make_unique<MultiRecognizerHitTestTarget>();
  auto parent_recognizer =
      std::make_unique<TapGestureRecognizer>(gesture_manager());
  parent_recognizer->SetTapUpCallback(
      [&tap_flag](const PointerEvent& pointer) { tap_flag = 2; });
  target_parent->recognizers_.emplace_back(std::move(parent_recognizer));

  AddHitTestTarget(target_child.get());
  AddHitTestTarget(target_parent.get());

  int pointer_id = ID();
  auto pointers =
      CreatePointer(pointer_id, PointerEvent::EventType::kDownEvent);
  gesture_manager()->HandlePointerEvents(root(), pointers);
  auto pointers2 = CreatePointer(pointer_id, PointerEvent::EventType::kUpEvent);
  gesture_manager()->HandlePointerEvents(root(), pointers2);
  EXPECT_EQ(tap_flag, 1);  // child wins.
}

TEST_F(GestureManagerTest, NoConflict_MultiTap_Test) {
  std::unique_ptr<MockHitTestTargetBase> empty_target;
  std::unique_ptr<MultiRecognizerHitTestTarget> hit_test_target;
  std::unique_ptr<MultiTapGestureRecognizer> multi_tap_recognizer;
  fml::AutoResetWaitableEvent latch;
  int tap_counts = 0;

  auto init_all = [&]() {
    tap_counts = 0;
    empty_target = std::make_unique<MockHitTestTargetBase>();
    AddHitTestTarget(empty_target.get());

    hit_test_target = std::make_unique<MultiRecognizerHitTestTarget>();
    multi_tap_recognizer =
        std::make_unique<MultiTapGestureRecognizer>(gesture_manager());
    multi_tap_recognizer->SetMultiTapCallback(
        [&tap_counts](const PointerEvent& pointer, int count) {
          tap_counts = count;
        });
    multi_tap_recognizer->SetTaskRunner(task_runner());
    hit_test_target->recognizers_.emplace_back(std::move(multi_tap_recognizer));
    AddHitTestTarget(hit_test_target.get());
  };

  auto reset_all = [&]() {
    ClearHitTestTarget();
    empty_target.reset();
    hit_test_target.reset();
    multi_tap_recognizer.reset();
  };

  task_runner()->PostTask([&, this]() {
    init_all();

    GESTURE_LOG << "Test second tap drift too much.";

    auto pointers =
        CreatePointer(ID(), PointerEvent::EventType::kDownEvent, {1000, 0});
    auto& pointer = pointers[0];

    gesture_manager()->HandlePointerEvents(root(), pointers);
    UpPointer(pointer);
    gesture_manager()->HandlePointerEvents(root(), pointers);

    auto pointers2 =
        CreatePointer(ID(), PointerEvent::EventType::kDownEvent, {1000, 0});
    auto& pointer2 = pointers2[0];
    gesture_manager()->HandlePointerEvents(root(), pointers2);
    MovePointer(pointer2, FloatSize(1000.f, 1000.f), kFastMoveTime);
    gesture_manager()->HandlePointerEvents(root(), pointers2);
    UpPointer(pointer2);
    gesture_manager()->HandlePointerEvents(root(), pointers2);
    EXPECT_EQ(tap_counts, 1);
    reset_all();
    latch.Signal();
  });
  latch.Wait();

  {
    GESTURE_LOG << "Test normal case.";
    latch.Reset();

    task_runner()->PostTask([&]() {
      init_all();
      auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
      auto& pointer = pointers[0];

      gesture_manager()->HandlePointerEvents(root(), pointers);
      // Although tap recognizer has already won, tap shouldn't callback until
      // up.
      EXPECT_EQ(tap_counts, 0);

      UpPointer(pointer);
      gesture_manager()->HandlePointerEvents(root(), pointers);
      EXPECT_EQ(tap_counts, 1);

      auto pointers2 = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
      auto& pointer2 = pointers2[0];
      gesture_manager()->HandlePointerEvents(root(), pointers2);
      EXPECT_EQ(tap_counts, 1);
      MovePointer(pointer2, FloatSize(1.f, 1.f), kFastMoveTime);
      gesture_manager()->HandlePointerEvents(root(), pointers2);
      UpPointer(pointer2);
      gesture_manager()->HandlePointerEvents(root(), pointers2);
      EXPECT_EQ(tap_counts, 2);
      reset_all();
      latch.Signal();
    });
    latch.Wait();
  }

  // Test drift too much.
  {
    GESTURE_LOG << "Test drift too much.";
    latch.Reset();
    task_runner()->PostTask([&]() {
      init_all();
      auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
      auto& pointer = pointers[0];

      gesture_manager()->HandlePointerEvents(root(), pointers);
      // Although tap recognizer has already won, tap shouldn't callback until
      // up.
      EXPECT_EQ(tap_counts, 0);

      UpPointer(pointer);
      gesture_manager()->HandlePointerEvents(root(), pointers);
      EXPECT_EQ(tap_counts, 1);

      auto pointers2 = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
      auto& pointer2 = pointers2[0];
      // Second tap is too far from the first tap.
      MovePointer(pointer2, FloatSize(1000.f, 1000.f), kFastMoveTime);
      gesture_manager()->HandlePointerEvents(root(), pointers2);
      UpPointer(pointer2);
      gesture_manager()->HandlePointerEvents(root(), pointers2);
      EXPECT_EQ(tap_counts, 1);

      // Although pointer 2 is abandoned, pointer 3 still can be accepted.
      auto pointers3 = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
      auto& pointer3 = pointers3[0];
      gesture_manager()->HandlePointerEvents(root(), pointers3);
      UpPointer(pointer3);
      gesture_manager()->HandlePointerEvents(root(), pointers3);
      EXPECT_EQ(tap_counts, 2);
      reset_all();
      latch.Signal();
    });
    latch.Wait();
  }

  {
    GESTURE_LOG << "Test timeout.";
    latch.Reset();
    auto pointers = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer = pointers[0];
    auto pointers2 = CreatePointer(ID(), PointerEvent::EventType::kDownEvent);
    auto& pointer2 = pointers2[0];

    task_runner()->PostTask([&]() {
      init_all();
      EXPECT_EQ(tap_counts, 0);
      gesture_manager()->HandlePointerEvents(root(), pointers);
      UpPointer(pointer);
      gesture_manager()->HandlePointerEvents(root(), pointers);
      EXPECT_EQ(tap_counts, 1);
      task_runner()->PostDelayedTask(
          [&]() {
            gesture_manager()->HandlePointerEvents(root(), pointers2);
            MovePointer(pointer2, {}, 1000);
            gesture_manager()->HandlePointerEvents(root(), pointers2);
            UpPointer(pointer2);
            gesture_manager()->HandlePointerEvents(root(), pointers2);
            reset_all();
            latch.Signal();
          },
          fml::TimeDelta::FromMilliseconds(1000));
    });
    latch.Wait();
    EXPECT_EQ(tap_counts, 1);
  }
}

// TODO(yulitao): Test multi pointer in one sequence.

}  // namespace testing

}  // namespace clay
