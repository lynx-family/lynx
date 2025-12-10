// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_BASE_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_BASE_TEXT_SHADOW_NODE_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/vertical_align_style.h"

namespace clay {

class BaseTextShadowNode : public ShadowNode {
 public:
  BaseTextShadowNode(ShadowNodeOwner* owner, std::string tag, int id);

  void OnLayout(float width, TextMeasureMode width_mode, float height,
                TextMeasureMode height_mode,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders) override;

  void AddChild(ShadowNode* node) override;
  void AddChild(ShadowNode* child, int index) override;

  void RemoveChild(ShadowNode* child) override;

  bool IsBaseTextShadowNode() override { return true; }

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  void SetEnableFontScaling(bool enabled);
  void SetFontSize(float font_size);
  void SetLineHeight(float line_height);
  // Not supported by the underlining layout engine.
  void SetLineSpacing(float line_spacing);
  void SetLetterSpacing(float letter_spacing);
  void SetTextAlign(TextAlignment text_align);
  void SetFontWeight(FontWeight font_weight);
  void SetFontStyle(FontStyle font_style);
  void SetTextColor(const Color& text_color);
  void SetTextBackgroundColor(const Color& color);
  void SetFontFamily(const std::string& font_family);
  std::optional<TextDecoration> GetTextDecoration() const {
    return text_style_ ? std::nullopt : text_style_->text_decoration;
  }
  void SetTextDirection(TextDirection text_direction);
  void SetTextDecoration(const TextDecoration& text_decoration);
  void AppendTextShadow(Shadow&& text_shadow);
  void SetTextShadows(std::vector<Shadow>&& text_shadows);
  void SetTextStrokeColor(Color color);
  void SetTextStrokeWidth(double width);
  void SetTextGradient(const Gradient& gradient);
  void SetTextMaxLine(uint32_t max_lines);
  void SetTextOverflow(TextOverflow overflow);
  void SetTextEllipsis(std::u16string ellipsis);
  void SetWordBreak(WordBreak break_type);
  void SetTextSingleLineVerticalAlign(const clay::Value& value);

  void RelayoutWhenSetFontFamily(const std::string& font_family);

  void EnsureDefaultStyle();

  void TextLayout(LayoutContext* context) override;
  void PreLayout(PreLayoutContext* context) override;
  void ProcessChildLayout(LayoutContext* context);

  void MeasureInlineView(const MeasureConstraint& constraint);

  void AlignNativeNode(txt::Paragraph* paragraph);

  // Used for calculate skia paragraph line_height multiplier.
  // Retrieve the maximum line height and font size.
  void CollectMaxLineHeight(float& line_height, float& font_size);

  void SetBaselineOffset(double baseline_offset) override {
    EnsureDefaultStyle();
    text_style_->baseline_shift = -baseline_offset;
  }

  void SetWhiteSpaceType(WhiteSpace white_space);
  void SetTextIndent(double indent);

  std::u16string GetRawText();

  std::optional<TextStyle> text_style_;
  // The absolute line height.
  std::optional<float> line_height_;
  std::optional<uint32_t> max_length_;

  double stroke_width_ = 0.0;
  std::optional<Color> stroke_color_;

  bool enable_auto_font_size_ = false;
  double auto_font_size_max_size_ = 0.;
  double auto_font_size_min_size_ = 0.;
  double auto_font_size_step_granularity_ = 1.;
  std::vector<double> auto_font_size_preset_sizes_;
  bool text_indent_use_percent_ = false;
  float text_indent_ = 0.f;
  TextMeasureMode width_mode_ = TextMeasureMode::kDefinite;
  void SetAttribute(KeywordID attr, const char* attr_c,
                    const clay::Value&) override;
  void CreateRawTextNodeIfNeed(std::string text);

 protected:
 private:
  void ResetTextColorAndGradient();

  fml::WeakPtrFactory<BaseTextShadowNode> weak_factory_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_BASE_TEXT_SHADOW_NODE_H_
