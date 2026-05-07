// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_WINDOWLESS_LYNX_UI_RENDERER_WINDOWLESS_H_
#define PLATFORM_EMBEDDER_WINDOWLESS_LYNX_UI_RENDERER_WINDOWLESS_H_

#include <memory>
#include <thread>

#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "clay/public/clay.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"
#include "clay/shell/platform/headless/public/embdder_headless_delegate.h"
#include "platform/embedder/lynx_ui_renderer.h"
#include "platform/embedder/windowless/lynx_windowless_renderer_priv.h"

namespace lynx {
namespace embedder {
class LynxUIRendererWindowless : public LynxUIRenderer,
                                 public HeadlessDelegate {
 public:
  explicit LynxUIRendererWindowless(lynx_view_builder_t* builder);
  ~LynxUIRendererWindowless() override;

  bool RunsTaskOnCurrentThread() const;

  void PostTask(ClayTask task, uint64_t target_time);

  void SetParent(NativeWindow parent) override {}

  NativeWindow GetNativeWindow() override { return nullptr; }

  void SetPixelRatio(float pixel_ratio) override;
  void SetFrame(float x, float y, float width, float height) override;

  void Reset() override;

  void OnEnterForeground() override;

  void OnEnterBackground() override;

  void InjectBubbleEvent(const char* params) override {}
  lynx::tasm::UIDelegate* GetUIDelegate() override;

  void RegisterIMEHandler(void* handler, void* opaque) override {}

  /* headless delegate */
  const char* GetClipboardData() const override;
  void SetClipboardData(const char* data) override;
  void ActivateSystemCursor(int type, const char* path) override;
  void ShowTextInput() override;
  void HideTextInput() override;
  void UpdateCaretPosition(float x, float y, float width,
                           float height) override;
  void SetMarkedTextRect(float x, float y, float width, float height) override;
  void SetEditableTransform(const float transform_matrix[16]) override;
  void SetCursorPosition(int position) override;

 private:
  lynx::fml::RefPtr<LynxWindowlessRenderer> windowless_renderer_;
  std::thread::id main_thread_id_;
  ClayTaskRunnerDescription description_ = {};

  std::unique_ptr<clay::ClayHeadlessEngine> headless_engine_;
  std::unique_ptr<lynx::tasm::UIDelegate> ui_delegate_;
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_WINDOWLESS_LYNX_UI_RENDERER_WINDOWLESS_H_
