// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_placeholder_shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

void InlinePlaceholderShadowNode::OnAppendToParagraph(
    ParagraphBuilderHarmony& builder, float width, float height) {
  baseline_offset_ = 0.f;
  OH_Drawing_PlaceholderVerticalAlignment vertical_align_type =
      CalcVerticalAlignValue(&baseline_offset_);
  PlaceholderHarmony placeholder_harmony{size_.width_, size_.height_,
                                         vertical_align_type, baseline_offset_};
  placeholder_index_ = builder.GetCharCount();
  builder.AddPlaceholder(placeholder_harmony, Signature());
}

void InlinePlaceholderShadowNode::MeasureChildrenNode(float width,
                                                      MeasureMode width_mode,
                                                      float height,
                                                      MeasureMode height_mode,
                                                      bool final_measure) {
  size_ =
      MeasureLayoutNode(width, width_mode, height, height_mode, final_measure);
}

bool InlinePlaceholderShadowNode::IsPlaceholder() const { return true; }

OH_Drawing_PlaceholderVerticalAlignment
InlinePlaceholderShadowNode::CalcVerticalAlignValue(
    float* out_baseline_offset) const {
  LynxVerticalAlignStyle vertical_align_style =
      text_props_.has_value() ? text_props_->vertical_align_style_
                              : LynxVerticalAlignStyle();
  switch (vertical_align_style.vertical_align_type) {
    case starlight::VerticalAlignType::kBaseline:
      *out_baseline_offset = size_.baseline_ - size_.height_;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kSub:
      *out_baseline_offset = -size_.height_ * 0.1;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kSuper:
      *out_baseline_offset = size_.height_ * 0.1;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kTop:
    case starlight::VerticalAlignType::kTextTop:
      return ALIGNMENT_TOP_OF_ROW_BOX;
    case starlight::VerticalAlignType::kMiddle:
    case starlight::VerticalAlignType::kCenter:
      return ALIGNMENT_CENTER_OF_ROW_BOX;
    case starlight::VerticalAlignType::kBottom:
    case starlight::VerticalAlignType::kTextBottom:
      return ALIGNMENT_BOTTOM_OF_ROW_BOX;
    case starlight::VerticalAlignType::kLength:
      *out_baseline_offset =
          ScaleDensity() * vertical_align_style.baseline_offset;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    case starlight::VerticalAlignType::kPercent:
      *out_baseline_offset =
          line_height_ * vertical_align_style.baseline_offset * 0.01f;
      return ALIGNMENT_OFFSET_AT_BASELINE;
    default:
      *out_baseline_offset = 0.f;
      return ALIGNMENT_ABOVE_BASELINE;
  }
}

float InlinePlaceholderShadowNode::CalcPlaceholderTopOffset(
    lynx::tasm::harmony::LineMetricsHarmony* line_metrics) const {
  LynxVerticalAlignStyle vertical_align_style =
      text_props_.has_value() ? text_props_->vertical_align_style_
                              : LynxVerticalAlignStyle();
  switch (vertical_align_style.vertical_align_type) {
    case starlight::VerticalAlignType::kBaseline:
    case starlight::VerticalAlignType::kSub:
    case starlight::VerticalAlignType::kSuper:
    case starlight::VerticalAlignType::kLength:
    case starlight::VerticalAlignType::kPercent:
      return line_metrics->Top() + line_metrics->Ascent() - size_.height_ -
             baseline_offset_;
    case starlight::VerticalAlignType::kTop:
    case starlight::VerticalAlignType::kTextTop:
      return line_metrics->Top();
    case starlight::VerticalAlignType::kMiddle:
      return line_metrics->Ascent() -
             (line_metrics->XHeight() + size_.height_) / 2.f;
    case starlight::VerticalAlignType::kCenter:
      return line_metrics->Top() +
             (line_metrics->Height() - size_.height_) / 2.f;
    case starlight::VerticalAlignType::kBottom:
    case starlight::VerticalAlignType::kTextBottom:
      return line_metrics->Top() + (line_metrics->Height() - size_.height_);
    default:
      return line_metrics->Top() + line_metrics->Ascent() - size_.height_;
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
