// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_render_node.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxRenderNode::LynxRenderNode(LynxRenderer* renderer)
    : renderer_(renderer),
      node_(NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM)) {
  if (node_ == nullptr) {
    return;
  }
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_HIT_TEST_BEHAVIOR,
      static_cast<int32_t>(ARKUI_HIT_TEST_MODE_NONE));
  NodeManager::Instance().AddNodeCustomEventReceiver(node_,
                                                     CustomEventReceiver);
  NodeManager::Instance().RegisterNodeCustomEvent(
      node_, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW, this);
}

LynxRenderNode::~LynxRenderNode() {
  if (node_ == nullptr) {
    return;
  }
  Detach();
  NodeManager::Instance().UnregisterNodeCustomEvent(
      node_, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW);
  NodeManager::Instance().RemoveNodeCustomEventReceiver(node_,
                                                        CustomEventReceiver);
  NodeManager::Instance().DisposeNode(node_);
}

void LynxRenderNode::SetSegmentIndex(size_t segment_index) {
  segment_index_ = segment_index;
}

void LynxRenderNode::UpdateFrame(float width, float height) {
  if (node_ == nullptr) {
    return;
  }
  NodeManager::Instance().SetAttributeWithNumberValue(node_, NODE_POSITION, 0.f,
                                                      0.f);
  NodeManager::Instance().SetAttributeWithNumberValue(node_, NODE_WIDTH, width);
  NodeManager::Instance().SetAttributeWithNumberValue(node_, NODE_HEIGHT,
                                                      height);
}

void LynxRenderNode::AttachAfter(ArkUI_NodeHandle parent,
                                 ArkUI_NodeHandle sibling) {
  if (node_ == nullptr || parent == nullptr) {
    return;
  }
  Detach();
  if (sibling != nullptr &&
      NodeManager::Instance().GetParent(sibling) == parent &&
      NodeManager::Instance().InsertNodeAfter(parent, node_, sibling)) {
    return;
  }
  NodeManager::Instance().InsertNode(parent, node_, -1);
}

void LynxRenderNode::Detach() {
  if (node_ == nullptr) {
    return;
  }
  auto parent = NodeManager::Instance().GetParent(node_);
  if (parent != nullptr) {
    NodeManager::Instance().RemoveNode(parent, node_);
  }
}

void LynxRenderNode::Invalidate() {
  if (node_ != nullptr) {
    NodeManager::Instance().Invalidate(node_);
  }
}

void LynxRenderNode::CustomEventReceiver(ArkUI_NodeCustomEvent* event) {
  if (event == nullptr ||
      OH_ArkUI_NodeCustomEvent_GetEventTargetId(event) != LYNX_EVENT_ID ||
      OH_ArkUI_NodeCustomEvent_GetEventType(event) !=
          ARKUI_NODE_CUSTOM_EVENT_ON_DRAW) {
    return;
  }
  auto* render_node =
      static_cast<LynxRenderNode*>(OH_ArkUI_NodeCustomEvent_GetUserData(event));
  if (render_node == nullptr || render_node->renderer_ == nullptr) {
    return;
  }
  auto* draw_context = OH_ArkUI_NodeCustomEvent_GetDrawContextInDraw(event);
  if (draw_context == nullptr) {
    return;
  }
  render_node->renderer_->DrawSegment(
      render_node->segment_index_,
      reinterpret_cast<OH_Drawing_Canvas*>(
          OH_ArkUI_DrawContext_GetCanvas(draw_context)));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
