// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_TEXT_RENDER_H_
#define CLAY_UI_SHADOW_TEXT_RENDER_H_

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/component/text/unicode_util.h"
#include "clay/ui/shadow/inline_truncation_shadow_node.h"
#include "clay/ui/shadow/shadow_layout_context.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

class TextShadowNode;

struct LineInfo {
  int start;
  int end;
  int ellipsis_count;
};

class TextRender {
 public:
  explicit TextRender(TextShadowNode* measure_node)
      : measure_node_(measure_node) {}
  ~TextRender() = default;

  void Measure(const MeasureConstraint& constraint,
               ShadowLayoutContextMeasure* context);
  void BuildTextLayout(const MeasureConstraint& constraint,
                       LayoutContext* context);
  std::unique_ptr<txt::Paragraph> LayoutParagraph(double layout_width);

  void SetUpdateFlag(TextUpdateFlag flag) { update_flag_ = flag; }

  void AddUpdateFlag(TextUpdateFlag flag) {
    update_flag_ = static_cast<TextUpdateFlag>(static_cast<int>(update_flag_) |
                                               static_cast<int>(flag));
  }
  txt::Paragraph* GetCacheParagraph() { return cache_paragraph_.get(); }
  void SetCacheParagraph(std::unique_ptr<txt::Paragraph> paragraph) {
    cache_paragraph_ = std::move(paragraph);
  }
  std::unique_ptr<txt::Paragraph> MoveParagraph() {
    return std::move(cache_paragraph_);
  }

  TextAlignment EffectAlign();

#ifndef ENABLE_SKITY
  // support vertical-align attribute (only in skia)
  void ReprocessAttributeIfNeeded(double layout_width);
#endif
  std::shared_ptr<txt::Paragraph> LayoutXCharacter(double layout_width,
                                                   const TextStyle& style);
  float GetMaxFontSize();

  std::vector<LineInfo> GetLineInfo();

  // support -x-auto-font-size attribute
  void HandleAutoSize(const MeasureConstraint& constraint,
                      ShadowLayoutContextMeasure* context);
  bool CheckTextFullyDisplayed(const MeasureConstraint& constraint,
                               ShadowLayoutContextMeasure* context_measure);
  void TryShrinkFontSize(const MeasureConstraint& constraint,
                         ShadowLayoutContextMeasure* context_measure);
  void TryExpandFontSize(const MeasureConstraint& constraint,
                         ShadowLayoutContextMeasure* context_measure);
  // "shrink_or_expand = true" represents text needs to be shrink
  // "shrink_or_expand = false" represents text needs to be expand
  void FlexInlineFontSize(bool shrink_or_expand, float font_size,
                          ShadowNode* shadow_node);

  // support inline-truncation
  void HandleInlineTruncation(const MeasureConstraint& constraint,
                              ShadowLayoutContextMeasure* context);
  void ProcessTruncationContent(size_t& display_glyph_num, ShadowNode* node);

  static clay::Value GetTextInfo(const char* text, const clay::Value& params);

  static void MeasureText(const std::string& text, bool show_content,
                          const std::optional<double>& max_width,
                          const std::optional<uint32_t>& max_length,
                          const TextStyle& text_style, float* measured_width,
                          float* measured_height,
                          std::vector<std::string>* measured_content);

 private:
  TextShadowNode* measure_node_;
  float prev_layout_width_ = std::numeric_limits<float>::quiet_NaN();
  TextUpdateFlag update_flag_ = TextUpdateFlag::kUpdateFlagNone;
  int measured_width_;
  int measured_height_;
  std::unique_ptr<txt::Paragraph> cache_paragraph_;
  size_t end_glyph_position_ = 0;
  TextDirection truncation_direction_ = TextDirection::kLtr;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_TEXT_RENDER_H_
