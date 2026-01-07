// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "platform/embedder/plugin/cef/src/macos/CEFWebviewClientMac.h"

#include "platform/embedder/plugin/cef/src/macos/CEFWebviewMac.h"

namespace lynx {
namespace plugin {
namespace embedder {

void CEFWebviewClientMac::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
  if (!webview_) {
    return;
  }
  auto& bounds = static_cast<CEFWebviewMac*>(webview_)->bounds_;
  rect.Set(0, 0, std::max(1L, long(bounds.size.width)), std::max(1L, long(bounds.size.height)));
}

bool CEFWebviewClientMac::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) {
  float ratio = webview_->GetPixelRatio();
  CefRect view_rect;
  GetViewRect(browser, view_rect);
  screen_info.device_scale_factor = ratio;
  screen_info.rect = view_rect;
  screen_info.available_rect = view_rect;
  return true;
}

bool CEFWebviewClientMac::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY,
                                         int& screenX, int& screenY) {
  NSPoint view_pt = {static_cast<CGFloat>(viewX), static_cast<CGFloat>(viewY)};
  NSView* view = (__bridge NSView*)(lynx_view_get_native_window(webview_->GetLynxView()));
  NSPoint window_pt = [view convertPoint:view_pt toView:nil];
  NSPoint screen_pt = [[view window] convertPointToScreen:window_pt];
  screenX = screen_pt.x;
  screenY = screen_pt.y;
  return true;
}

void CEFWebviewClientMac::OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
                                             const RectList& dirtyRects,
                                             const CefAcceleratedPaintInfo& info) {
  float ratio = webview_->GetPixelRatio();
  auto& bounds = static_cast<CEFWebviewMac*>(webview_)->bounds_;
  if (info.extra.source_size.width != int(bounds.size.width * ratio) ||
      info.extra.source_size.height != int(bounds.size.height * ratio)) {
    return;
  }
  // TODO(chenyouhui): check image sink
  static const float transform[3 * 3] = {1, 0, 0, 0, -1, 1, 0, 0, 1};
  webview_->PresentSurface(
      info.extra.source_size.width, info.extra.source_size.height, transform,
      reinterpret_cast<lynx_surface_handle_t*>(info.shared_texture_io_surface));
}

void CEFWebviewClientMac::OnImeCompositionRangeChanged(
    CefRefPtr<CefBrowser> browser, const CefRange& selection_range,
    const CefRenderHandler::RectList& character_bounds) {
  auto& bounds = static_cast<CEFWebviewMac*>(webview_)->bounds_;
  CefRenderHandler::RectList device_bounds;
  for (auto it = character_bounds.begin(); it != character_bounds.end(); ++it) {
    CefRect rect(bounds.origin.x + it->x, bounds.origin.y + it->y, it->width, it->height);
    device_bounds.push_back(rect);
  }
  [text_input_client_ ChangeCompositionRange:selection_range character_bounds:device_bounds];
}

namespace {
static BOOL isKeyPadEvent(NSEvent* event) {
  if ([event modifierFlags] & NSEventModifierFlagNumericPad) {
    return true;
  }

  switch ([event keyCode]) {
    case 71:  // Clear
    case 81:  // =
    case 75:  // /
    case 67:  // *
    case 78:  // -
    case 69:  // +
    case 76:  // Enter
    case 65:  // .
    case 82:  // 0
    case 83:  // 1
    case 84:  // 2
    case 85:  // 3
    case 86:  // 4
    case 87:  // 5
    case 88:  // 6
    case 89:  // 7
    case 91:  // 8
    case 92:  // 9
      return true;
  }

  return false;
}

static int getModifiersForEvent(NSEvent* event) {
  int modifiers = 0;

  if ([event modifierFlags] & NSEventModifierFlagControl) {
    modifiers |= EVENTFLAG_CONTROL_DOWN;
  }
  if ([event modifierFlags] & NSEventModifierFlagShift) {
    modifiers |= EVENTFLAG_SHIFT_DOWN;
  }
  if ([event modifierFlags] & NSEventModifierFlagOption) {
    modifiers |= EVENTFLAG_ALT_DOWN;
  }
  if ([event modifierFlags] & NSEventModifierFlagCommand) {
    modifiers |= EVENTFLAG_COMMAND_DOWN;
  }
  if ([event modifierFlags] & NSEventModifierFlagCapsLock) {
    modifiers |= EVENTFLAG_CAPS_LOCK_ON;
  }

  if ([event type] == NSEventTypeKeyUp || [event type] == NSEventTypeKeyDown ||
      [event type] == NSEventTypeFlagsChanged) {
    // Only perform this check for key events
    if (isKeyPadEvent(event)) {
      modifiers |= EVENTFLAG_IS_KEY_PAD;
    }
  }

  // OS X does not have a modifier for NumLock, so I'm not entirely sure how to
  // set EVENTFLAG_NUM_LOCK_ON;
  //
  // There is no EVENTFLAG for the function key either.

  // Mouse buttons
  switch ([event type]) {
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeLeftMouseDown:
    case NSEventTypeLeftMouseUp:
      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
      break;
    case NSEventTypeRightMouseDragged:
    case NSEventTypeRightMouseDown:
    case NSEventTypeRightMouseUp:
      modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
      break;
    case NSEventTypeOtherMouseDragged:
    case NSEventTypeOtherMouseDown:
    case NSEventTypeOtherMouseUp:
      modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
      break;
    default:
      break;
  }

  return modifiers;
}

static void getKeyEvent(CefKeyEvent& keyEvent, NSEvent* event) {
  if ([event type] == NSEventTypeKeyDown || [event type] == NSEventTypeKeyUp) {
    NSString* s = [event characters];
    if ([s length] > 0) {
      keyEvent.character = [s characterAtIndex:0];
    }

    s = [event charactersIgnoringModifiers];
    if ([s length] > 0) {
      keyEvent.unmodified_character = [s characterAtIndex:0];
    }
  }

  if ([event type] == NSEventTypeFlagsChanged) {
    keyEvent.character = 0;
    keyEvent.unmodified_character = 0;
  }

  keyEvent.native_key_code = [event keyCode];

  keyEvent.modifiers = getModifiersForEvent(event);
}

static BOOL isKeyUpEvent(NSEvent* event) {
  if ([event type] != NSEventTypeFlagsChanged) {
    return [event type] == NSEventTypeKeyUp;
  }

  // FIXME: This logic fails if the user presses both Shift keys at once, for
  // example: we treat releasing one of them as keyDown.
  switch ([event keyCode]) {
    case 54:  // Right Command
    case 55:  // Left Command
      return ([event modifierFlags] & NSEventModifierFlagCommand) == 0;

    case 57:  // Capslock
      return ([event modifierFlags] & NSEventModifierFlagCapsLock) == 0;

    case 56:  // Left Shift
    case 60:  // Right Shift
      return ([event modifierFlags] & NSEventModifierFlagShift) == 0;

    case 58:  // Left Alt
    case 61:  // Right Alt
      return ([event modifierFlags] & NSEventModifierFlagOption) == 0;

    case 59:  // Left Ctrl
    case 62:  // Right Ctrl
      return ([event modifierFlags] & NSEventModifierFlagControl) == 0;

    case 63:  // Function
      return ([event modifierFlags] & NSEventModifierFlagFunction) == 0;
  }
  return false;
}

static void ime_callback(NSEvent* event, CEFWebviewClientMac* client) {
  CefKeyEvent keyEvent;
  switch (event.type) {
    case NSEventTypeFlagsChanged:
      if (isKeyUpEvent(event)) {
        client->OnKeyUp(event);
      } else {
        client->OnKeyDown(event);
      }
      break;
    case NSEventTypeKeyDown:
      client->OnKeyDown(event);
      break;
    case NSEventTypeKeyUp:
      client->OnKeyUp(event);
      break;
    default:
      break;
  }
}
}  // namespace

void CEFWebviewClientMac::OnKeyUp(NSEvent* event) {
  CefKeyEvent keyEvent;
  getKeyEvent(keyEvent, event);

  keyEvent.type = KEYEVENT_KEYUP;
  webview_->GetBrowser()->GetHost()->SendKeyEvent(keyEvent);
}

void CEFWebviewClientMac::OnKeyDown(NSEvent* event) {
  if ([event type] != NSEventTypeFlagsChanged) {
    if (text_input_client_) {
      [text_input_client_ HandleKeyEventBeforeTextInputClient:event];

      [text_input_context_osr_mac_ handleEvent:event];

      CefKeyEvent keyEvent;
      getKeyEvent(keyEvent, event);

      [text_input_client_ HandleKeyEventAfterTextInputClient:keyEvent];
    }
  }
}

void CEFWebviewClientMac::OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser,
                                                     TextInputMode input_mode) {
  switch (input_mode) {
    case CEF_TEXT_INPUT_MODE_NONE:
      if (!ime_shown_) {
        return;
      }
      webview_->RegisterIMEHandler(nullptr, nullptr);
      text_input_context_osr_mac_ = nullptr;
      text_input_client_ = nullptr;
      ime_shown_ = false;
      break;
    default:
      if (ime_shown_) {
        return;
      }
      ime_shown_ = true;
      text_input_client_ =
          [[CefTextInputClientOSRMac alloc] initWithBrowser:webview_->GetBrowser()];
      text_input_context_osr_mac_ = [[NSTextInputContext alloc] initWithClient:text_input_client_];
      webview_->RequestFocus();
      webview_->RegisterIMEHandler(reinterpret_cast<void*>(ime_callback), this);
      [text_input_context_osr_mac_ activate];
      break;
  }
}

bool CEFWebviewClientMac::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor,
                                         cef_cursor_type_t type,
                                         const CefCursorInfo& custom_cursor_info) {
  if (webview_ && webview_->UseOSR()) {
    [CAST_CEF_CURSOR_HANDLE_TO_NSCURSOR(cursor) set];
    if (type == CT_NONE) {
      [NSCursor unhide];
    }
    return true;
  }
  return false;
}

void CEFWebviewClientMac::AttachToView(CefRefPtr<CefBrowser> browser) {
  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  cef_window_handle_t native_view = host->GetWindowHandle();
  webview_->SetNativeView(native_view);

  NSView* nsView = CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(native_view);
  nsView.autoresizingMask = NSViewNotSizable;
  nsView.frame = static_cast<CEFWebviewMac*>(webview_)->bounds_;

  if (!static_cast<CEFWebviewMac*>(webview_)->IsAttachedToView()) {
    static_cast<CEFWebviewMac*>(webview_)->AddSubview(nsView);
  }
}

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx
