// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/x_text_shadow_node.h"

#include <cstring>
#include <utility>

#include "clay/ui/common/attribute_utils.h"

namespace clay {

XTextShadowNode::XTextShadowNode(ShadowNodeOwner* owner, std::string tag,
                                 int id)
    : TextShadowNode(owner, std::move(tag), id) {}

void XTextShadowNode::SetAttribute(const char* attr_c,
                                   const clay::Value& value) {
  if (std::strcmp(attr_c, "ellipsize-mode") == 0) {
    const std::string mode = attribute_utils::GetCString(value);
    if (mode == "tail") {
      SetTextOverflow(TextOverflow::kEllipsis);
      return;
    }
    if (mode == "clip") {
      SetTextOverflow(TextOverflow::kClip);
      return;
    }
  }
  TextShadowNode::SetAttribute(attr_c, value);
}

}  // namespace clay
