// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef PLATFORM_LINUX_GTK_GTK_POINTER_BRIDGE_H_
#define PLATFORM_LINUX_GTK_GTK_POINTER_BRIDGE_H_

#include <gtk/gtk.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <vector>

#include "platform/embedder/public/capi/lynx_types.h"

namespace lynx::gtk {
// GTK performs coordinate translation; Clay owns recognition of touch gestures.
class PointerBridge {
 public:
  using Send = std::function<void(lynx_pointer_event_t*)>;
  PointerBridge(GtkWidget* widget, std::function<double()> scale, Send send)
      : widget_(widget), scale_(std::move(scale)), send_(std::move(send)) {
    auto* motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "enter",
                     G_CALLBACK(+[](GtkEventControllerMotion*, double x,
                                    double y, gpointer p) {
                       auto* self = static_cast<PointerBridge*>(p);
                       self->Position(x, y);
                       self->AddMouse();
                     }),
                     this);
    g_signal_connect(motion, "motion",
                     G_CALLBACK(+[](GtkEventControllerMotion* c, double x,
                                    double y, gpointer p) {
                       auto* self = static_cast<PointerBridge*>(p);
                       auto* e = gtk_event_controller_get_current_event(
                           GTK_EVENT_CONTROLLER(c));
                       if (e && gdk_event_get_pointer_emulated(e)) return;
                       self->Position(x, y);
                       self->AddMouse();
                       self->Mouse(self->buttons_ ? kLynxPointerPhaseMove
                                                  : kLynxPointerPhaseHover);
                     }),
                     this);
    g_signal_connect(motion, "leave",
                     G_CALLBACK(+[](GtkEventControllerMotion*, gpointer p) {
                       auto* self = static_cast<PointerBridge*>(p);
                       // Keep a dragged pointer alive until release; GTK owns
                       // the implicit grab.
                       if (!self->buttons_) self->RemoveMouse();
                     }),
                     this);
    Add(motion);
    auto* scroll =
        gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
    g_signal_connect(
        scroll, "scroll",
        G_CALLBACK(+[](GtkEventControllerScroll* c, double dx, double dy,
                       gpointer p) -> gboolean {
          auto* self = static_cast<PointerBridge*>(p);
          self->AddMouse();
          auto e = self->MouseEvent(self->buttons_ ? kLynxPointerPhaseMove
                                                   : kLynxPointerPhaseHover);
          e.signal_kind = kLynxPointerSignalKindScroll;
          auto* native =
              gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(c));
          bool wheel = !native || gdk_scroll_event_get_direction(native) !=
                                      GDK_SCROLL_SMOOTH;
#if GDK_VERSION_MIN_REQUIRED >= GDK_VERSION_4_8
          if (native)
            wheel = gdk_scroll_event_get_unit(native) == GDK_SCROLL_UNIT_WHEEL;
#endif
          e.is_precise_scroll = !wheel;
          e.scroll_delta_x = dx * (wheel ? 40 : 1) * self->scale_();
          e.scroll_delta_y = dy * (wheel ? 40 : 1) * self->scale_();
          self->send_(&e);
          return true;
        }),
        this);
    Add(scroll);
    // GtkGestureClick tracks a single button; raw buttons preserve chords. Raw
    // touch preserves IDs/cancel without a recognizer competing with Clay.
    // Motion and scrolling use dedicated controllers.
    auto* touch = gtk_event_controller_legacy_new();
    g_signal_connect(touch, "event",
                     G_CALLBACK(+[](GtkEventControllerLegacy*, GdkEvent* e,
                                    gpointer p) -> gboolean {
                       return static_cast<PointerBridge*>(p)->Touch(e);
                     }),
                     this);
    Add(touch);
    auto* focus = gtk_event_controller_focus_new();
    g_signal_connect(focus, "leave",
                     G_CALLBACK(+[](GtkEventControllerFocus*, gpointer p) {
                       static_cast<PointerBridge*>(p)->Cancel();
                     }),
                     this);
    Add(focus);
    g_signal_connect(widget_, "unmap", G_CALLBACK(+[](GtkWidget*, gpointer p) {
                       static_cast<PointerBridge*>(p)->Cancel();
                     }),
                     this);
  }
  ~PointerBridge() {
    Cancel();
    g_signal_handlers_disconnect_by_data(widget_, this);
    for (auto* c : controllers_) {
      g_signal_handlers_disconnect_by_data(c, this);
      gtk_widget_remove_controller(widget_, c);
    }
  }
  void Cancel() {
    PanZoom(GDK_TOUCHPAD_GESTURE_PHASE_CANCEL, 0, 0, 0, 0, 1, 0);
    if (buttons_) {
      Mouse(kLynxPointerPhaseCancel);
      buttons_ = 0;
    }
    RemoveMouse();
    for (auto& item : touches_) {
      auto e = item.second;
      e.timestamp = g_get_monotonic_time();
      e.phase = kLynxPointerPhaseCancel;
      send_(&e);
      e.phase = kLynxPointerPhaseRemove;
      send_(&e);
    }
    touches_.clear();
  }
  // GDK pinch scale is absolute; pan and angle are deltas. Touchscreen events
  // never enter this path, so Clay remains their only gesture recognizer.
  void PanZoom(GdkTouchpadGesturePhase phase, double x, double y, double dx,
               double dy, double scale, double angle) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(dx) ||
        !std::isfinite(dy) || !std::isfinite(scale) || scale <= 0 ||
        !std::isfinite(angle))
      return;
    if (phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN) {
      if (pan_active_)
        PanZoom(GDK_TOUCHPAD_GESTURE_PHASE_CANCEL, 0, 0, 0, 0, 1, 0);
      pan_event_ = {};
      pan_event_.struct_size = sizeof(pan_event_);
      pan_event_.device = -1;
      pan_event_.device_kind = kLynxPointerDeviceKindTrackpad;
      pan_event_.scale = 1;
      pan_x_ = pan_y_ = 0;
      pan_active_ = true;
      pan_event_.x = x * scale_();
      pan_event_.y = y * scale_();
      pan_event_.timestamp = g_get_monotonic_time();
      pan_event_.phase = kLynxPointerPhaseAdd;
      send_(&pan_event_);
      pan_event_.phase = kLynxPointerPhasePanZoomStart;
      send_(&pan_event_);
      return;
    }
    if (!pan_active_) return;
    pan_event_.timestamp = g_get_monotonic_time();
    if (phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE) {
      pan_x_ += dx;
      pan_y_ += dy;
      pan_event_.pan_x = pan_x_ * scale_();
      pan_event_.pan_y = pan_y_ * scale_();
      pan_event_.scale = scale;
      pan_event_.rotation += angle;
      pan_event_.phase = kLynxPointerPhasePanZoomUpdate;
      send_(&pan_event_);
      return;
    }
    pan_event_.phase = phase == GDK_TOUCHPAD_GESTURE_PHASE_CANCEL
                           ? kLynxPointerPhaseCancel
                           : kLynxPointerPhasePanZoomEnd;
    send_(&pan_event_);
    pan_event_.phase = kLynxPointerPhaseRemove;
    send_(&pan_event_);
    pan_active_ = false;
  }

 private:
  void Add(GtkEventController* c) {
    controllers_.push_back(c);
    gtk_widget_add_controller(widget_, c);
  }
  void Position(double x, double y) {
    x_ = x;
    y_ = y;
  }
  lynx_pointer_event_t MouseEvent(lynx_pointer_phase_e phase) {
    lynx_pointer_event_t e{};
    e.struct_size = sizeof(e);
    e.timestamp = g_get_monotonic_time();
    e.phase = phase;
    e.device_kind = kLynxPointerDeviceKindMouse;
    e.device = 0;
    e.x = x_ * scale_();
    e.y = y_ * scale_();
    e.buttons = buttons_;
    return e;
  }
  void Mouse(lynx_pointer_phase_e phase) {
    auto e = MouseEvent(phase);
    send_(&e);
  }
  void AddMouse() {
    if (!mouse_added_) {
      mouse_added_ = true;
      Mouse(kLynxPointerPhaseAdd);
    }
  }
  void RemoveMouse() {
    if (mouse_added_) {
      Mouse(kLynxPointerPhaseRemove);
      mouse_added_ = false;
    }
  }
  void Button(GdkEvent* native, double x, double y, bool down) {
    if (gdk_event_get_event_sequence(native) ||
        gdk_event_get_pointer_emulated(native))
      return;
    auto b = gdk_button_event_get_button(native);
    if (!b || b > 63) return;
    int64_t bit = b == 1 ? 1 : b == 2 ? 4 : b == 3 ? 2 : int64_t{1} << (b - 1);
    Position(x, y);
    AddMouse();
    auto previous = buttons_;
    buttons_ = down ? buttons_ | bit : buttons_ & ~bit;
    if (previous == buttons_) return;
    if (down) gtk_widget_grab_focus(widget_);
    Mouse(!previous   ? kLynxPointerPhaseDown
          : !buttons_ ? kLynxPointerPhaseUp
                      : kLynxPointerPhaseMove);
    if (!buttons_ && (x < 0 || y < 0 || x >= gtk_widget_get_width(widget_) ||
                      y >= gtk_widget_get_height(widget_)))
      RemoveMouse();
  }
  bool LocalPosition(GdkEvent* event, double& x, double& y) {
    if (!gdk_event_get_position(event, &x, &y)) return false;
    auto* root = gtk_widget_get_native(widget_);
    if (!root) return false;
    double tx = 0, ty = 0;
    gtk_native_get_surface_transform(root, &tx, &ty);
    graphene_point_t from{float(x - tx), float(y - ty)}, to{};
    if (!gtk_widget_compute_point(GTK_WIDGET(root), widget_, &from, &to))
      return false;
    x = to.x;
    y = to.y;
    return true;
  }
  bool Touch(GdkEvent* native) {
    auto type = gdk_event_get_event_type(native);
    if (type == GDK_TOUCHPAD_PINCH || type == GDK_TOUCHPAD_SWIPE) {
      double x = 0, y = 0, dx = 0, dy = 0;
      LocalPosition(native, x, y);
      gdk_touchpad_event_get_deltas(native, &dx, &dy);
      PanZoom(gdk_touchpad_event_get_gesture_phase(native), x, y, dx, dy,
              type == GDK_TOUCHPAD_PINCH
                  ? gdk_touchpad_event_get_pinch_scale(native)
                  : 1,
              type == GDK_TOUCHPAD_PINCH
                  ? gdk_touchpad_event_get_pinch_angle_delta(native)
                  : 0);
      return true;
    }
    if (type == GDK_BUTTON_PRESS || type == GDK_BUTTON_RELEASE) {
      double x, y;
      if (LocalPosition(native, x, y))
        Button(native, x, y, type == GDK_BUTTON_PRESS);
      return true;
    }
    if (type != GDK_TOUCH_BEGIN && type != GDK_TOUCH_UPDATE &&
        type != GDK_TOUCH_END && type != GDK_TOUCH_CANCEL)
      return false;
    auto* sequence = gdk_event_get_event_sequence(native);
    auto it = touches_.find(sequence);
    if (type == GDK_TOUCH_BEGIN) {
      if (it != touches_.end()) return true;
      lynx_pointer_event_t e{};
      e.struct_size = sizeof(e);
      // Reuse a free positive ID; unbounded input must not overflow a counter.
      e.device = 1;
      while (std::any_of(
          touches_.begin(), touches_.end(),
          [&e](const auto& entry) { return entry.second.device == e.device; }))
        ++e.device;
      e.device_kind = kLynxPointerDeviceKindTouch;
      it = touches_.emplace(sequence, e).first;
      gtk_widget_grab_focus(widget_);
    } else if (it == touches_.end())
      return true;
    auto& e = it->second;
    double x, y;
    if (LocalPosition(native, x, y)) {
      e.x = x * scale_();
      e.y = y * scale_();
    }
    e.timestamp = g_get_monotonic_time();
    if (type == GDK_TOUCH_BEGIN) {
      e.phase = kLynxPointerPhaseAdd;
      send_(&e);
    }
    e.phase = type == GDK_TOUCH_BEGIN    ? kLynxPointerPhaseDown
              : type == GDK_TOUCH_UPDATE ? kLynxPointerPhaseMove
              : type == GDK_TOUCH_END    ? kLynxPointerPhaseUp
                                         : kLynxPointerPhaseCancel;
    send_(&e);
    if (type == GDK_TOUCH_END || type == GDK_TOUCH_CANCEL) {
      e.phase = kLynxPointerPhaseRemove;
      send_(&e);
      touches_.erase(it);
    }
    return true;
  }
  GtkWidget* widget_;
  std::function<double()> scale_;
  Send send_;
  std::vector<GtkEventController*> controllers_;
  std::map<GdkEventSequence*, lynx_pointer_event_t> touches_;
  int64_t buttons_ = 0;
  double x_ = 0, y_ = 0;
  bool mouse_added_ = false;
  bool pan_active_ = false;
  double pan_x_ = 0, pan_y_ = 0;
  lynx_pointer_event_t pan_event_{};
};
}  // namespace lynx::gtk
#endif
