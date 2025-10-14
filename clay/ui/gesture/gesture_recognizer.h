// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_GESTURE_RECOGNIZER_H_
#define CLAY_UI_GESTURE_GESTURE_RECOGNIZER_H_

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_set>
#include <utility>

#include "base/include/fml/time/timer.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/arena_manager.h"
#include "clay/ui/gesture/arena_member.h"
#include "clay/ui/gesture/pointer_router.h"

namespace clay {

// From flutter. Indicate how much distance moving is needed before accept.
#if OS_ANDROID
constexpr float kTouchSlop = 8.f;
#elif OS_HARMONY
constexpr float kTouchSlop = 5.f;
#else
// touch slop's default value is 8 the same as Lynx.
constexpr float kTouchSlop = 8.f;
#endif

using OnMultiTapCallback =
    std::function<void(const PointerEvent& up_event, int tap_counts)>;
using OnTapUpCallback = std::function<void(const PointerEvent& up_event)>;
using OnTapDownCallback = std::function<void(const PointerEvent& down_event)>;
using OnTapCallback = std::function<void()>;
using OnTapCancelCallback = std::function<void()>;

class GestureManager;

enum class GestureRecognizerState {
  kReady,
  kPossible,
  // Not possible to accept, unless state back to kReady.
  kDefunct,
};

enum class GestureRecognizerType {
  kNone = 0,
  kTap,
  kDoubleTap,
  kLongPress,
  kDragGesture,
  kHorizontalDrag,
  kVerticalDrag
};

class GestureRecognizer : public ArenaMember {
 public:
  class Delegate {
   public:
    virtual bool IsPointerAllowed(const GestureRecognizer& gesture_recognizer,
                                  const PointerEvent& event) = 0;
  };

  explicit GestureRecognizer(GestureManager* gesture_manager)
      : gesture_manager_(gesture_manager) {
    FML_DCHECK(gesture_manager_);
  }
  virtual ~GestureRecognizer() = default;

  virtual GestureRecognizerType getType() const {
    return GestureRecognizerType::kNone;
  }

  // Called when pointer down.
  void AddPointer(const PointerEvent& pointer);
  GestureManager* gesture_manager() const { return gesture_manager_; }
  RouteId route_id() const { return route_id_; }
  const char* GetMemberTag() const override { return "[unknown_gesture]"; }
  void SetTaskRunner(const fml::RefPtr<fml::TaskRunner>& task_runner) {
    timer_runner_ = task_runner;
  }
  const fml::RefPtr<fml::TaskRunner>& timer_runner() { return timer_runner_; }

  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

 protected:
  void OnGestureAccepted(int pointer_id) override;
  void OnGestureRejected(int pointer_id) override {}

  // Called when a pointer down.
  virtual void AddAllowedPointer(const PointerEvent& pointer) {}

  GestureManager* gesture_manager_;
  RouteId route_id_ = GenerateRouteId();
  fml::RefPtr<fml::TaskRunner> timer_runner_;
  Delegate* delegate_{nullptr};
};

// All pointers are done in one sequence, that means if at one time point
// all pointers are up, the recognition will be end.
class OneSequenceGestureRecognizer : public GestureRecognizer {
 public:
  explicit OneSequenceGestureRecognizer(GestureManager* gesture_manager)
      : GestureRecognizer(gesture_manager) {}
  virtual ~OneSequenceGestureRecognizer();

  virtual void HandleEvent(const PointerEvent&) = 0;

 protected:
  void AddAllowedPointer(const PointerEvent& pointer) override {
    StartTrackingPointer(pointer.pointer_id);
  }

  void StartTrackingPointer(int pointer_id);
  void StopTrackingPointer(int pointer_id);
  virtual void DidStopTrackingLastPointer(int pointer_id) {}

  // Resolve and destroy related entries.
  virtual void ResolveAll(GestureDisposition disposition);
  virtual void ResolveOne(int pointer_id, GestureDisposition disposition);

  // Called when ResolveAll / ResolveOne.
  // As OnGestureAccepted / OnGestureRejected are called passively, and not
  // guaranteed whether will be called when resolve.
  // So provides this callbacks for listening.
  virtual void OnResolveAll(GestureDisposition disposition) {}
  virtual void OnResolveOne(int pointer_id, GestureDisposition disposition) {}

  std::set<int> tracking_pointers_;
  std::map<int, std::unique_ptr<ArenaEntry>> arena_entries_;
};

// Only handle one pointer.
class PrimaryPointerGestureRecognizer : public OneSequenceGestureRecognizer {
 public:
  explicit PrimaryPointerGestureRecognizer(GestureManager* gesture_manager,
                                           std::optional<uint64_t> timeout_ms)
      : super(gesture_manager), timeout_ms_(timeout_ms) {}

  void HandleEvent(const PointerEvent&) override;

  void SetDriftTolerance(float tolerance) { drift_tolerance_ = tolerance; }

 protected:
  void AddAllowedPointer(const PointerEvent& pointer) override;

  void DidStopTrackingLastPointer(int pointer_id) override;

  void OnGestureAccepted(int pointer_id) override;
  void OnGestureRejected(int pointer_id) override;

  virtual void HandlePrimaryPointerEvent(const PointerEvent& event) = 0;
  virtual void OnTimeout();

  int primary_pointer() const { return primary_pointer_; }

  GestureRecognizerState recognizer_state_ = GestureRecognizerState::kReady;

  void Reset();

 private:
  using super = OneSequenceGestureRecognizer;

  bool IsWithinDriftTolerance(const FloatPoint& position);

  // Tolerance of movement between down and up event.
  std::optional<float> drift_tolerance_;
  // If set, timer will start when touch down.
  // Note that if gesture accepted or rejected, timer will be canceled.
  std::optional<uint64_t> timeout_ms_;
  std::unique_ptr<fml::OneshotTimer> timer_;
  FloatPoint initial_position_;
  int primary_pointer_ = -1;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_GESTURE_RECOGNIZER_H_
