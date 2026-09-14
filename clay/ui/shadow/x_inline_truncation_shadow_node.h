// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_X_INLINE_TRUNCATION_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_X_INLINE_TRUNCATION_SHADOW_NODE_H_

#include <string>

#include "clay/ui/shadow/inline_truncation_shadow_node.h"

namespace clay {

class XInlineTruncationShadowNode : public InlineTruncationShadowNode {
 public:
  XInlineTruncationShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~XInlineTruncationShadowNode() override = default;

 protected:
  void AppendTruncationPrefix(LayoutContextText* context) override;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_X_INLINE_TRUNCATION_SHADOW_NODE_H_
