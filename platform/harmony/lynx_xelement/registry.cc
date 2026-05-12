// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/registry.h"

#include "platform/harmony/lynx_xelement/blur_view/ui_blur_view.h"
#include "platform/harmony/lynx_xelement/input/input_shadow_node.h"
#include "platform/harmony/lynx_xelement/input/ui_input.h"
#include "platform/harmony/lynx_xelement/input/ui_textarea.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/shadow_node/markdown_shadow_node.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/ui/ui_markdown.h"
#include "platform/harmony/lynx_xelement/overlay/overlay_shadow_node.h"
#include "platform/harmony/lynx_xelement/overlay/ui_overlay.h"
#include "platform/harmony/lynx_xelement/refresh/refresh_shadow_node.h"
#include "platform/harmony/lynx_xelement/refresh/ui_refresh.h"
#include "platform/harmony/lynx_xelement/scroll_coordinator/ui_scroll_coordinator.h"
#include "platform/harmony/lynx_xelement/viewpager/ui_viewpager.h"
#include "platform/harmony/lynx_xelement/viewpager/ui_viewpager_item.h"

namespace lynx {
namespace tasm {
namespace harmony {

void XElementRegistry::Initialize() {
  auto& map = LynxContext::GetCAPINodeInfoMap();
  map["blur-view"] = {UIBlurView::Make};
  map["input"] = {UIInput::Make, InputShadowNode::Make, LayoutNodeType::CUSTOM};
  map["textarea"] = {UITextArea::Make, InputShadowNode::Make,
                     LayoutNodeType::CUSTOM};
  map["overlay"] = {UIOverlay::Make, OverlayShadowNode::Make,
                    LayoutNodeType::CUSTOM};
  map["refresh"] = {UIRefresh::Make, RefreshShadowNode::Make,
                    LayoutNodeType::CUSTOM};
  map["refresh-header"] = {UIRefreshHeader::Make};
  map["scroll-coordinator"] = {UIScrollCoordinator::Make};
  map["scroll-coordinator-toolbar"] = {UIScrollCoordinatorToolBar::Make};
  map["scroll-coordinator-header"] = {UIScrollCoordinatorHeader::Make};
  map["scroll-coordinator-slot"] = {UIScrollCoordinatorSlot::Make};
  map["scroll-coordinator-slot-drag"] = {UIScrollCoordinatorSlotDrag::Make};
  map["viewpager"] = {UIViewPager::Make};
  map["viewpager-item"] = {UIViewPagerItem::Make};
  map["markdown"] = {UIMarkdown::Make, MarkdownShadowNode::Make,
                     LayoutNodeType::CUSTOM};
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
