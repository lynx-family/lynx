// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
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

void LynxRenderer::UpdateDisplayList(const DisplayList* display_list) {
  display_list_ = display_list;
  if (!display_list_applier_) {
    display_list_applier_ =
        std::make_unique<LynxDisplayListApplier>(context_.get(), host_);
  }
  segments_ = display_list_applier_->UpdateDisplayList(*display_list_);
  UpdateDrawingBounds();
  UpdateRenderNodeOrder();
}

void LynxRenderer::Draw(OH_Drawing_Canvas* canvas) {
  if (!draw_first_segment_in_node_) {
    DrawSegment(0, canvas);
  }
}

void LynxRenderer::DrawSegment(size_t index, OH_Drawing_Canvas* canvas) {
  if (canvas == nullptr || !display_list_applier_ || !display_list_ ||
      index >= segments_.size()) {
    return;
  }
  const auto& segment = segments_[index];
  if (draw_first_segment_in_node_) {
    auto lynx_context = context_->GetLynxContext();
    if (!lynx_context) {
      return;
    }
    const float density = lynx_context->ScaledDensity();
    OH_Drawing_CanvasSave(canvas);
    OH_Drawing_CanvasTranslate(canvas, -overflow_left_ * density,
                               -overflow_top_ * density);
    display_list_applier_->ApplyDisplayList(*display_list_, canvas, segment);
    OH_Drawing_CanvasRestore(canvas);
  } else {
    display_list_applier_->ApplyDisplayList(*display_list_, canvas, segment);
  }
}

void LynxRenderer::InvalidateRenderNodes() {
  for (const auto& node : render_nodes_) {
    node->Invalidate();
  }
}

void LynxRenderer::UpdateDrawingBounds() {
  draw_first_segment_in_node_ = false;
  overflow_left_ = 0.f;
  overflow_top_ = 0.f;
  drawing_width_ = 0.f;
  drawing_height_ = 0.f;
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list_->GetContentItemsData());
  const size_t count = display_list_->GetContentItemsSize();
  if (count == 0 || items[0].type != DisplayListOpType::kBegin) {
    return;
  }
  const auto& root = items[0].payload.begin;
  drawing_width_ = root.w;
  drawing_height_ = root.h;
  // A zero-size host supplies an empty drawing canvas even without clipping.
  // Preserve its layout and use drawing nodes for its flattened content.
  if ((root.w > 0.f && root.h > 0.f) || !root.overflow_x || !root.overflow_y ||
      display_list_->RootNeedClipBounds()) {
    return;
  }

  // The root's position is already applied by its native host. Accumulate
  // nested offsets to size a separate drawing surface without changing layout.
  float left = 0.f;
  float top = 0.f;
  float right = std::max(0.f, root.w);
  float bottom = std::max(0.f, root.h);
  float x = 0.f;
  float y = 0.f;
  std::vector<std::pair<float, float>> offsets;
  for (size_t i = 0; i < count; ++i) {
    const auto& item = items[i];
    if (item.type == DisplayListOpType::kBegin) {
      offsets.emplace_back(x, y);
      if (i != 0) {
        x += item.payload.begin.x;
        y += item.payload.begin.y;
      }
    } else if (item.type == DisplayListOpType::kEnd && !offsets.empty()) {
      x = offsets.back().first;
      y = offsets.back().second;
      offsets.pop_back();
    } else if (item.type == DisplayListOpType::kRecordBox) {
      const auto& box = item.payload.record_box;
      if (box.w > 0.f && box.h > 0.f) {
        left = std::min(left, x + box.x);
        top = std::min(top, y + box.y);
        right = std::max(right, x + box.x + box.w);
        bottom = std::max(bottom, y + box.y + box.h);
      }
    }
  }
  if (right <= left || bottom <= top) {
    return;
  }
  draw_first_segment_in_node_ = true;
  overflow_left_ = left;
  overflow_top_ = top;
  drawing_width_ = right - left;
  drawing_height_ = bottom - top;
}

void LynxRenderer::UpdateRenderNodeOrder() {
  auto host = host_.lock();
  auto* owner = context_->GetUIOwner();
  if (!host || !owner || segments_.empty()) {
    render_nodes_.clear();
    return;
  }
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list_->GetContentItemsData());
  auto& manager = NodeManager::Instance();
  base::OrderedFlatMap<ArkUI_NodeHandle, ArkUI_NodeHandle> previous_nodes;
  size_t node_index = 0;
  // Place native children and drawing segments in display-list order.
  // UIBase keeps ownership and its logical child tree unchanged.
  for (size_t i = draw_first_segment_in_node_ ? 0 : 1; i < segments_.size();
       ++i) {
    const auto& segment = segments_[i];
    auto parent_node = host->Node();
    ArkUI_NodeHandle child_node = nullptr;
    auto* child = owner->FindUIBySign(segment.preceding_view_id);
    if (i > 0 && child != nullptr && child->Parent() == host.get()) {
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
    node->Update(i, drawing_width_, drawing_height_, overflow_left_,
                 overflow_top_);
    if (i == 0) {
      node->AttachFirst(parent_node);
    } else {
      node->AttachAfter(parent_node, previous_nodes[parent_node]);
    }
    previous_nodes[parent_node] = node->Node();
    node->Invalidate();
  }
  render_nodes_.resize(node_index);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
