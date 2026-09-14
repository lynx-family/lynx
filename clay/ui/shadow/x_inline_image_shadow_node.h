// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_X_INLINE_IMAGE_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_X_INLINE_IMAGE_SHADOW_NODE_H_

#include <string>

#include "clay/ui/shadow/inline_image_shadow_node.h"

namespace clay {

class XInlineImageShadowNode : public InlineImageShadowNode {
 public:
  XInlineImageShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~XInlineImageShadowNode() override = default;

 protected:
  friend class TextTest_XInlineImageUsesMiddlePlaceholderAlignment_Test;

  txt::PlaceholderAlignment GetPlaceholderAlignment() const override;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_X_INLINE_IMAGE_SHADOW_NODE_H_
