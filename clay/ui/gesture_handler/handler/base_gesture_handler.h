// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_HANDLER_BASE_GESTURE_HANDLER_H_
#define CLAY_UI_GESTURE_HANDLER_HANDLER_BASE_GESTURE_HANDLER_H_

#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/public/value.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/gesture_detector.h"

namespace clay {

class GestureDetector;
class PageView;

namespace GestureConstants {
// Gesture event names
constexpr const char ON_TOUCHES_DOWN[] = "onTouchesDown";
constexpr const char ON_TOUCHES_MOVE[] = "onTouchesMove";
constexpr const char ON_TOUCHES_UP[] = "onTouchesUp";
constexpr const char* ON_TOUCHES_CANCEL = "onTouchesCancel";
constexpr const char* ON_BEGIN = "onBegin";
constexpr const char* ON_UPDATE = "onUpdate";
constexpr const char* ON_END = "onEnd";
constexpr const char* ON_START = "onStart";

// Gesture Configs
constexpr const char* MIN_DURATION = "minDuration";
constexpr const char* MAX_DURATION = "maxDuration";
constexpr const char* MIN_DISTANCE = "minDistance";
constexpr const char* MAX_DISTANCE = "maxDistance";

// Gesture Status
constexpr int LYNX_STATE_INIT = 0;
constexpr int LYNX_STATE_BEGIN = 1;
constexpr int LYNX_STATE_ACTIVE = 2;
constexpr int LYNX_STATE_FAIL = 3;
constexpr int LYNX_STATE_END = 4;
constexpr int LYNX_STATE_UNDETERMINED = 5;

// Scroll container direction
constexpr int DIRECTION_HORIZONTAL = -1;
constexpr int DIRECTION_VERTICAL = 1;
constexpr int DIRECTION_UNDETERMINED = 0;

// Other constants
constexpr int MIN_SCROLL = std::numeric_limits<int>::min();
constexpr int MAX_SCROLL = std::numeric_limits<int>::max();
constexpr int FLING_SPEED_THRESHOLD = 300;

}  // namespace GestureConstants

class LynxContext;
class GestureArenaMember;
class GestureExtraBundle;
class TouchEvent;

class BaseGestureHandler;

using GestureHandlerMap =
    std::unordered_map<uint32_t, std::shared_ptr<BaseGestureHandler>>;

/**
 * BaseGestureHandler is an abstract class that serves as the base for
 implementing gesture handlers.
 * It provides common functionality and defines abstract methods to handle
 gesture events.

 * Gesture handlers are responsible for processing touch events and generating
 corresponding
 * gesture events based on the gesture type and user interactions.

 * The class includes methods for converting gesture detectors to gesture
 handlers, handling enable
 * gesture callbacks, sending gesture events, and retrieving event parameters
 from touch events.

 * Subclasses of BaseGestureHandler must implement the abstract methods to
 handle specific types
 * of gestures such as PanGestureHandler, FlingGestureHandler... and define
 their behavior when the gesture begins, updates, and ends.
 *
 */

class BaseGestureHandler {
 public:
  BaseGestureHandler(int sign, PageView* page_view,
                     std::shared_ptr<GestureDetector> gesture_detector,
                     fml::WeakPtr<GestureArenaMember> gesture_arena_member);

  virtual ~BaseGestureHandler() = default;

  static GestureHandlerMap ConvertToGestureHandler(
      int sign, PageView* page_view, fml::WeakPtr<GestureArenaMember> member,
      const GestureMap& gesture_detectors);

  void HandleEnableGestureCallback(
      const std::vector<std::string>& callback_names);
  void HandleMotionEvent(
      const PointerEvent* pointer_event, float delta_x, float delta_y,
      bool handle_by_simultaneous,
      const std::shared_ptr<GestureExtraBundle>& extra_bundle);

  bool IsEnd() const;
  bool IsActive() const;
  int GetGestureStatus() const;

  void SendGestureEvent(const std::string& event_name,
                        const PointerEvent* pointer_event,
                        Value& additional_params);

  bool IsOnBeginEnable() const;
  bool IsOnUpdateEnable() const;
  bool IsOnStartEnable() const;
  bool IsOnEndEnable() const;

  virtual void Activate();
  virtual void Reset();
  virtual void Fail();
  virtual void Begin();
  virtual void Ignore();
  virtual void End();

  void OnTouchesDown(const PointerEvent* pointer_event);
  void OnTouchesMove(const PointerEvent* pointer_event);
  void OnTouchesUp(const PointerEvent* pointer_event);
  void OnTouchesCancel(const PointerEvent* pointer_event);

  std::shared_ptr<GestureDetector> GetGestureDetector() const;

 protected:
  int sign_;
  int status_;

  std::unordered_map<std::string, bool> enable_flags_;
  std::shared_ptr<GestureDetector> gesture_detector_;
  fml::WeakPtr<GestureArenaMember> gesture_arena_member_;
  PageView* page_view_;
  virtual void OnHandle(
      const PointerEvent* pointer_event, float fling_delta_x,
      float fling_delta_y, bool handle_by_simultaneous,
      const std::shared_ptr<GestureExtraBundle>& extra_bundle) = 0;

  virtual void OnBegin(float x, float y, const PointerEvent* event) = 0;
  virtual void OnUpdate(
      float delta_x, float deltaY, const PointerEvent* event,
      const std::shared_ptr<GestureExtraBundle>& extra_bundle) = 0;
  virtual void OnStart(float x, float y, const PointerEvent* event) = 0;
  virtual void OnEnd(float x, float y, const PointerEvent* event) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_BASE_GESTURE_HANDLER_H_
