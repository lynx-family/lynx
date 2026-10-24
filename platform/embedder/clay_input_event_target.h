// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_CLAY_INPUT_EVENT_TARGET_H_
#define PLATFORM_EMBEDDER_CLAY_INPUT_EVENT_TARGET_H_

#include <array>
#include <cstddef>
#include <functional>

#include "clay/public/clay.h"
#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace embedder {

struct ClayInputViewport {
  float width = 0;
  float height = 0;
  float pixel_ratio = 0;
  bool IsValid() const;
};

// UI-sequence-only input for one LynxView. Coordinates are logical pixels
// relative to the top-left of the view, independent of screencast mode.
class ClayInputEventTarget final : public devtool::input::InputEventTarget {
 public:
  using ViewportCallback = std::function<ClayInputViewport()>;
  using SendCallback = std::function<void(const ClayPointerEvent&)>;

  ClayInputEventTarget(ViewportCallback viewport, SendCallback send);
  ~ClayInputEventTarget() override;

  ClayInputEventTarget(const ClayInputEventTarget&) = delete;
  ClayInputEventTarget& operator=(const ClayInputEventTarget&) = delete;

  devtool::input::PointerCapabilities GetPointerCapabilities() const override;
  bool InjectPointerEvent(const devtool::input::PointerEvent& event) override;

  // Call before resetting the page or detaching the renderer.
  void CancelActivePointers();
  // Call before destroying the renderer captured by the callbacks.
  void Invalidate();

 private:
  static constexpr std::size_t kMaxTouchPoints = 2;

  struct TouchPointerState {
    int32_t pointer_id = 0;
    int32_t device_id = 0;
    ClayPointerEvent last_event = {};
    bool active = false;
  };

  bool InjectMouseEvent(const devtool::input::PointerEvent& event,
                        const ClayInputViewport& viewport);
  bool InjectTouchEvent(const devtool::input::PointerEvent& event,
                        const ClayInputViewport& viewport);
  ClayPointerEvent MakeClayEvent(const devtool::input::Pointer& pointer,
                                 const ClayInputViewport& viewport,
                                 ClayPointerDeviceKind device_kind,
                                 int32_t device_id, int64_t timestamp_us) const;
  TouchPointerState* FindTouchPointer(int32_t pointer_id);
  const TouchPointerState* FindTouchPointer(int32_t pointer_id) const;
  std::size_t ActiveTouchCount() const;
  bool HasSameActiveTouchSet(const devtool::input::PointerEvent& event) const;
  void CancelMouse();
  void RemoveMouse();
  void CancelTouches();
  void RemoveTouch(TouchPointerState& pointer);
  void CancelAll();

  ViewportCallback viewport_;
  SendCallback send_;
  ClayInputViewport active_viewport_;
  ClayPointerEvent last_mouse_event_ = {};
  int32_t mouse_pointer_id_ = 0;
  bool mouse_active_ = false;
  bool mouse_added_ = false;
  std::array<TouchPointerState, kMaxTouchPoints> touch_pointers_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CLAY_INPUT_EVENT_TARGET_H_
