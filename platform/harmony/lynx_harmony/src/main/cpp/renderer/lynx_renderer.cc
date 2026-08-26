// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_render_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxRenderer::LynxRenderer(std::shared_ptr<LynxRendererContext> context,
                           int32_t sign)
    : context_(std::move(context)), sign_(sign) {}

LynxRenderer::~LynxRenderer() {
  render_nodes_.clear();
  if (display_list_applier_) {
    display_list_applier_->Reset();
  }
}

void LynxRenderer::UpdateDisplayList(DisplayList&& display_list,
                                     const std::shared_ptr<UIBase>& host) {
  display_list_ = std::move(display_list);
  if (!display_list_applier_) {
    display_list_applier_ =
        std::make_unique<LynxDisplayListApplier>(context_.get());
  }
  host_ = host;
  segment_result_ = SegmentDisplayList(display_list_);
  display_list_applier_->UpdateDisplayListResources(display_list_);
  UpdateRenderNodes(host);
}

void LynxRenderer::Draw(OH_Drawing_Canvas* canvas,
                        const std::shared_ptr<UIBase>& host) {
  if (canvas == nullptr || host == nullptr || !display_list_applier_ ||
      segment_result_.segments.empty() ||
      !segment_result_.segments[0].has_drawable_content) {
    return;
  }
  display_list_applier_->DrawSegment(display_list_, segment_result_,
                                     segment_result_.segments[0], host, canvas);
}

void LynxRenderer::DrawSegment(size_t segment_index,
                               OH_Drawing_Canvas* canvas) {
  auto host = host_.lock();
  if (canvas == nullptr || host == nullptr || !display_list_applier_ ||
      segment_index >= segment_result_.segments.size()) {
    return;
  }
  display_list_applier_->DrawSegment(display_list_, segment_result_,
                                     segment_result_.segments[segment_index],
                                     host, canvas);
}

void LynxRenderer::UpdateRenderNodeOrder(const std::shared_ptr<UIBase>& host) {
  host_ = host;
  UpdateRenderNodes(host);
}

void LynxRenderer::UpdateBounds(float width, float height) {
  width_ = width;
  height_ = height;
  for (const auto& node : render_nodes_) {
    node->UpdateFrame(width_, height_);
  }
}

void LynxRenderer::InvalidateRenderNodes() {
  for (const auto& node : render_nodes_) {
    node->Invalidate();
  }
}

void LynxRenderer::UpdateRenderNodes(const std::shared_ptr<UIBase>& host) {
  if (host == nullptr || context_ == nullptr) {
    render_nodes_.clear();
    return;
  }

  size_t render_node_index = 0;
  for (size_t segment_index = 1;
       segment_index < segment_result_.segments.size(); ++segment_index) {
    const auto& segment = segment_result_.segments[segment_index];
    auto* ui_owner = context_->GetUIOwner();
    auto* preceding_view =
        ui_owner != nullptr ? ui_owner->FindUIBySign(segment.preceding_view_id)
                            : nullptr;
    if (preceding_view != nullptr) {
      preceding_view->UpdateLayoutOffsetIfNeeded(
          segment.preceding_view_offset_x, segment.preceding_view_offset_y);
    }
    if (!segment.has_drawable_content) {
      continue;
    }

    if (render_node_index == render_nodes_.size()) {
      render_nodes_.emplace_back(std::make_unique<LynxRenderNode>(this));
    }
    auto& render_node = render_nodes_[render_node_index++];
    render_node->SetSegmentIndex(segment_index);
    render_node->UpdateFrame(width_, height_);

    ArkUI_NodeHandle parent_node = host->Node();
    ArkUI_NodeHandle sibling_node = nullptr;
    if (preceding_view != nullptr) {
      auto candidate = preceding_view->DrawNode();
      if (candidate != nullptr &&
          NodeManager::Instance().GetParent(candidate) == parent_node) {
        sibling_node = candidate;
      }
    }
    render_node->AttachAfter(parent_node, sibling_node);
  }

  render_nodes_.resize(render_node_index);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
