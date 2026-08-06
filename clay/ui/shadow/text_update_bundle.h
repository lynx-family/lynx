// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_TEXT_UPDATE_BUNDLE_H_
#define CLAY_UI_SHADOW_TEXT_UPDATE_BUNDLE_H_

#include <list>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/public/style_types.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/text/inline_emoji_bitmap.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/shadow/bundle.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck
namespace clay {

struct TextInfo {
  int id = -1;
  int parent_id = -1;
  bool need_mount = true;
  // for inline_image/inline_view/inline_truncation
  std::optional<int> placeholder_index;
  // for inline_image/inline_text/inline_truncation
  std::optional<ClayLayoutStyles> view_style;
  // for inline_image
  std::optional<FloatPoint> location;
  // for inline_text
  std::optional<std::list<TextRange>> range_;
  // inline-text inside inline-view is laid out by the inline-view subtree, not
  // by the outer text paragraph.
  bool use_inline_view_layout_box = false;
  std::unique_ptr<txt::Paragraph> inline_view_paragraph;
  std::u16string inline_view_text;
};

class TextUpdateBundle : public Bundle {
 public:
  TextUpdateBundle() = default;
  ~TextUpdateBundle();
  void SetParagraph(std::unique_ptr<txt::Paragraph> paragraph) {
    paragraph_ = std::move(paragraph);
  }
  void SetTextPaintAlign(TextAlignment align) { text_paint_align_ = align; }
  void SetText(std::u16string text) { text_ = text; }
  void SetGradientShaderMap(
      std::map<int, std::shared_ptr<ColorSource>>& gradient_shader_map,
      std::map<int, std::pair<size_t, size_t>>& range_map) {
    gradient_shader_map_ = std::move(gradient_shader_map);
    range_map_ = std::move(range_map);
  }
  void SetTextStrokeMap(std::unordered_map<int, TextStroke>& text_stroke_map) {
    text_stroke_map_ = std::move(text_stroke_map);
  }
  void SetLineSpacingOffset(double offset) { line_spacing_offset_ = offset; }
  void PushTextInfo(TextInfo info) { info_.emplace_back(std::move(info)); }
  void SetInlineEmojiInfo(std::vector<InlineEmojiInfo> inline_emoji_info) {
    inline_emoji_info_ = std::move(inline_emoji_info);
  }
  void UpdateExtraData(BaseView* view) override;

 private:
  FRIEND_TEST(TextTest, InlineViewNestedTextBuildsOneLocalParagraph);

  std::unique_ptr<txt::Paragraph> paragraph_ = nullptr;
  TextAlignment text_paint_align_ = TextAlignment::kLeft;
  std::u16string text_;
  double line_spacing_offset_ = 0;
  std::map<int, std::shared_ptr<ColorSource>> gradient_shader_map_;
  std::map<int, std::pair<size_t, size_t>> range_map_;
  std::unordered_map<int, TextStroke> text_stroke_map_;
  std::vector<TextInfo> info_;
  std::vector<InlineEmojiInfo> inline_emoji_info_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_TEXT_UPDATE_BUNDLE_H_
