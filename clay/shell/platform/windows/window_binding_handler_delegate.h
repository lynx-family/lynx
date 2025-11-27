// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_BINDING_HANDLER_DELEGATE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_BINDING_HANDLER_DELEGATE_H_

#include <functional>

#include "clay/public/clay.h"

namespace clay {

class WindowBindingHandlerDelegate {
 public:
  using KeyEventCallback = std::function<void(bool)>;

  // Notifies delegate that backing window size has changed.
  // Typically called by currently configured WindowBindingHandler, this is
  // called on the platform thread.
  virtual void OnWindowSizeChanged(size_t width, size_t height) = 0;

  // Notifies delegate that backing window needs to be repainted.
  // Typically called by currently configured WindowBindingHandler.
  virtual void OnWindowRepaint() = 0;

  // Notifies delegate that backing window mouse has moved.
  // Typically called by currently configured WindowBindingHandler.
  virtual void OnPointerMove(double x, double y,
                             ClayPointerDeviceKind device_kind,
                             int32_t device_id, int modifiers_state) = 0;

  // Notifies delegate that backing window mouse pointer button has been
  // pressed. Typically called by currently configured WindowBindingHandler.
  virtual void OnPointerDown(double x, double y,
                             ClayPointerDeviceKind device_kind,
                             int32_t device_id,
                             ClayPointerMouseButtons button) = 0;

  // Notifies delegate that backing window mouse pointer button has been
  // released. Typically called by currently configured WindowBindingHandler.
  virtual void OnPointerUp(double x, double y,
                           ClayPointerDeviceKind device_kind, int32_t device_id,
                           ClayPointerMouseButtons button) = 0;

  // Notifies delegate that backing window mouse pointer has left the window.
  // Typically called by currently configured WindowBindingHandler.
  virtual void OnPointerLeave(double x, double y,
                              ClayPointerDeviceKind device_kind,
                              int32_t device_id) = 0;

  // Notifies delegate that a pan/zoom gesture has started.
  // Typically called by DirectManipulationEventHandler.
  virtual void OnPointerPanZoomStart(int32_t device_id) = 0;

  // Notifies delegate that a pan/zoom gesture has updated.
  // Typically called by DirectManipulationEventHandler.
  virtual void OnPointerPanZoomUpdate(int32_t device_id, double pan_x,
                                      double pan_y, double scale,
                                      double rotation) = 0;

  // Notifies delegate that a pan/zoom gesture has ended.
  // Typically called by DirectManipulationEventHandler.
  virtual void OnPointerPanZoomEnd(int32_t device_id) = 0;

  // Notifies delegate that backing window has received text.
  // Typically called by currently configured WindowBindingHandler.
  virtual void OnText(const std::u16string&) = 0;

  // Notifies delegate that backing window size has received key press. Should
  // return true if the event was handled and should not be propagated.
  // Typically called by currently configured WindowBindingHandler.
  virtual void OnKey(int key, int scancode, int action, char32_t character,
                     bool extended, bool was_down,
                     KeyEventCallback callback) = 0;

  // Notifies the delegate that IME composing mode has begun.
  //
  // Triggered when the user begins editing composing text using a multi-step
  // input method such as in CJK text input.
  virtual void OnComposeBegin() = 0;

  // Notifies the delegate that IME composing region have been committed.
  //
  // Triggered when the user commits the current composing text while using a
  // multi-step input method such as in CJK text input. Composing continues with
  // the next keypress.
  virtual void OnComposeCommit() = 0;

  // Notifies the delegate that IME composing mode has ended.
  //
  // Triggered when the user commits the composing text while using a multi-step
  // input method such as in CJK text input.
  virtual void OnComposeEnd() = 0;

  // Notifies the delegate that IME composing region contents have changed.
  //
  // Triggered when the user edits the composing text while using a multi-step
  // input method such as in CJK text input.
  virtual void OnComposeChange(const std::u16string& text, int cursor_pos) = 0;

  // Notifies delegate that backing window size has recevied scroll.
  // Typically called by currently configured WindowBindingHandler.
  virtual void OnScroll(double x, double y, double delta_x, double delta_y,
                        int scroll_offset_multiplier,
                        ClayPointerDeviceKind device_kind,
                        int32_t device_id) = 0;

  // Notifies delegate that scroll inertia should be cancelled.
  // Typically called by DirectManipulationEventHandler
  virtual void OnScrollInertiaCancel(int32_t device_id) = 0;

  // Update the status of the high contrast feature
  virtual void UpdateHighContrastEnabled(bool enabled) = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_BINDING_HANDLER_DELEGATE_H_
