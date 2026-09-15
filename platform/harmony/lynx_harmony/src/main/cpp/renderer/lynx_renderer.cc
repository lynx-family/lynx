// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer.h"

#include <unordered_map>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_render_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {
LynxRenderer::LynxRenderer(std::shared_ptr<LynxRendererContext> context,
                           int32_t sign, std::weak_ptr<UIBase> host)
    : context_(std::move(context)), host_(std::move(host)), sign_(sign) {}

LynxRenderer::~LynxRenderer() = default;

void LynxRenderer::UpdateDisplayList(DisplayList display_list) {
  display_list_ = std::move(display_list);
  if (!display_list_applier_) {
    display_list_applier_ =
        std::make_unique<LynxDisplayListApplier>(context_.get(), host_);
  }
  segments_ = display_list_applier_->UpdateDisplayList(display_list_);
  UpdateRenderNodeOrder();
}

void LynxRenderer::Draw(OH_Drawing_Canvas* canvas) { DrawSegment(0, canvas); }

void LynxRenderer::DrawSegment(size_t index, OH_Drawing_Canvas* canvas) {
  if (canvas == nullptr || !display_list_applier_ ||
      index >= segments_.size()) {
    return;
  }
  const auto& segment = segments_[index];
  display_list_applier_->ApplyDisplayList(display_list_, canvas, segment);
}

void LynxRenderer::InvalidateRenderNodes() {
  for (const auto& node : render_nodes_) {
    node->Invalidate();
  }
}

void LynxRenderer::UpdateRenderNodeOrder() {
  auto host = host_.lock();
  auto* owner = context_->GetUIOwner();
  if (!host || !owner || segments_.empty()) {
    render_nodes_.clear();
    return;
  }
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list_.GetContentItemsData());
  const bool has_root = items[0].type == DisplayListOpType::kBegin;
  const float width = has_root ? items[0].payload.begin.w : 0.f;
  const float height = has_root ? items[0].payload.begin.h : 0.f;
  auto& manager = NodeManager::Instance();
  std::unordered_map<ArkUI_NodeHandle, ArkUI_NodeHandle> previous_nodes;
  size_t node_index = 0;
  // Place native children and drawing segments in display-list order.
  // UIBase keeps ownership and its logical child tree unchanged.
  for (size_t i = 1; i < segments_.size(); ++i) {
    const auto& segment = segments_[i];
    auto parent_node = host->Node();
    ArkUI_NodeHandle child_node = nullptr;
    auto* child = owner->FindUIBySign(segment.preceding_view_id);
    if (child != nullptr && child->Parent() == host.get()) {
      const auto& draw_view =
          items[segment.start_item_index - 1].payload.draw_view;
      child->UpdateFragmentLayerOffset(draw_view.offset_x, draw_view.offset_y);
      auto attached_parent = manager.GetParent(child->DrawNode());
      if (attached_parent != nullptr) {
        child_node = child->DrawNode();
        parent_node = attached_parent;
        auto previous = previous_nodes[parent_node];
        if (previous && manager.GetNextSibling(previous) != child_node) {
          manager.RemoveNode(parent_node, child_node);
          manager.InsertNodeAfter(parent_node, child_node, previous);
        }
        previous_nodes[parent_node] = child_node;
      }
    }
    // A not-yet-attached native child must not hide the following CUI content.
    if (!segment.has_drawing) {
      continue;
    }
    if (node_index == render_nodes_.size()) {
      render_nodes_.emplace_back(std::make_unique<LynxRenderNode>(this));
    }
    auto& node = render_nodes_[node_index++];
    node->Update(i, width, height);
    node->AttachAfter(parent_node, previous_nodes[parent_node]);
    previous_nodes[parent_node] = node->Node();
    node->Invalidate();
  }
  render_nodes_.resize(node_index);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
