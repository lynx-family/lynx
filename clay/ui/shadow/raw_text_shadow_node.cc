// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/raw_text_shadow_node.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/string/string_utils.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/component/text/unicode_util.h"
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
  text_ = ProcessWordBreakIfNeed(origin_text_);
  auto need_text_indent = IfNeedTextIndent();
  LayoutContextText* text_context = static_cast<LayoutContextText*>(context);
  if (parent_ && parent_->IsInlineTextShadowNode()) {
    auto start = text_context->TextSizeIncludingPlaceholders();
    auto end = start + std::min(GetEndIndex(), text_.length());
    static_cast<InlineTextShadowNode*>(parent_)->AddTextRange(start, end);
  }
  auto parent_text_shadow_node = FindTextShadowNodeAncestor();
  std::optional<TextStyle> text_style = std::nullopt;
  if (parent_text_shadow_node) {
    text_style = parent_text_shadow_node->text_style_;
  }
  if (text_style.has_value() && text_style->white_space.has_value() &&
      (text_style->white_space == WhiteSpace::kNoWrap ||
       text_style->white_space == WhiteSpace::kNormal)) {
    auto text = CollapsesWhitespaces(text_.substr(0, GetEndIndex()));
    text_context->AddText(text, need_text_indent);
  } else {
    text_context->AddText(text_.substr(0, GetEndIndex()), need_text_indent);
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

std::u16string RawTextShadowNode::CollapsesWhitespaces(std::u16string text) {
  auto end = std::unique(text.begin(), text.end(), [](char16_t a, char16_t b) {
    return iswspace(a) && iswspace(b);
  });
  text.erase(end, text.end());
  std::replace_if(
      text.begin(), text.end(),
      [](char16_t c) { return c != ' ' && iswspace(c); }, u' ');
  return text;
}

}  // namespace clay
