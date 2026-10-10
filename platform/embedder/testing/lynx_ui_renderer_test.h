// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_TESTING_LYNX_UI_RENDERER_TEST_H_
#define PLATFORM_EMBEDDER_TESTING_LYNX_UI_RENDERER_TEST_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "platform/embedder/lynx_ui_renderer.h"

namespace lynx {
namespace embedder {

// Records the output of the real engine without creating a native window.
struct TestPaintingState {
  struct Node {
    std::string tag;
    int parent = -1;
    float width = 0;
    float height = 0;
  };
  std::unordered_map<int, Node> nodes;
};

class TestLynxUIRenderer : public LynxUIRenderer {
 public:
  explicit TestLynxUIRenderer(lynx_view_builder_t* builder);
  ~TestLynxUIRenderer() override;

  void SetParent(NativeWindow parent) override {}
  NativeWindow GetNativeWindow() override { return nullptr; }
  void OnEnterForeground() override {}
  void OnEnterBackground() override {}
  void RegisterIMEHandler(void* handler, void* opaque) override {}
  tasm::UIDelegate* GetUIDelegate() override { return ui_delegate_.get(); }

  std::shared_ptr<TestPaintingState> painting_state() { return state_; }

 private:
  std::shared_ptr<TestPaintingState> state_;
  std::unique_ptr<tasm::UIDelegate> ui_delegate_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_TESTING_LYNX_UI_RENDERER_TEST_H_
