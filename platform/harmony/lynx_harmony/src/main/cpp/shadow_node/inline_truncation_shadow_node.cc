// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_truncation_shadow_node.h"

#include <string>
namespace lynx {
namespace tasm {
namespace harmony {
InlineTruncationShadowNode::InlineTruncationShadowNode(int sign,
                                                       const std::string& tag)
    : BaseTextShadowNode(sign, tag) {}
bool InlineTruncationShadowNode::IsInlineTruncation() const { return true; }

void InlineTruncationShadowNode::OnAppendToParagraph(
    ParagraphBuilderHarmony& builder, float width, float height) {
  LoadFontFamilyIfNeeded(style_.GetFontFamilies(), builder.GetFontCollection());
  BaseTextShadowNode::OnAppendToParagraph(builder, width, height);
}

bool InlineTruncationShadowNode::IsVirtual() const { return true; }

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
