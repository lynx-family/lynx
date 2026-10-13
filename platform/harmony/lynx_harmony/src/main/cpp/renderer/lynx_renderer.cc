// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer.h"

#include <sstream>
#include <string>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_render_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/svg_path_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

std::string BuildClipPath(float x, float y, float width, float height,
                          float density) {
  const float left = x * density;
  const float top = y * density;
  const float right = (x + width) * density;
  const float bottom = (y + height) * density;
  std::ostringstream path;
  SvgPathUtils::MoveTo(path, left, top);
  SvgPathUtils::LineTo(path, right, top);
  SvgPathUtils::LineTo(path, right, bottom);
  SvgPathUtils::LineTo(path, left, bottom);
  SvgPathUtils::Close(path);
  return path.str();
}

void SetClipPath(ArkUI_NodeHandle node, float width, float height,
                 const std::string& path) {
  ArkUI_NumberValue values[] = {
      {.i32 = static_cast<int32_t>(ARKUI_CLIP_TYPE_PATH)},
      {.f32 = width},
      {.f32 = height}};
  ArkUI_AttributeItem attribute = {
      .value = values, .size = 3, .string = path.c_str()};
  NodeManager::Instance().SetAttribute(node, NODE_CLIP_SHAPE, &attribute);
}

}  // namespace

LynxRenderer::LynxRenderer(std::shared_ptr<LynxRendererContext> context,
                           int32_t sign, std::weak_ptr<UIBase> host)
    : context_(std::move(context)), host_(std::move(host)), sign_(sign) {}

LynxRenderer::~LynxRenderer() { ResetHostClipPath(); }

void LynxRenderer::UpdateDisplayList(const DisplayList* display_list) {
  display_list_ = display_list;
  if (!display_list_applier_) {
    display_list_applier_ =
        std::make_unique<LynxDisplayListApplier>(context_.get(), host_);
  }
  UpdateHostClipPath();
  segments_ = display_list_applier_->UpdateDisplayList(*display_list_);
  UpdateRenderNodeOrder();
}

void LynxRenderer::ResetHostClipPath() {
  if (!host_has_clip_path_) {
    return;
  }
  if (auto host = host_.lock()) {
    NodeManager::Instance().ResetAttribute(host->Node(), NODE_CLIP_SHAPE);
  }
  host_has_clip_path_ = false;
}

void LynxRenderer::UpdateHostClipPath() {
  auto host = host_.lock();
  if (!host) {
    return;
  }
  const float width = host->width_;
  const float height = host->height_;
  const float density = host->GetContext()->ScaledDensity();
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list_->GetContentItemsData());
  int depth = 0;
  for (size_t i = 0; i < display_list_->GetContentItemsSize(); ++i) {
    const auto& item = items[i];
    if (item.type == DisplayListOpType::kBegin) {
      ++depth;
    } else if (item.type == DisplayListOpType::kEnd) {
      --depth;
    } else if (depth == 1 && item.type == DisplayListOpType::kClipRect) {
      const auto& clip = item.payload.clip_rect;
      if (!clip.has_radii) {
        // DrawClip extends the visible axis by the screen size. Preserve the
        // host border on the hidden axis, as in ordinary overflow clipping.
        const bool visible_x = clip.x < 0.f && clip.x + clip.w > width;
        const bool visible_y = clip.y < 0.f && clip.y + clip.h > height;
        if (visible_x != visible_y) {
          const float x = visible_x ? clip.x : 0.f;
          const float y = visible_y ? clip.y : 0.f;
          const float w = visible_x ? clip.w : width;
          const float h = visible_y ? clip.h : height;
          SetClipPath(host->Node(), w * density, h * density,
                      BuildClipPath(x, y, w, h, density));
          host_has_clip_path_ = true;
          return;
        }
      }
      break;
    }
  }
  ResetHostClipPath();
}

void LynxRenderer::Draw(OH_Drawing_Canvas* canvas) { DrawSegment(0, canvas); }

void LynxRenderer::DrawSegment(size_t index, OH_Drawing_Canvas* canvas) {
  if (canvas == nullptr || !display_list_applier_ || !display_list_ ||
      index >= segments_.size()) {
    return;
  }
  const auto& segment = segments_[index];
  display_list_applier_->ApplyDisplayList(*display_list_, canvas, segment);
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
      display_list_->GetContentItemsData());
  const bool has_root = items[0].type == DisplayListOpType::kBegin;
  const float width = has_root ? items[0].payload.begin.w : 0.f;
  const float height = has_root ? items[0].payload.begin.h : 0.f;
  auto& manager = NodeManager::Instance();
  base::OrderedFlatMap<ArkUI_NodeHandle, ArkUI_NodeHandle> previous_nodes;
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
