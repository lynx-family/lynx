// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef PLATFORM_LINUX_GTK_GTK_KEYBOARD_BRIDGE_H_
#define PLATFORM_LINUX_GTK_GTK_KEYBOARD_BRIDGE_H_
#include <gtk/gtk.h>

#include <functional>
#include <map>
#include <vector>

#include "clay/ui/event/key_codes.h"
#include "platform/embedder/public/capi/lynx_types.h"
#include "platform/linux/gtk/gtk_key_mapping.h"
namespace lynx::gtk {
class KeyboardBridge {
 public:
  using Send = std::function<void(lynx_key_event_t*)>;
  KeyboardBridge(GtkWidget* widget, Send send)
      : widget_(widget), send_(std::move(send)) {
    controller_ = gtk_event_controller_key_new();
    g_signal_connect(
        controller_, "modifiers",
        G_CALLBACK(+[](GtkEventControllerKey*, GdkModifierType state,
                       gpointer p) -> gboolean {
          static_cast<KeyboardBridge*>(p)->Modifiers(state);
          return false;
        }),
        this);
    g_signal_connect(
        controller_, "key-pressed",
        G_CALLBACK(+[](GtkEventControllerKey*, guint value, guint code,
                       GdkModifierType state, gpointer p) -> gboolean {
          static_cast<KeyboardBridge*>(p)->Key(value, code, state, true);
          return true;
        }),
        this);
    g_signal_connect(
        controller_, "key-released",
        G_CALLBACK(+[](GtkEventControllerKey*, guint value, guint code,
                       GdkModifierType state, gpointer p) {
          static_cast<KeyboardBridge*>(p)->Key(value, code, state, false);
        }),
        this);
    gtk_widget_add_controller(widget_, controller_);
    focus_ = gtk_event_controller_focus_new();
    g_signal_connect(focus_, "leave",
                     G_CALLBACK(+[](GtkEventControllerFocus*, gpointer p) {
                       static_cast<KeyboardBridge*>(p)->Release();
                     }),
                     this);
    gtk_widget_add_controller(widget_, focus_);
    g_signal_connect(widget_, "map",
                     G_CALLBACK(+[](GtkWidget* widget, gpointer) {
                       auto* root = gtk_widget_get_root(widget);
                       if (root && !gtk_root_get_focus(root))
                         gtk_widget_grab_focus(widget);
                     }),
                     this);
    g_signal_connect(widget_, "unmap", G_CALLBACK(+[](GtkWidget*, gpointer p) {
                       static_cast<KeyboardBridge*>(p)->Release();
                     }),
                     this);
    if (gtk_widget_get_mapped(widget_)) {
      auto* root = gtk_widget_get_root(widget_);
      if (root && !gtk_root_get_focus(root)) gtk_widget_grab_focus(widget_);
    }
  }
  ~KeyboardBridge() {
    Release();
    g_signal_handlers_disconnect_by_data(widget_, this);
    for (auto* c : {controller_, focus_}) {
      g_signal_handlers_disconnect_by_data(c, this);
      gtk_widget_remove_controller(widget_, c);
    }
  }
  void Release() {
    auto pressed = std::move(pressed_);
    pressed_.clear();
    for (auto& item : pressed) {
      auto e = Event(item.first, item.second, kLynxKeyEventTypeUp);
      e.synthesized = true;
      send_(&e);
    }
  }
  void SetIMContext(GtkIMContext* context) {
    gtk_event_controller_key_set_im_context(
        GTK_EVENT_CONTROLLER_KEY(controller_), context);
  }

 private:
  void Modifiers(GdkModifierType state) {
    if (!gtk_event_controller_key_get_im_context(
            GTK_EVENT_CONTROLLER_KEY(controller_)))
      return;
    // An IME can consume the native modifier key-up during preedit. Reconcile
    // observed keys with GTK's authoritative state before the next key event.
    using namespace clay::keycodes;
    const std::pair<uint64_t, unsigned> modifiers[] = {
        {kPhysicalShiftLeft, GDK_SHIFT_MASK},
        {kPhysicalShiftRight, GDK_SHIFT_MASK},
        {kPhysicalControlLeft, GDK_CONTROL_MASK},
        {kPhysicalControlRight, GDK_CONTROL_MASK},
        {kPhysicalAltLeft, GDK_ALT_MASK},
        {kPhysicalAltRight, GDK_ALT_MASK},
        {kPhysicalMetaLeft, GDK_SUPER_MASK | GDK_META_MASK},
        {kPhysicalMetaRight, GDK_SUPER_MASK | GDK_META_MASK}};
    std::vector<std::pair<uint64_t, uint64_t>> released;
    for (auto [physical, mask] : modifiers) {
      auto found = pressed_.find(physical);
      if (!(state & mask) && found != pressed_.end() &&
          found->second != kLogicalAltGraph) {
        released.emplace_back(found->first, found->second);
        pressed_.erase(found);
      }
    }
    for (auto [physical, logical] : released) {
      auto e = Event(physical, logical, kLynxKeyEventTypeUp);
      e.synthesized = true;
      send_(&e);
    }
  }
  lynx_key_event_t Event(uint64_t physical, uint64_t logical,
                         lynx_key_event_type_e type) {
    lynx_key_event_t e{};
    e.struct_size = sizeof(e);
    e.timestamp = g_get_monotonic_time();
    e.physical = physical;
    e.logical = logical;
    e.type = type;
    return e;
  }
  void Key(guint value, guint code, GdkModifierType state, bool down) {
    auto physical_it = xkb_to_physical_key_map.find(code);
    uint64_t physical = physical_it == xkb_to_physical_key_map.end()
                            ? 0x01500000000ULL | code
                            : physical_it->second;
    auto existing = pressed_.find(physical);
    if (!down && existing == pressed_.end()) return;
    uint64_t logical;
    if (existing != pressed_.end())
      logical = existing->second;
    else {
      auto logical_it = gtk_keyval_to_logical_key_map.find(value);
      auto unicode = gdk_keyval_to_unicode(gdk_keyval_to_lower(value));
      logical = logical_it != gtk_keyval_to_logical_key_map.end()
                    ? logical_it->second
                : unicode ? unicode
                          : 0x01500000000ULL | value;
    }
    auto type = !down                        ? kLynxKeyEventTypeUp
                : existing != pressed_.end() ? kLynxKeyEventTypeRepeat
                                             : kLynxKeyEventTypeDown;
    auto e = Event(physical, logical, type);
    char character[8]{};
    auto unicode = gdk_keyval_to_unicode(value);
    // Text commits belong to the IME bridge. This character accompanies the
    // raw key event; shortcuts must not be interpreted as text insertion.
    if (down && unicode && !g_unichar_iscntrl(unicode) &&
        !(state & (GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK))) {
      g_unichar_to_utf8(unicode, character);
      e.character = character;
    }
    if (down)
      pressed_[physical] = logical;
    else
      pressed_.erase(physical);
    send_(&e);
  }
  GtkWidget* widget_;
  GtkEventController* controller_;
  GtkEventController* focus_;
  Send send_;
  std::map<uint64_t, uint64_t> pressed_;
};
}  // namespace lynx::gtk
#endif
