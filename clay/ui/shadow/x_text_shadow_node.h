// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_X_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_X_TEXT_SHADOW_NODE_H_

#include <string>

#include "clay/ui/shadow/text_shadow_node.h"

namespace clay {

class XTextShadowNode : public TextShadowNode {
 public:
  XTextShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~XTextShadowNode() override = default;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_X_TEXT_SHADOW_NODE_H_
