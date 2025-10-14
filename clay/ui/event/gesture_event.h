// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_EVENT_GESTURE_EVENT_H_
#define CLAY_UI_EVENT_GESTURE_EVENT_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"

namespace clay {

/// Action code for when a primary pointer touched the screen.
///
/// Android's
/// [MotionEvent.ACTION_DOWN](https://developer.android.com/reference/android/view/MotionEvent#ACTION_DOWN)
static constexpr int kActionDown = 0;

/// Action code for when a primary pointer stopped touching the screen.
///
/// Android's
/// [MotionEvent.ACTION_UP](https://developer.android.com/reference/android/view/MotionEvent#ACTION_UP)
static constexpr int kActionUp = 1;

/// Action code for when the event only includes information about pointer
/// movement.
///
/// Android's
/// [MotionEvent.ACTION_MOVE](https://developer.android.com/reference/android/view/MotionEvent#ACTION_MOVE)
static constexpr int kActionMove = 2;

/// Action code for when a motion event has been canceled.
///
/// Android's
/// [MotionEvent.ACTION_CANCEL](https://developer.android.com/reference/android/view/MotionEvent#ACTION_CANCEL)
static constexpr int kActionCancel = 3;

/// Action code for when a secondary pointer touched the screen.
///
/// Android's
/// [MotionEvent.ACTION_POINTER_DOWN](https://developer.android.com/reference/android/view/MotionEvent#ACTION_POINTER_DOWN)
static constexpr int kActionPointerDown = 5;

/// Action code for when a secondary pointer stopped touching the screen.
///
/// Android's
/// [MotionEvent.ACTION_POINTER_UP](https://developer.android.com/reference/android/view/MotionEvent#ACTION_POINTER_UP)
static constexpr int kActionPointerUp = 6;

/// Android's
/// [InputDevice.SOURCE_UNKNOWN](https://developer.android.com/reference/android/view/InputDevice#SOURCE_UNKNOWN)
static const int kInputDeviceSourceUnknown = 0;

/// Android's
/// [InputDevice.SOURCE_TOUCHSCREEN](https://developer.android.com/reference/android/view/InputDevice#SOURCE_TOUCHSCREEN)
static const int kInputDeviceSourceTouchScreen = 4098;

/// Android's
/// [InputDevice.SOURCE_MOUSE](https://developer.android.com/reference/android/view/InputDevice#SOURCE_MOUSE)
static const int kInputDeviceSourceMouse = 8194;

/**
 * Base class for touch, stylus, or mouse events.
 *
 * @note It contains the union set of members in the derived class. Therefore, a
 * value of a member variable is not necessarily meaningful for all derived
 * class. Using the getters in the derived classes is high recommended.
 *
 * Pointer events operate in the coordinate space of the screen, scaled to
 * logical pixels. Logical pixels approximate a grid with about 38 pixels per
 * centimeter, or 96 pixels per inch.
 *
 * This allows gestures to be recognized independent of the precise hardware
 * characteristics of the device. In particular, features such as touch slop
 * (see [kTouchSlop]) can be defined in terms of roughly physical lengths so
 * that the user can shift their finger by the same distance on a high-density
 * display as on a low-resolution device.
 *
 * For similar reasons, pointer events are not affected by any transforms in
 * the rendering layer. This means that deltas may need to be scaled before
 * being applied to movement within the rendering. For example, if a scrolling
 * list is shown scaled by 2x, the pointer deltas will have to be scaled by the
 * inverse amount if the list is to appear to scroll with the user's finger.
 */
struct PointerEvent {
  enum class EventType {
    kUnkownEvent = 0,
    kDownEvent,
    kUpEvent,
    kMoveEvent,
    kHoverEvent,
    kSignalEvent,
    kCancel,
    kPanZoomStartEvent,
    kPanZoomUpdateEvent,
    kPanZoomEndEvent,
  };

  enum DeviceType {
    kTouch,
    kMouse,
    kStylus,
    kInvertedStylus,
    kTrackpad,
  };

  enum class SignalKind {
    kNone,
    kStartScroll,
    kScroll,
    kEndScroll,
  };

  // used in bit field buttons to show which buttons are pressed
  // MouseBotton should keep the same as FlutterPointerMouseButtons
  enum MouseButton {
    kPrimary = 1 << 0,
    kSecondary = 1 << 1,
    kMiddle = 1 << 2,
    kBack = 1 << 3,
    kForward = 1 << 4,
  };

  PointerEvent() = default;

  explicit PointerEvent(PointerEvent::EventType event_type)
      : type(event_type) {}

  std::string ToString() const;

  EventType type = EventType::kUnkownEvent;
  DeviceType device = kTouch;

  /**
   * Unique identifier that ties the `PointerEvent` to the embedder event that
   * created it.
   *
   * No two pointer events can have the same `embedder_id` on platforms that set
   * it. This is different from `pointer_id` identifier - used for hit-testing,
   * whereas `embedder_id` is used to identify the platform event.
   *
   * On Android this is ID of the underlying
   * [MotionEvent](https://developer.android.com/reference/android/view/MotionEvent).
   */
  int embedder_id = 0;
  /**
   * Time of event dispatch, relative to an arbitrary timeline.
   */
  uint64_t timestamp = 0;
  /**
   * Unique identifier for the pointer, not reused. Changes for each new pointer
   * down event.
   */
  int pointer_id = 0;
  /**
   * Unique identifier for the pointing device, reused across interactions.
   */
  int device_id = 0;
  /**
   * Coordinate of the position of the pointer, in logical pixels in the global
   * coordinate space.
   */
  FloatPoint position;
  /**
   * Distance in logical pixels that the pointer moved since the last
   * `PointerMoveEvent` or `PointerHoverEvent`.
   *
   * This value is always 0.0 for down, up, and cancel events.
   */
  FloatSize delta;
  /**
   * Set if the pointer is currently down.
   *
   * For touch and stylus pointers, this means the object (finger, pen) is in
   * contact with the input surface. For mice, it means a button is pressed.
   */
  bool down = false;

  int64_t buttons = 0;  // bit field to indicate which buttons are pressed

  /**
   * The pressure of the touch.
   *
   * This value is a number ranging from 0.0, indicating a touch with no
   * discernible pressure, to 1.0, indicating a touch with "normal" pressure,
   * and possibly beyond, indicating a stronger touch. For devices that do not
   * detect pressure (e.g. mice), returns 1.0.
   */
  double pressure = 1.0;
  /**
   *  The minimum value that `pressure` can return for this pointer.
   *
   *  For devices that do not detect pressure (e.g. mice), returns 1.0.
   *  This will always be a number less than or equal to 1.0.
   */
  double pressure_min = 1.0;
  /**
   * The maximum value that `pressure` can return for this pointer.
   *
   * For devices that do not detect pressure (e.g. mice), returns 1.0.
   * This will always be a greater than or equal to 1.0.
   */
  double pressure_max = 1.0;
  /**
   * The distance of the detected object from the input surface.
   *
   * For instance, this value could be the distance of a stylus or finger
   * from a touch screen, in arbitrary units on an arbitrary (not necessarily
   * linear) scale. If the pointer is down, this is 0.0 by definition.
   */
  double distance = 0.0;
  /**
   * The maximum value that `distance` can return for this pointer.
   *
   * If this input device cannot detect "hover touch" input events, then this
   * will be 0.0.
   */
  double distance_max = 0.0;
  /**
   * The area of the screen being pressed.
   *
   * This value is scaled to a range between 0 and 1. It can be used to
   * determine fat touch events. This value is only set on Android and is
   * a device specific approximation within the range of detectable values.
   * So, for example, the value of 0.1 could mean a touch with the tip of
   * the finger, 0.2 a touch with full finger, and 0.3 the full palm.
   *
   * Because this value uses device-specific range and is uncalibrated,
   * it is of limited use and is primarily retained in order to be able
   * to reconstruct original pointer events for [AndroidView].
   */
  double size = 0.0;
  /**
   * The radius of the contact ellipse along the major axis, in logical pixels.
   */
  double radius_major = 0.0;
  /**
   * The radius of the contact ellipse along the minor axis, in logical pixels.
   */
  double radius_minor = 0.0;
  /**
   * The minimum value that could be reported for `radius_major` and
   * `radius_minor` for this pointer, in logical pixels.
   */
  double radius_min = 0.0;
  /**
   * The maximum value that could be reported for `radiusMajor` and
   * `radiusMinor` for this pointer, in logical pixels.
   */
  double radius_max = 0.0;
  /**
   * The orientation angle of the detected object, in radians.
   *
   * For [PointerDeviceKind.touch] events:
   *
   * The angle of the contact ellipse, in radians in the range:
   *
   *     -pi/2 < orientation <= pi/2
   *
   * ...giving the angle of the major axis of the ellipse with the y-axis
   * (negative angles indicating an orientation along the top-left /
   * bottom-right diagonal, positive angles indicating an orientation along the
   * top-right / bottom-left diagonal, and zero indicating an orientation
   * parallel with the y-axis).
   *
   * For [PointerDeviceKind.stylus] and [PointerDeviceKind.invertedStylus]
   * events:
   *
   * The angle of the stylus, in radians in the range:
   *
   *     -pi < orientation <= pi
   *
   * ...giving the angle of the axis of the stylus projected onto the input
   * surface, relative to the positive y-axis of that surface (thus 0.0
   * indicates the stylus, if projected onto that surface, would go from the
   * contact point vertically up in the positive y-axis direction, pi would
   * indicate that the stylus would go down in the negative y-axis direction;
   * pi/4 would indicate that the stylus goes up and to the right, -pi/2 would
   * indicate that the stylus goes to the left, etc).
   */
  double orientation = 0.0;
  /**
   * The tilt angle of the detected object, in radians.
   *
   * For [PointerDeviceKind.stylus] and [PointerDeviceKind.invertedStylus]
   * events:
   *
   * The angle of the stylus, in radians in the range:
   *
   *     0 <= tilt <= pi/2
   *
   * ...giving the angle of the axis of the stylus, relative to the axis
   * perpendicular to the input surface (thus 0.0 indicates the stylus is
   * orthogonal to the plane of the input surface, while pi/2 indicates that
   * the stylus is flat on that surface).
   */
  double tilt = 0.0;
  /**
   * Opaque platform-specific data associated with the event.
   */
  int platform_data = 0;
  /**
   * Set if the event was synthesized by Flutter.
   *
   * We occasionally synthesize PointerEvents that aren't exact translations of
   * `PointerData` from the engine to cover small cross-OS discrepancies in
   * pointer behaviors.
   *
   * For instance, on end events, Android always drops any location changes
   * that happened between its reporting intervals when emitting the end events.
   *
   * On iOS, minor incorrect location changes from the previous move events can
   * be reported on end events. We synthesize a `PointerEvent` to cover the
   * difference between the 2 events in that case.
   */
  bool synthesized = false;

  SignalKind signal_kind = SignalKind::kNone;

  float scroll_delta_x = 0.0;
  float scroll_delta_y = 0.0;

  /// The total pan offset of the pan/zoom.
  FloatPoint pan = {0.0, 0.0};
  /// The amount the pan offset changed since the last event.
  FloatSize pan_delta = {0.0, 0.0};
  /// The scale (zoom factor) of the pan/zoom.
  double scale = 1.0;
  /// The amount the pan/zoom has rotated in radians so far.
  double rotation = 0.0;

  bool is_precise_scroll = true;

  double source = kInputDeviceSourceTouchScreen;
};

}  // namespace clay

#endif  // CLAY_UI_EVENT_GESTURE_EVENT_H_
