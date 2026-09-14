// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/x_inline_image_shadow_node.h"

#include <utility>

namespace clay {

XInlineImageShadowNode::XInlineImageShadowNode(ShadowNodeOwner* owner,
                                               std::string tag, int id)
    : InlineImageShadowNode(owner, std::move(tag), id) {}

txt::PlaceholderAlignment XInlineImageShadowNode::GetPlaceholderAlignment()
    const {
  return txt::PlaceholderAlignment::kMiddle;
}

}  // namespace clay
