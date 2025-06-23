// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_BASE_GESTURE_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_BASE_GESTURE_HANDLER_H_

#include <arkui/ui_input_event.h>

#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class GestureDetector;
struct GestureCallback;

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

// Other constants
constexpr int MIN_SCROLL = std::numeric_limits<int>::min();
constexpr int MAX_SCROLL = std::numeric_limits<int>::max();
constexpr int FLING_SPEED_THRESHOLD = 300;

}  // namespace GestureConstants

namespace harmony {
class LynxContext;
class GestureArenaMember;
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
  BaseGestureHandler(int sign, LynxContext* lynx_context,
                     std::shared_ptr<GestureDetector> gesture_detector,
                     std::weak_ptr<GestureArenaMember> gesture_arena_member);

  virtual ~BaseGestureHandler() = default;

  static GestureHandlerMap ConvertToGestureHandler(
      int sign, LynxContext* lynx_context,
      std::weak_ptr<GestureArenaMember> member,
      const GestureMap& gesture_detectors);

  void HandleEnableGestureCallback(
      const std::vector<GestureCallback>& callbacks);
  void HandleMotionEvent(const ArkUI_UIInputEvent* input_event,
                         std::shared_ptr<TouchEvent> lynx_touch_event,
                         float delta_x, float delta_y);

  bool IsEnd() const;
  bool IsActive() const;
  int GetGestureStatus() const;

  void SendGestureEvent(const std::string& event_name,
                        const lepus::Value& event_params);

  bool IsOnBeginEnable() const;
  bool IsOnUpdateEnable() const;
  bool IsOnStartEnable() const;
  bool IsOnEndEnable() const;

  const lepus::Value GetEventParamsFromTouchEvent(
      std::shared_ptr<TouchEvent> touch_event) const;

  int Px2vp(float px_value) const;
  int Dip2Px(float dp_value) const;

  virtual void Activate();
  virtual void Reset();
  virtual void Fail();
  virtual void Begin();
  virtual void Ignore();
  virtual void End();

  void OnTouchesDown(std::shared_ptr<TouchEvent> touch_event);
  void OnTouchesMove(std::shared_ptr<TouchEvent> touch_event);
  void OnTouchesUp(std::shared_ptr<TouchEvent> touch_event);
  void OnTouchesCancel(std::shared_ptr<TouchEvent> touch_event);

  std::shared_ptr<GestureDetector> GetGestureDetector() const;

 protected:
  int sign_;
  int status_;

  LynxContext* lynx_context_;
  std::unordered_map<std::string, bool> enable_flags_;
  std::shared_ptr<GestureDetector> gesture_detector_;
  std::weak_ptr<GestureArenaMember> gesture_arena_member_;

  virtual void OnHandle(const ArkUI_UIInputEvent* event,
                        std::shared_ptr<TouchEvent> lynx_touch_event,
                        float fling_delta_x, float fling_delta_y) = 0;

  virtual void OnBegin(float x, float y, std::shared_ptr<TouchEvent> event) = 0;
  virtual void OnUpdate(float delta_x, float deltaY,
                        std::shared_ptr<TouchEvent> event) = 0;
  virtual void OnStart(float x, float y, std::shared_ptr<TouchEvent> event) = 0;
  virtual void OnEnd(float x, float y, std::shared_ptr<TouchEvent> event) = 0;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_BASE_GESTURE_HANDLER_H_
