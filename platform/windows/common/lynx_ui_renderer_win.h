// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_WINDOWS_COMMON_LYNX_UI_RENDERER_WIN_H_
#define PLATFORM_WINDOWS_COMMON_LYNX_UI_RENDERER_WIN_H_

#include <list>
#include <memory>

#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "clay/shell/platform/windows/dpi_utils.h"
#include "clay/shell/platform/windows/flutter_window.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"
#include "platform/embedder/frame_timing_listener_impl.h"
#include "platform/embedder/lynx_ui_renderer.h"

namespace lynx {
namespace embedder {
class LynxUIRendererWin : public LynxUIRenderer {
 public:
  explicit LynxUIRendererWin(lynx_view_builder_t* builder);
  ~LynxUIRendererWin() override;

  void SetParent(NativeWindow parent) override;

  NativeWindow GetNativeWindow() override;

  void SetPixelRatio(float pixel_ratio) override;
  void SetFrame(float x, float y, float width, float height) override;

  void Reset() override;

  void OnEnterForeground() override;

  void OnEnterBackground() override;

  void EmulateMouseEvent(const char* event_name, float x, float y,
                         float delta_x, float delta_y) override;

  void InjectBubbleEvent(const char* params) override;

  void RegisterNativeView(const char* name, lynx_native_view_creator creator,
                          void* opaque) override;

  lynx::tasm::UIDelegate* GetUIDelegate() override;

  void RegisterIMEHandler(void* handler, void* opaque) override;

  void AddClient(LynxViewClients* client) override;

 private:
  void AdjustWindowRect();

  std::unique_ptr<clay::FlutterWindowsEngine> engine_;
  std::unique_ptr<clay::FlutterWindowsView> flutter_view_;
  std::unique_ptr<lynx::tasm::UIDelegateClay> ui_delegate_;
  std::shared_ptr<FrameTimingListenerImpl> frame_timing_listener_;
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_WINDOWS_COMMON_LYNX_UI_RENDERER_WIN_H_
