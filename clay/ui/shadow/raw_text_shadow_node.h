// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_RAW_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_RAW_TEXT_SHADOW_NODE_H_

#include <cstddef>
#include <string>
#include <vector>

#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/text_shadow_node.h"

namespace clay {

class BaseTextShadowNode;
class LayoutContextText;
class RenderRawText;
class RawTextShadowNode : public ShadowNode {
 public:
  RawTextShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~RawTextShadowNode() override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void SetText(const std::string& text);
  void SetText(const std::u16string& text);

  void TextLayout(LayoutContext* context) override;
  bool IfNeedTextIndent();

  std::u16string Text() { return text_.substr(0, truncated_index_); }

  bool IsVirtual() override { return true; }

  bool IsRawTextShadowNode() override { return true; }

  std::u16string CollapsesWhitespaces(
      const std::u16string& text,
      std::vector<size_t>* layout_text_utf16_to_raw_end_indices = nullptr,
      std::vector<size_t>* layout_text_utf32_to_raw_end_indices = nullptr);

  TextShadowNode* FindTextShadowNodeAncestor();
  BaseTextShadowNode* FindBaseTextShadowNodeAncestor();

  std::u16string ProcessWordBreakIfNeed(const std::u16string& text);

  void AddTextWithInlineEmoji(LayoutContextText* text_context,
                              const std::u16string& text,
                              bool need_text_indent);
  // Returns the layout text length in UTF-32 code points.
  size_t GetLayoutTextLength() const;
  // Converts a UTF-16 layout text length to the corresponding UTF-32 length.
  size_t GetLayoutTextLengthForUtf16Length(
      size_t layout_text_utf16_length) const;
  // Maps a UTF-32 layout text length back to the raw UTF-16 end index.
  size_t GetRawEndIndexForLayoutTextLength(size_t layout_text_length) const;
  // These helpers keep UTF-16 semantics for paragraph/txt indices.
  size_t GetLayoutTextUtf16Length() const;
  size_t GetRawEndIndexForLayoutTextUtf16Length(
      size_t layout_text_utf16_length) const;

 private:
  struct InlineEmojiTextRange {
    size_t start_utf16 = 0;
    size_t end_utf16 = 0;
    size_t start_utf32 = 0;
    size_t end_utf32 = 0;
  };

  size_t CurrentRawTextEnd() const;
  size_t LayoutTextUtf16Length() const;
  size_t RawEndIndexForLayoutTextUtf16Index(
      size_t layout_text_utf16_index) const;
  size_t RawEndIndexForLayoutTextUtf32Index(
      size_t layout_text_utf32_index) const;
  void BuildIdentityLayoutTextMapping(const std::u16string& text);

  std::u16string origin_text_;
  std::u16string text_;
  std::vector<InlineEmojiTextRange> inline_emoji_text_ranges_;
  std::vector<size_t> layout_text_utf16_to_raw_end_indices_;
  std::vector<size_t> layout_text_utf32_to_raw_end_indices_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_RAW_TEXT_SHADOW_NODE_H_
