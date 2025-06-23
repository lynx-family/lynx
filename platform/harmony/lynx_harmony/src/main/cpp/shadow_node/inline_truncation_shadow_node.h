// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_INLINE_TRUNCATION_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_INLINE_TRUNCATION_SHADOW_NODE_H_
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/base_text_shadow_node.h"
namespace lynx {
namespace tasm {
namespace harmony {
class InlineTruncationShadowNode : public BaseTextShadowNode {
 public:
  InlineTruncationShadowNode(int sign, const std::string& tag);

  static ShadowNode* Make(int sign, const std::string& tag) {
    return new InlineTruncationShadowNode(sign, tag);
  }

  bool IsInlineTruncation() const override;

  void OnAppendToParagraph(ParagraphBuilderHarmony& builder, float width,
                           float height) override;

  bool IsVirtual() const override;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_INLINE_TRUNCATION_SHADOW_NODE_H_
