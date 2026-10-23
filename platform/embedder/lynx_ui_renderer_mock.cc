// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/interfaces/renderer_factory.h"
#include "platform/embedder/lynx_ui_renderer.h"

namespace lynx {
namespace embedder {
class MockLynxUIRenderer : public LynxUIRenderer {
 public:
  explicit MockLynxUIRenderer(RendererConfig config) : LynxUIRenderer(config) {}
  ~MockLynxUIRenderer() override {}

  void SetParent(NativeWindow parent) override {}

  NativeWindow GetNativeWindow() override { return nullptr; }

  void OnEnterForeground() override {}

  void OnEnterBackground() override {}

  void RegisterIMEHandler(void* handler, void* opaque) override {}

  lynx::tasm::UIDelegate* GetUIDelegate() override { return nullptr; }
};

std::unique_ptr<LynxUIRenderer> CreateDefaultUIRenderer(RendererConfig config) {
  return std::make_unique<MockLynxUIRenderer>(std::move(config));
}
}  // namespace embedder
}  // namespace lynx
