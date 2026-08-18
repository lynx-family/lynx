// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/platform_renderer_scroll.h"

#include <functional>
#include <stack>
#include <string>

#include "core/renderer/dom/fragment/display_list_reader.h"
#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

bool PlatformRendererScroll::WalkFramesRelativeToContent(
    const fml::RefPtr<PlatformRendererImpl>& renderer, float base_x,
    float base_y, int depth_base, bool skip_root_begin_offset,
    const std::function<WalkAction(const FrameInfo&, DisplayListOpType)>&
        visitor) {
  if (!renderer) {
    return true;
  }
  const DisplayList& display_list = renderer->GetDisplayList();
  DisplayListReader reader(display_list);
  const auto& children = renderer->Children();
  size_t child_idx = 0;

  int local_depth = 0;
  int skip_subtree_depth = -1;
  // DisplayList is linear instead of a real tree object. Even when skipping a
  // node's subtree, traversal still has to consume ops until the matching End
  // is reached so we know where that subtree finishes.
  auto is_in_skipped_subtree = [&local_depth, &skip_subtree_depth]() {
    return skip_subtree_depth >= 0 && local_depth > skip_subtree_depth;
  };
  std::stack<LayoutOffset> offset_stack;
  offset_stack.push({base_x, base_y});

  while (reader.HasNext()) {
    const DisplayListItem& item = reader.Next();
    switch (item.type) {
      case DisplayListOpType::kBegin: {
        const auto& begin = item.payload.begin;
        const LayoutOffset& offset = offset_stack.top();
        const bool is_local_root = local_depth == 0;
        float absolute_x = offset.x;
        float absolute_y = offset.y;
        if (!skip_root_begin_offset || !is_local_root) {
          absolute_x += begin.x;
          absolute_y += begin.y;
        }
        FrameInfo frame{.id = begin.id,
                        .left = absolute_x,
                        .top = absolute_y,
                        .width = begin.w,
                        .height = begin.h,
                        .depth = depth_base + local_depth};
        if (!is_in_skipped_subtree()) {
          const WalkAction& action = visitor(frame, item.type);
          if (action == WalkAction::kStop) {
            return false;
          } else if (action == WalkAction::kSkipSubtree) {
            // Store the skipped subtree root depth. Descendant ops have a
            // greater depth and are ignored until traversal leaves this
            // subtree.
            skip_subtree_depth = local_depth;
          }
        }
        offset_stack.push({absolute_x, absolute_y});
        ++local_depth;
        break;
      }
      case DisplayListOpType::kDrawView: {
        if (child_idx < children.size()) {
          fml::RefPtr<PlatformRendererImpl> child_renderer =
              fml::static_ref_ptr_cast<PlatformRendererImpl>(
                  children[child_idx++]);
          if (!is_in_skipped_subtree()) {
            const LayoutOffset& offset = offset_stack.top();
            if (!WalkFramesRelativeToContent(child_renderer, offset.x, offset.y,
                                             depth_base + local_depth, false,
                                             visitor)) {
              return false;
            }
          }
        }
        break;
      }
      case DisplayListOpType::kScrollContentEnd: {
        if (!is_in_skipped_subtree()) {
          const auto& scroll_content_end = item.payload.scroll_content_end;
          // ScrollContentEnd is emitted before the owner's End op, so
          // local_depth points to the next depth while the owner is still open.
          const int owner_depth =
              local_depth > 0 ? local_depth - 1 : local_depth;
          FrameInfo frame{
              .content_width = scroll_content_end.width,
              .content_height = scroll_content_end.height,
              .depth = depth_base + owner_depth,
          };
          if (visitor(frame, item.type) == WalkAction::kStop) {
            return false;
          }
        }
        break;
      }
      case DisplayListOpType::kEnd: {
        if (local_depth > 0) {
          --local_depth;
          offset_stack.pop();
        }
        if (skip_subtree_depth >= 0 && local_depth <= skip_subtree_depth) {
          // Restore normal visitor behavior after leaving the skipped subtree.
          skip_subtree_depth = -1;
        }
        break;
      }
      default:
        break;
    }
  }
  return true;
}

PlatformRendererScroll::PlatformRendererScroll() = default;

PlatformRendererScroll::~PlatformRendererScroll() = default;

void PlatformRendererScroll::UpdateScrollAttributes(
    const fml::RefPtr<PropBundle>& scroll_attributes) {
  if (!scroll_attributes || !scroll_attributes->IsNative()) {
    return;
  }
  NativePropBundle* native_scroll_attributes =
      static_cast<NativePropBundle*>(scroll_attributes.get());
  for (const auto& [key, value] : native_scroll_attributes->GetProps()) {
    UpdateScrollAttributeInternal(key, value);
  }
  OnPropsUpdated(scroll_props_);
}

void PlatformRendererScroll::GenerateContentInfoFromDisplayList(
    const fml::RefPtr<PlatformRendererImpl>& root) {
  if (root) {
    content_info_.Reset();
    WalkFramesRelativeToContent(
        root, 0.f, 0.f, 0, true,
        [this](const FrameInfo& frame, DisplayListOpType op) {
          if (frame.depth == 0 && op == DisplayListOpType::kScrollContentEnd) {
            content_info_.width = frame.content_width;
            content_info_.height = frame.content_height;
            return WalkAction::kContinue;
          } else if (frame.depth == 1 && op == DisplayListOpType::kBegin) {
            content_info_.children_offset.emplace_back(
                ScrollContentOffset{.x = frame.left, .y = frame.top});
            // The direct child offset has been recorded. Its descendants do
            // not need to be visited when resolving scroll-to-index.
            return WalkAction::kSkipSubtree;
          }
          return WalkAction::kContinue;
        });
    OnContentInfoUpdated(content_info_);
  }
}

void PlatformRendererScroll::UpdateScrollAttributeInternal(
    const std::string& key, const lepus::Value& value) {
  if (key == kScrollX) {
    const auto& value_str = value.StdString();
    scroll_props_.is_vertical = value_str == kFalse;
  } else if (key == kScrollY) {
    const auto& value_str = value.StdString();
    scroll_props_.is_vertical = value_str == kTrue;
  } else if (key == kScrollOrientation && value.IsString()) {
    const auto& value_str = value.StdString();
    scroll_props_.is_vertical = value_str == kVertical;
  }
}

}  // namespace tasm
}  // namespace lynx
