// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_CEF_WEBVIEW_WIN_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_CEF_WEBVIEW_WIN_H_

#include <Windows.h>

#include "platform/embedder/plugin/cef/src/cef_webview.h"

namespace lynx {
namespace plugin {
namespace embedder {

class CEFWebviewClientWin;
class CEFWebviewWin : public CEFWebview {
 public:
  explicit CEFWebviewWin(lynx_view_t* lynx_view);

  // LynxNativeView
  void OnAttach() override;
  void OnDetach() override;
  void OnDestroy() override;
  void OnLayoutChanged(float left, float top, float width, float height,
                       float pixel_ratio) override;

  float GetPixelRatio() const override;
  void SetupClient() override;
  void OnMouseWheelEvent(int x, int y, int modifiers, double delta_x,
                         double delta_y) override;
  void OnMouseMoveEvent(int x, int y, int modifiers, bool mouse_leave) override;
  void OnMouseClickEvent(int x, int y, int buttons, bool mouse_up) override;

  void MoveOwnedWinOnly();
  void AdjustOwnedWinAndChild(HWND child);

 private:
  friend CEFWebviewClientWin;
  // for Windows 7
  HWND GetOrCreateOwnedWin();

  HWND win7_owned_win_{nullptr};
  RECT bounds_{0, 0, 0, 0};
  HWND hwnd_{nullptr};

  int last_click_x_ = 0;
  int last_click_y_ = 0;
  int last_click_button_ = 0;
  int last_click_count_ = 1;
  double last_click_time_ = 0;
};

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_CEF_WEBVIEW_WIN_H_
