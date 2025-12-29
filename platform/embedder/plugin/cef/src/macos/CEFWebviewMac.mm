// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/plugin/cef/src/macos/CEFWebviewMac.h"

#include "include/cef_app.h"
#include "platform/embedder/plugin/cef/src/macos/CEFWebviewClientMac.h"

namespace lynx {
namespace plugin {
namespace embedder {

// LynxNativeView
void CEFWebviewMac::OnAttach() {
  if (browser_) {
    CefRefPtr<CefBrowserHost> host = browser_->GetHost();
    if (native_view_) {
      NSView* nsView = CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(native_view_);
      NSView* parentView = (__bridge NSView*)(lynx_view_get_native_window(lynx_view_));
      [parentView addSubview:nsView];
      attached_to_view_ = true;
    }
    host->WasHidden(false);
  }
}

void CEFWebviewMac::OnDetach() {
  if (browser_) {
    CefRefPtr<CefBrowserHost> host = browser_->GetHost();
    if (native_view_) {
      NSView* nsView = CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(native_view_);
      [nsView removeFromSuperview];
      attached_to_view_ = false;
    }
    host->WasHidden(true);
  }
}

void CEFWebviewMac::OnDestroy() {
  if (browser_) {
    CefRefPtr<CefBrowserHost> host = browser_->GetHost();
    host->CloseBrowser(true);
  }
  browser_.reset();
  if (client_) {
    client_->Reset();
    client_ = nullptr;
  }
  if (lynx_view_) {
    lynx_view_ = nullptr;
  }
}

void CEFWebviewMac::OnLayoutChanged(float left, float top, float width, float height,
                                    float pixel_ratio) {
  if (use_osr_) {
    NSRect prev = bounds_;
    bounds_ = NSMakeRect(left, top, width, height);
    if (browser_ &&
        (prev.size.width != bounds_.size.width || prev.size.height != bounds_.size.height)) {
      CefRefPtr<CefBrowserHost> host = browser_->GetHost();
      // host->NotifyScreenInfoChanged();
      host->WasResized();
      host->Invalidate(PET_VIEW);
    }
  } else {
    bounds_ = NSMakeRect(left, top, width, height);
    if (native_view_) {
      NSView* nsView = CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(native_view_);
      nsView.frame = bounds_;
    }
  }
}

float CEFWebviewMac::GetPixelRatio() const {
  if (!lynx_view_) {
    return 1.0;
  }
  NSView* parentView = (__bridge NSView*)(lynx_view_get_native_window(lynx_view_));
  if (!parentView.window) {
    return 1.0;
  }

  NSScreen* screen = parentView.window.screen;
  if (!screen) {
    return 1.0;
  }
  return screen.backingScaleFactor;
}

void CEFWebviewMac::SetupClient() {
  CefBrowserSettings settings;
  CefWindowInfo window_info;
  settings.chrome_status_bubble = STATE_DISABLED;
  NSView* view = (__bridge NSView*)(lynx_view_get_native_window(lynx_view_));
  if (use_osr_) {
    window_info.SetAsWindowless(CAST_NSVIEW_TO_CEF_WINDOW_HANDLE(view));
    window_info.shared_texture_enabled = 1;
    settings.windowless_frame_rate = fps_;
  } else {
    window_info.SetAsChild(CAST_NSVIEW_TO_CEF_WINDOW_HANDLE(view), CefRect(0, 0, 800, 600));
  }
  client_ = new CEFWebviewClientMac(this);
  CefBrowserHost::CreateBrowser(window_info, client_, url_, settings, nullptr, nullptr);
}

void CEFWebviewMac::OnMouseWheelEvent(int x, int y, int modifiers, double delta_x, double delta_y) {
  if (!browser_) {
    return;
  }
  if (CefRefPtr<CefBrowserHost> host = browser_->GetHost()) {
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = 0;
    if (modifiers & 1) {
      mouse_event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if (modifiers & 2) {
      mouse_event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }
    if (modifiers & 4) {
      mouse_event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    host->SendMouseWheelEvent(mouse_event, delta_x, delta_y);
  }
}

void CEFWebviewMac::OnMouseMoveEvent(int x, int y, int modifiers, bool mouse_leave) {
  if (!browser_) {
    return;
  }
  if (CefRefPtr<CefBrowserHost> host = browser_->GetHost()) {
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = 0;
    if (modifiers & 1) {
      mouse_event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if (modifiers & 2) {
      mouse_event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }
    if (modifiers & 4) {
      mouse_event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    host->SendMouseMoveEvent(mouse_event, mouse_leave);
  }
}

void CEFWebviewMac::OnMouseClickEvent(int x, int y, int buttons, bool mouse_up) {
  if (!browser_) {
    return;
  }

  if (!mouse_up) {
    const int kDoubleClickPixels = 5;
    const int kDoubleClickTime = 500;  // ms
    double currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch())
                             .count();
    if ((abs(last_click_x_ - x) > kDoubleClickPixels) ||
        (abs(last_click_y_ - y) > kDoubleClickPixels) ||
        ((currentTime - last_click_time_) > kDoubleClickTime) || buttons != last_click_button_) {
      last_click_count_ = 1;
    } else {
      ++last_click_count_;
    }
    last_click_time_ = currentTime;
    last_click_button_ = buttons;
    last_click_x_ = x;
    last_click_y_ = y;
  }

  if (CefRefPtr<CefBrowserHost> host = browser_->GetHost()) {
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    host->SendMouseClickEvent(
        mouse_event,
        buttons & 1 ? MBT_LEFT : (buttons & 2 ? MBT_RIGHT : (buttons & 4 ? MBT_MIDDLE : MBT_LEFT)),
        mouse_up, last_click_count_);
  }
}

void CEFWebviewMac::AddSubview(NSView* view) {
  if (lynx_view_) {
    NSView* parentView = (__bridge NSView*)(lynx_view_get_native_window(lynx_view_));
    [parentView addSubview:view];
  }
}

void CEFWebviewMac::RequestIME(void* callback, void* opaque) {
  // TODO(chenyouhui): Add implementation.
}

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

LYNX_EXTERN_C lynx_native_view_t* cef_webview_create_view(void* opaque) {
  lynx::plugin::embedder::CEFWebviewMac* view =
      new lynx::plugin::embedder::CEFWebviewMac(static_cast<lynx_view_t*>(opaque));
  return view->native_view();
}
