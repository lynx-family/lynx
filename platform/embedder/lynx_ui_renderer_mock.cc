// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_ui_renderer.h"

namespace lynx {
namespace embedder {
class MockLynxUIRenderer : public LynxUIRenderer {
 public:
  explicit MockLynxUIRenderer(lynx_view_builder_t* builder) {}
  ~MockLynxUIRenderer() override {}

  void SetParent(NativeWindow parent) override {}

  NativeWindow GetNativeWindow() override { return nullptr; }

  void SetFrame(float x, float y, float width, float height) override {}

  void OnEnterForeground() override {}

  void OnEnterBackground() override {}

  void RegisterIMEHandler(void* handler, void* opaque) override {}

  lynx::tasm::UIDelegate* GetUIDelegate() override { return nullptr; }
};

std::unique_ptr<LynxUIRenderer> LynxUIRenderer::CreateWithBuilder(
    lynx_view_builder_t* builder) {
  return std::make_unique<MockLynxUIRenderer>(builder);
}
}  // namespace embedder
}  // namespace lynx
