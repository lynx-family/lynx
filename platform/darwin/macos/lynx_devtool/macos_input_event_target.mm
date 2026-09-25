// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/darwin/macos/lynx_devtool/macos_input_event_target.h"

#import <AppKit/AppKit.h>

#include <cmath>

namespace lynx {
namespace devtool {

class MacOSInputEventTarget::Impl {
 public:
  explicit Impl(NSView* view) : view_(view) {}
  ~Impl() { Invalidate(); }

  NSWindow* GetWindow() const {
    if (!NSThread.isMainThread || !NSApp) {
      return nil;
    }
    NSWindow* window = view_.window;
    // AppKit rewrites queued mouse coordinates through the window server.
    // An unordered window has no usable mapping for that conversion.
    if (!window || !window.visible || window == closed_window_ || window.windowNumber <= 0 ||
        !window.contentView) {
      return nil;
    }
    const NSSize size = window.contentView.bounds.size;
    return size.width > 0 && size.height > 0 ? window : nil;
  }

  bool Inject(const input::PointerEvent& event) {
    if (!NSThread.isMainThread) {
      return false;
    }
    const auto* pointer = event.FindPointer(event.action_pointer_id);
    if (event.source_type != input::PointerSourceType::kMouse || event.pointers.size() != 1 ||
        !pointer || !std::isfinite(pointer->x) || !std::isfinite(pointer->y)) {
      Cancel();
      return false;
    }
    if (event.type == input::PointerEventType::kCancel) {
      return active_ && pointer->id == pointer_id_ && Cancel();
    }
    NSWindow* window = GetWindow();
    if (!window || (active_ && window != active_window_)) {
      Cancel();
      return false;
    }
    NSView* content = window.contentView;
    NSRect bounds = content.bounds;
    if (pointer->x < 0 || pointer->y < 0 || pointer->x >= NSWidth(bounds) ||
        pointer->y >= NSHeight(bounds)) {
      Cancel();
      return false;
    }
    // AppKit's window base space has an upward Y axis. Convert through the
    // content view so flipped roots and titled windows use the same contract.
    NSPoint point =
        NSMakePoint(NSMinX(bounds) + pointer->x,
                    content.isFlipped ? NSMinY(bounds) + pointer->y : NSMaxY(bounds) - pointer->y);
    point = [content convertPoint:point toView:nil];

    NSEventType type;
    switch (event.type) {
      case input::PointerEventType::kDown:
        if (active_) {
          Cancel();
          return false;
        }
        active_ = true;
        active_window_ = window;
        pointer_id_ = pointer->id;
        close_observer_ =
            [NSNotificationCenter.defaultCenter addObserverForName:NSWindowWillCloseNotification
                                                            object:window
                                                             queue:nil
                                                        usingBlock:^(NSNotification* notification) {
                                                          closed_window_ = notification.object;
                                                          Cancel();
                                                        }];
        type = NSEventTypeLeftMouseDown;
        break;
      case input::PointerEventType::kMove:
      case input::PointerEventType::kUp:
        if (!active_ || pointer->id != pointer_id_) {
          Cancel();
          return false;
        }
        type = event.type == input::PointerEventType::kUp ? NSEventTypeLeftMouseUp
                                                          : NSEventTypeLeftMouseDragged;
        break;
      default:
        Cancel();
        return false;
    }

    const bool accepted = Post(type, point, event.timestamp_us, event.click_count);
    if (!accepted) {
      Cancel();
      return false;
    }
    if (event.type == input::PointerEventType::kUp) {
      Reset();
    }
    return true;
  }

  void CancelActivePointer() { Cancel(); }

  void Invalidate() {
    Cancel();
    view_ = nil;
  }

 private:
  bool Post(NSEventType type, NSPoint point, int64_t timestamp_us, int click_count) {
    NSWindow* window = active_window_;
    if (!NSApp || !window || window.windowNumber <= 0) {
      return false;
    }
    NSTimeInterval timestamp =
        timestamp_us > 0 ? timestamp_us / 1000000.0 : NSProcessInfo.processInfo.systemUptime;
    NSEvent* event = [NSEvent mouseEventWithType:type
                                        location:point
                                   modifierFlags:0
                                       timestamp:timestamp
                                    windowNumber:window.windowNumber
                                         context:nil
                                     eventNumber:0
                                      clickCount:click_count
                                        pressure:type == NSEventTypeLeftMouseUp ? 0 : 1];
    if (!event) {
      return false;
    }
    // Queue instead of synchronously calling mouseDown: native controls can
    // enter a tracking loop that must still be able to receive the later up.
    [NSApp postEvent:event atStart:NO];
    return true;
  }

  bool Cancel() {
    if (!active_) {
      return true;
    }
    // AppKit has no mouse-cancel event. Drag well outside the content before
    // releasing to end native tracking and cancel a pending tap recognizer.
    NSWindow* window = active_window_;
    const NSRect bounds = window.contentView.bounds;
    NSPoint outside = NSMakePoint(-NSWidth(bounds) - 10000, -NSHeight(bounds) - 10000);
    const bool moved = Post(NSEventTypeLeftMouseDragged, outside, 0, 0);
    const bool released = Post(NSEventTypeLeftMouseUp, outside, 0, 0);
    Reset();
    return moved && released;
  }

  void Reset() {
    if (close_observer_) {
      [NSNotificationCenter.defaultCenter removeObserver:close_observer_];
      close_observer_ = nil;
    }
    active_ = false;
    active_window_ = nil;
  }

  __weak NSView* view_;
  __weak NSWindow* active_window_ = nil;
  __weak NSWindow* closed_window_ = nil;
  id close_observer_ = nil;
  bool active_ = false;
  int32_t pointer_id_ = 0;
};

MacOSInputEventTarget::MacOSInputEventTarget(void* host_view)
    : impl_(std::make_unique<Impl>((__bridge NSView*)host_view)) {}

MacOSInputEventTarget::~MacOSInputEventTarget() = default;

input::PointerCapabilities MacOSInputEventTarget::GetPointerCapabilities() const {
  input::PointerCapabilities capabilities;
  if (impl_->GetWindow()) {
    capabilities.default_source_type = input::PointerSourceType::kMouse;
    capabilities.supports_mouse = true;
  }
  return capabilities;
}

bool MacOSInputEventTarget::InjectPointerEvent(const input::PointerEvent& event) {
  return impl_->Inject(event);
}

void MacOSInputEventTarget::Invalidate() { impl_->Invalidate(); }

void MacOSInputEventTarget::CancelActivePointer() { impl_->CancelActivePointer(); }

}  // namespace devtool
}  // namespace lynx
