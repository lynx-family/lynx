// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_INLINE_PLACEHOLDER_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_INLINE_PLACEHOLDER_SHADOW_NODE_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/base_text_shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

class InlinePlaceholderShadowNode : public BaseTextShadowNode {
 public:
  static ShadowNode* Make(int sign, const std::string& tag) {
    return new InlinePlaceholderShadowNode(sign, tag);
  }
  void OnAppendToParagraph(ParagraphBuilderHarmony& builder, float width,
                           float height) override;
  ~InlinePlaceholderShadowNode() override = default;
  InlinePlaceholderShadowNode(int sign, const std::string& tag)
      : BaseTextShadowNode(sign, tag) {}
  void MeasureChildrenNode(float width, MeasureMode width_mode, float height,
                           MeasureMode height_mode,
                           bool final_measure) override;
  bool IsPlaceholder() const override;

  int32_t Index() const { return placeholder_index_; }
  float CalcPlaceholderTopOffset(LineMetricsHarmony* line_metrics) const;
  void SetLineHeight(double height) { line_height_ = height; }

 private:
  OH_Drawing_PlaceholderVerticalAlignment CalcVerticalAlignValue(
      float* out_baseline_offset) const;
  LayoutResult size_;
  int32_t placeholder_index_;
  float baseline_offset_{0.f};
  double line_height_{0.f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_INLINE_PLACEHOLDER_SHADOW_NODE_H_
