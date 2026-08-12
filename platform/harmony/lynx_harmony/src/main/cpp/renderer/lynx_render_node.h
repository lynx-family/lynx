// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDER_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDER_NODE_H_

#include <arkui/native_node.h>

#include <cstddef>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxRenderer;

class LynxRenderNode {
 public:
  explicit LynxRenderNode(LynxRenderer* renderer);
  ~LynxRenderNode();

  LynxRenderNode(const LynxRenderNode&) = delete;
  LynxRenderNode& operator=(const LynxRenderNode&) = delete;

  void SetSegmentIndex(size_t segment_index);
  void UpdateFrame(float width, float height);
  void AttachAfter(ArkUI_NodeHandle parent, ArkUI_NodeHandle sibling);
  void Detach();
  void Invalidate();

 private:
  static void CustomEventReceiver(ArkUI_NodeCustomEvent* event);

  LynxRenderer* renderer_{nullptr};
  ArkUI_NodeHandle node_{nullptr};
  size_t segment_index_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDER_NODE_H_
