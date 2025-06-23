// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_text_shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

void InlineTextShadowNode::UpdateProps(PropBundleHarmony* props) {
  // todo(renzhongyue): implement styles for background and borders.
  BaseTextShadowNode::UpdateProps(props);
}

void InlineTextShadowNode::AppendToParagraph(ParagraphBuilderHarmony& builder,
                                             float width, float height) {
  builder.PushTextEventTarget(Signature(), event_through_, ignore_focus_);
  BaseTextShadowNode::AppendToParagraph(builder, width, height);
  builder.PopTextEventTarget();
}
void InlineTextShadowNode::OnAppendToParagraph(ParagraphBuilderHarmony& builder,
                                               float width, float height) {
  LoadFontFamilyIfNeeded(style_.GetFontFamilies(), builder.GetFontCollection());
  BaseTextShadowNode::OnAppendToParagraph(builder, width, height);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
