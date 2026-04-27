// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/raw_text_shadow_node.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/string/string_utils.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/component/text/unicode_util.h"
#include "clay/ui/platform/text_emoji_provider.h"
#include "clay/ui/rendering/render_dummy.h"
#include "clay/ui/shadow/base_text_shadow_node.h"
#include "clay/ui/shadow/icu_substitute.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_shadow_node.h"

namespace clay {

static const char16_t kZeroWidthJoinerCharacter = u'\u200D';
static const char16_t kZeroWidthSpaceCharacter = u'\u200B';
static const char16_t kZeroWidthWordJoinerCharacter = u'\u2060';

namespace {

bool IsBracketEmojiCandidate(const std::u16string& text, size_t start,
                             size_t* end) {
  if (start >= text.size() || text[start] != u'[') {
    return false;
  }
  size_t current = start + 1;
  if (current >= text.size() || text[current] == u']' ||
      text[current] == u'[') {
    return false;
  }
  while (current < text.size() && text[current] != u']') {
    if (text[current] == u'[') {
      return false;
    }
    ++current;
  }
  if (current >= text.size()) {
    return false;
  }
  *end = current + 1;
  return true;
}

}  // namespace

RawTextShadowNode::RawTextShadowNode(ShadowNodeOwner* owner, std::string tag,
                                     int id)
    : ShadowNode(owner, tag, id) {}

RawTextShadowNode::~RawTextShadowNode() = default;

void RawTextShadowNode::SetAttribute(const char* attr_c,
                                     const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kText) {
    SetText(attribute_utils::GetCString(value));
  }
  // set parent text node update flag
  MarkNeedsUpdate(TextUpdateFlag::kUpdateFlagChildren);
}

void RawTextShadowNode::SetText(const std::string& text) {
  std::u16string text_u16 = UnicodeUtil::Utf8ToUtf16(text);
  SetText(text_u16);
}

void RawTextShadowNode::SetText(const std::u16string& text) {
  if (text != text_) {
    text_ = text;
    origin_text_ = text_;
    SetEndIndex(text_.length());
    MarkDirty();
  }
}

void RawTextShadowNode::TextLayout(LayoutContext* context) {
  if (!context) {
    return;
  }
#ifndef CLAY_ENABLE_TTTEXT
  text_ = ProcessWordBreakIfNeed(origin_text_);
#endif
  auto need_text_indent = IfNeedTextIndent();
  LayoutContextText* text_context = static_cast<LayoutContextText*>(context);
  inline_emoji_text_ranges_.clear();
  layout_text_to_raw_end_indices_.clear();
  auto start = text_context->TextSizeIncludingPlaceholders();
  auto parent_text_shadow_node = FindTextShadowNodeAncestor();
  std::optional<TextStyle> text_style = std::nullopt;
  if (parent_text_shadow_node) {
    text_style = parent_text_shadow_node->text_style_;
  }
  if (text_style.has_value() && text_style->white_space.has_value() &&
      (text_style->white_space == WhiteSpace::kNoWrap ||
       text_style->white_space == WhiteSpace::kNormal)) {
    auto text = CollapsesWhitespaces(text_.substr(0, GetEndIndex()),
                                     &layout_text_to_raw_end_indices_);
    AddTextWithInlineEmoji(text_context, text, need_text_indent);
  } else {
    auto text = text_.substr(0, GetEndIndex());
    BuildIdentityLayoutTextMapping(text.length());
    AddTextWithInlineEmoji(text_context, text, need_text_indent);
  }
  if (parent_ && parent_->IsInlineTextShadowNode()) {
    auto end = text_context->TextSizeIncludingPlaceholders();
    static_cast<InlineTextShadowNode*>(parent_)->AddTextRange(start, end);
  }
}

std::u16string RawTextShadowNode::ProcessWordBreakIfNeed(
    const std::u16string& text) {
  WordBreak word_break = WordBreak::kNormal;
  auto parent = Parent();
  while (parent && parent->IsBaseTextShadowNode()) {
    auto text_node = static_cast<BaseTextShadowNode*>(parent);
    if (text_node->text_style_ && text_node->text_style_->word_break) {
      word_break = text_node->text_style_->word_break.value();
      break;
    }
    parent = parent->Parent();
  }
  if (word_break == WordBreak::kBreakAll) {
    std::u16string res;
    res.reserve(text.length());
    for (size_t i = 0; i < text.length(); i++) {
      res.push_back(text[i]);
      if (i < text.length() - 1 && !lynx::base::IsLeadingSurrogate(text[i]) &&
          text[i + 1] != kZeroWidthJoinerCharacter &&
          text[i] != kZeroWidthJoinerCharacter) {
        res.push_back(kZeroWidthSpaceCharacter);
      }
    }
    return res;
  } else if (word_break == WordBreak::kKeepAll) {
    std::u16string res;
    res.reserve(text.length());
    for (size_t i = 0; i < text.length(); i++) {
      res.push_back(text[i]);
      if (i < text.length() - 1 && icu_substitute::IsCJKCharacter(text[i]) &&
          !ispunct(text[i]) && icu_substitute::IsCJKCharacter(text[i + 1]) &&
          !ispunct(text[i + 1])) {
        res.push_back(kZeroWidthWordJoinerCharacter);
      }
    }
    return res;
  } else {
    return text;
  }
}

TextShadowNode* RawTextShadowNode::FindTextShadowNodeAncestor() {
  auto parent = Parent();
  while (parent) {
    if (parent->IsTextShadowNode()) {
      return static_cast<TextShadowNode*>(parent);
    } else {
      parent = parent->Parent();
    }
  }
  return nullptr;
}

BaseTextShadowNode* RawTextShadowNode::FindBaseTextShadowNodeAncestor() {
  auto parent = Parent();
  while (parent) {
    if (parent->IsBaseTextShadowNode()) {
      return static_cast<BaseTextShadowNode*>(parent);
    }
    parent = parent->Parent();
  }
  return nullptr;
}

void RawTextShadowNode::AddTextWithInlineEmoji(LayoutContextText* text_context,
                                               const std::u16string& text,
                                               bool need_text_indent) {
  auto parent_text_shadow_node = FindTextShadowNodeAncestor();
  if (!parent_text_shadow_node ||
      !parent_text_shadow_node->IsBracketRichType()) {
    text_context->AddText(text, need_text_indent);
    return;
  }

  auto* style_node = FindBaseTextShadowNodeAncestor();
  float font_size = 14.f * Logical2ClayPixelRatio();
  if (style_node && style_node->text_style_ &&
      style_node->text_style_->font_size.has_value()) {
    font_size = style_node->text_style_->font_size.value();
  }
  uintptr_t view_context =
      reinterpret_cast<uintptr_t>(owner_ ? owner_->GetViewContext() : nullptr);

  size_t cursor = 0;
  while (cursor < text.size()) {
    size_t emoji_start = text.find(u'[', cursor);
    if (emoji_start == std::u16string::npos) {
      text_context->AddText(text.substr(cursor), need_text_indent);
      return;
    }
    if (emoji_start > cursor) {
      text_context->AddText(text.substr(cursor, emoji_start - cursor),
                            need_text_indent);
    }

    size_t emoji_end = emoji_start;
    if (!IsBracketEmojiCandidate(text, emoji_start, &emoji_end)) {
      text_context->AddText(text.substr(emoji_start, 1), need_text_indent);
      cursor = emoji_start + 1;
      continue;
    }

    auto emoji_text = text.substr(emoji_start, emoji_end - emoji_start);
    auto bitmap = ResolveInlineEmojiBitmap(view_context, emoji_text, font_size);
    if (!bitmap.has_value()) {
      text_context->AddText(emoji_text, need_text_indent);
      cursor = emoji_end;
      continue;
    }

    txt::PlaceholderRun placeholder(bitmap->layout_width, bitmap->layout_height,
                                    txt::PlaceholderAlignment::kMiddle,
                                    txt::TextBaseline::kAlphabetic, 0);
    text_context->EnsureInitialTextIndentIfNeeded(need_text_indent);
    int placeholder_id = text_context->AddPlaceholder(placeholder);
    if (placeholder_id >= 0) {
      text_context->AddInlineEmojiInfo(placeholder_id, std::move(*bitmap));
      inline_emoji_text_ranges_.push_back({emoji_start, emoji_end});
    } else {
      text_context->AddText(emoji_text, need_text_indent);
    }
    cursor = emoji_end;
  }
}

size_t RawTextShadowNode::CurrentRawTextEnd() const {
  return std::min(truncated_index_, text_.length());
}

size_t RawTextShadowNode::LayoutTextLength() const {
  if (!layout_text_to_raw_end_indices_.empty()) {
    return layout_text_to_raw_end_indices_.size() - 1;
  }
  return CurrentRawTextEnd();
}

size_t RawTextShadowNode::RawEndIndexForLayoutTextIndex(
    size_t layout_text_index) const {
  if (!layout_text_to_raw_end_indices_.empty()) {
    return layout_text_to_raw_end_indices_[std::min(
        layout_text_index, layout_text_to_raw_end_indices_.size() - 1)];
  }
  return std::min(layout_text_index, CurrentRawTextEnd());
}

void RawTextShadowNode::BuildIdentityLayoutTextMapping(size_t length) {
  layout_text_to_raw_end_indices_.reserve(length + 1);
  for (size_t i = 0; i <= length; ++i) {
    layout_text_to_raw_end_indices_.push_back(i);
  }
}

size_t RawTextShadowNode::GetLayoutTextLength() const {
  size_t base_length = LayoutTextLength();
  size_t layout_length = base_length;
  for (const auto& range : inline_emoji_text_ranges_) {
    if (range.start >= base_length || range.end <= range.start) {
      continue;
    }
    size_t range_end = std::min(range.end, base_length);
    layout_length -= range_end - range.start;
    layout_length += 1;
  }
  return layout_length;
}

size_t RawTextShadowNode::GetRawEndIndexForLayoutTextLength(
    size_t layout_text_length) const {
  size_t base_length = LayoutTextLength();
  size_t text_cursor = 0;
  size_t layout_cursor = 0;
  for (const auto& range : inline_emoji_text_ranges_) {
    if (range.start >= base_length || range.end <= range.start) {
      continue;
    }
    size_t range_start = std::max(text_cursor, range.start);
    size_t normal_text_length = range_start - text_cursor;
    if (layout_text_length <= layout_cursor + normal_text_length) {
      return RawEndIndexForLayoutTextIndex(
          text_cursor + (layout_text_length - layout_cursor));
    }
    layout_cursor += normal_text_length;
    text_cursor = range_start;

    size_t range_end = std::min(range.end, base_length);
    if (layout_text_length <= layout_cursor + 1) {
      return RawEndIndexForLayoutTextIndex(range_end);
    }
    ++layout_cursor;
    text_cursor = range_end;
  }

  if (layout_text_length <= layout_cursor + base_length - text_cursor) {
    return RawEndIndexForLayoutTextIndex(text_cursor +
                                         (layout_text_length - layout_cursor));
  }
  return RawEndIndexForLayoutTextIndex(base_length);
}

bool RawTextShadowNode::IfNeedTextIndent() {
  auto parent = Parent();
  while (parent) {
    if (parent->IsTextShadowNode() &&
        static_cast<BaseTextShadowNode*>(parent)
            ->text_style_->text_indent.has_value()) {
      return true;
    }
    parent = parent->Parent();
  }
  return false;
}

std::u16string RawTextShadowNode::CollapsesWhitespaces(
    const std::u16string& text,
    std::vector<size_t>* layout_text_to_raw_end_indices) {
  std::u16string result;
  result.reserve(text.length());
  if (layout_text_to_raw_end_indices) {
    layout_text_to_raw_end_indices->clear();
    layout_text_to_raw_end_indices->push_back(0);
  }

  size_t index = 0;
  while (index < text.length()) {
    if (iswspace(text[index])) {
      do {
        ++index;
      } while (index < text.length() && iswspace(text[index]));
      result.push_back(u' ');
    } else {
      result.push_back(text[index]);
      ++index;
    }
    if (layout_text_to_raw_end_indices) {
      layout_text_to_raw_end_indices->push_back(index);
    }
  }
  return result;
}

}  // namespace clay
