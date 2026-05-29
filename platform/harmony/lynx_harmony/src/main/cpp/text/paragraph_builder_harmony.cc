// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_builder_harmony.h"

#include <renderer/css/css_style_utils.h>

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/text/emoji_resource_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/unicode_decode_utils.h"
namespace lynx {
namespace tasm {
namespace harmony {

namespace {

static constexpr std::string_view kRichTypeBracket = "bracket";
static constexpr char kBracketLeftDelimiter = '[';
static constexpr char kBracketRightDelimiter = ']';

}  // namespace

void ParagraphBuilderHarmony::Reset() {
  if (builder_) {
    OH_Drawing_DestroyTypographyHandler(builder_);
  }
  builder_ = OH_Drawing_CreateTypographyHandler(
      paragraph_style_->GetRawStruct(), font_collection_->GetRawStruct());
  text_no_wrap_and_after_break_ = false;
  needs_rebuilding_ = false;
  char16_count_ = 0;
  placeholder_signs_.clear();
  inline_emojis_.clear();
  cur_event_target_start_ = 0;
  event_target_stack_ = {};
  event_roots_.clear();
  effects_.reset();
  emoji_placeholder_size_stack_.clear();
  text_.clear();
}

void ParagraphBuilderHarmony::SetRichType(std::string_view rich_type) {
  emoji_parse_rule_.reset();
  if (rich_type == kRichTypeBracket) {
    emoji_parse_rule_ =
        EmojiParseRule{kBracketLeftDelimiter, kBracketRightDelimiter};
  }
}

void ParagraphBuilderHarmony::PushTextStyle(const TextStyleHarmony& style) {
  if (HasRichTypeParser()) {
    emoji_placeholder_size_stack_.emplace_back(
        static_cast<float>(style.GetEmojiPlaceholderSize()));
  }
  if (style.GradientColor()) {
    // Needs a rebuilding when we got a exact width and height to calculate the
    // gradient effect.
    needs_rebuilding_ = true;
  }
  if (!text_no_wrap_and_after_break_) {
    OH_Drawing_TypographyHandlerPushTextStyle(builder_, style.GetRawStruct());
    // Effects' lifetime should be as long as the Paragraph's.
    if (auto effect = style.GradientShaderEffect()) {
      // TODO(renzhongyue): extract the util function to a base public module.
      starlight::CSSStyleUtils::PrepareOptional(effects_);
      effects_->emplace_back(effect);
    }
  }
}

void ParagraphBuilderHarmony::PopTextStyle() {
  if (HasRichTypeParser() && !emoji_placeholder_size_stack_.empty()) {
    emoji_placeholder_size_stack_.pop_back();
  }
  if (!text_no_wrap_and_after_break_) {
    OH_Drawing_TypographyHandlerPopTextStyle(builder_);
  }
}

void ParagraphBuilderHarmony::PushTextEventTarget(
    int32_t sign, LynxEventPropStatus event_through,
    LynxEventPropStatus ignore_focus, LynxPointerEventsValue pointer_events) {
  cur_event_target_start_ = char16_count_;
  auto target = std::make_shared<TextEventTarget>(
      cur_event_target_start_, cur_event_target_start_ + 1, sign, event_through,
      ignore_focus, pointer_events);
  if (event_target_stack_.empty()) {
    event_roots_.emplace_back(target);
    event_target_stack_.push(target.get());
  } else {
    auto* parent = event_target_stack_.top();
    parent->AddChild(target);
    event_target_stack_.push(target.get());
  }
}

void ParagraphBuilderHarmony::PopTextEventTarget() {
  if (!event_target_stack_.empty()) {
    auto* target = event_target_stack_.top();
    target->SetEnd(char16_count_);
    event_target_stack_.pop();
  }
}

void ParagraphBuilderHarmony::AddText(const char* text) {
  if (text == nullptr || text[0] == '\0') {
    return;
  }

  if (HasRichTypeParser()) {
    AddRichTypeText(text);
    return;
  }

  AddPlainText(text);
}

void ParagraphBuilderHarmony::AddRichTypeText(std::string_view text) {
  if (text.empty() || !emoji_parse_rule_.has_value()) {
    return;
  }

  std::string_view input(text);
  const auto rule = *emoji_parse_rule_;
  size_t pos = 0;
  while (pos < input.size()) {
    size_t left = input.find(rule.left_delimiter, pos);
    if (left == std::string_view::npos) {
      AddPlainText(input.substr(pos));
      return;
    }

    if (left > pos) {
      AddPlainText(input.substr(pos, left - pos));
    }

    size_t right = input.find(rule.right_delimiter, left + 1);
    if (right == std::string_view::npos) {
      AddPlainText(input.substr(left));
      return;
    }

    std::string_view name = input.substr(left + 1, right - left - 1);
    std::string_view raw_text = input.substr(left, right - left + 1);
    if (!name.empty() &&
        name.find(rule.left_delimiter) == std::string_view::npos &&
        TryAddEmojiPlaceholder(name)) {
      pos = right + 1;
      continue;
    }
    AddPlainText(raw_text);
    pos = right + 1;
  }
}

void ParagraphBuilderHarmony::AddPlainText(std::string_view text) {
  if (text.empty()) {
    return;
  }
  if (char16_count_ >= max_char16_count_) {
    return;
  }
  auto u16text = base::U8StringToU16(text);
  int32_t text_char16_count = u16text.length();
  std::string text_string;
  if (max_char16_count_ - char16_count_ < text_char16_count) {
    auto splice_count = max_char16_count_ - char16_count_;
    if (splice_count <= 0) {
      return;
    }
    if (base::IsLeadingSurrogate(u16text[splice_count - 1])) {
      splice_count--;
    }
    auto u16text_splice = std::u16string_view(u16text).substr(0, splice_count);
    text_string = base::U16StringToU8(u16text_splice);
    text = text_string;
    text_char16_count = splice_count;
  }
  if (paragraph_style_->GetWhiteSpace() == starlight::WhiteSpaceType::kNowrap) {
    // white-space:nowrap, do not add text after first hard break
    if (!text_no_wrap_and_after_break_) {
      const size_t break_pos = text.find('\n');
      if (break_pos == std::string_view::npos) {
        AddTextToTypographyBuilder(text);
        char16_count_ += text_char16_count;
      } else {
        std::string_view str_before_break = text.substr(0, break_pos);
        AddTextToTypographyBuilder(str_before_break);
        text_no_wrap_and_after_break_ = true;
        char16_count_ += base::U8StringToU16(str_before_break).length();
      }
    }
  } else {
    AddTextToTypographyBuilder(text);
    char16_count_ += text_char16_count;
  }
}

void ParagraphBuilderHarmony::AddTextToTypographyBuilder(
    std::string_view text) {
  if (text.empty()) {
    return;
  }
  std::string text_string(text);
  OH_Drawing_TypographyHandlerAddText(builder_, text_string.c_str());
  text_ += text_string;
}

bool ParagraphBuilderHarmony::TryAddEmojiPlaceholder(std::string_view name) {
  auto& emoji_resource_manager = EmojiResourceManager::GetInstance();
  if (!emoji_resource_manager.GetEmojiImage(name)) {
    return false;
  }
  const float emoji_size = CurrentEmojiPlaceholderSize();
  if (emoji_size <= 0) {
    return false;
  }

  InlineEmojiInfo emoji;
  emoji.name = std::string(name);
  emoji.placeholder_index = placeholder_signs_.size();
  emoji.width = emoji_size;
  emoji.height = emoji_size;

  PlaceholderHarmony placeholder(emoji.width, emoji.height,
                                 ALIGNMENT_CENTER_OF_ROW_BOX, 0.f);
  if (!AddPlaceholder(placeholder, -1)) {
    return false;
  }
  inline_emojis_.emplace_back(std::move(emoji));
  text_ += u8"\uFFFC";
  return true;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
