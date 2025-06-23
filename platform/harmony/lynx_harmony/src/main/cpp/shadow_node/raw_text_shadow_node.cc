// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/raw_text_shadow_node.h"

#include "base/include/string/string_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

bool RawTextShadowNode::IsVirtual() const { return true; }

void RawTextShadowNode::AppendToParagraph(ParagraphBuilderHarmony& builder,
                                          float width, float height) {
  // use word-break set by parent
  OnAppendToParagraph(builder, width, height);
}

void RawTextShadowNode::OnAppendToParagraph(ParagraphBuilderHarmony& builder,
                                            float width, float height) {
  BaseTextShadowNode::AddTextToBuilder(builder, text());
}

void RawTextShadowNode::UpdateWordBreakType(
    starlight::WordBreakType word_break) {
  // use parent's word break type
  word_break_ = word_break;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
