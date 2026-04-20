// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_MACOS_COMMON_LYNX_UI_RENDERER_H_
#define PLATFORM_DARWIN_MACOS_COMMON_LYNX_UI_RENDERER_H_

#include <list>
#include <memory>

#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "platform/embedder/frame_timing_listener_impl.h"
#include "platform/embedder/lynx_ui_renderer.h"

namespace lynx {
namespace embedder {
class LynxUIRendererImpl : public LynxUIRenderer {
 public:
  explicit LynxUIRendererImpl(lynx_view_builder_t* builder);
  ~LynxUIRendererImpl() override;

  void SetParent(NativeWindow parent) override;

  NativeWindow GetNativeWindow() override;

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
  void* lynx_ui_renderer_ = nullptr;
  std::unique_ptr<lynx::tasm::UIDelegateClay> ui_delegate_;

  std::shared_ptr<FrameTimingListenerImpl> frame_timing_listener_;
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_DARWIN_MACOS_COMMON_LYNX_UI_RENDERER_H_
