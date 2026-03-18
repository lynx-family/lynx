// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/viewpager/ui_viewpager_item.h"

#include <string>

#include "core/renderer/dom/element_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

void UIViewPagerItem::UpdateLayout(float left, float top, float width,
                                   float height, const float* paddings,
                                   const float* margins, const float* sticky,
                                   float max_height, uint32_t node_index) {
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
}

UIViewPagerItem::UIViewPagerItem(LynxContext* context, int sign,
                                 const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
